#include "XbeeEndDevice.h"
#include "Util.h"
#include <XBee.h>

XbeeEndDevice::XbeeEndDevice( )
	: XbeeDev( ), CtsPin( -1 ), RtsPin( -1 ), ResetPin( -1 ), AssociatedFlag( false ), CurrentApiId( -1 )
{
}

void XbeeEndDevice::begin( Stream& stream, int cts, int rts, int reset )
{	
  	CtsPin = cts;
	RtsPin = rts;
	ResetPin = reset;
	AssociatedFlag = false;
	CurrentApiId = -1;

	// Setup hardware
	if( CtsPin != -1 )
		pinMode( CtsPin, INPUT );

	if( RtsPin != -1 )
		pinMode( RtsPin, OUTPUT );
		
	if( ResetPin != -1 )
		pinMode( ResetPin, OUTPUT );
		
	// Setup XBee
	XbeeDev.setSerial( stream );		
	assertRts( );
	hardReset( );

	// Enable API mode with character escaping.
	// Needed by XBee-Arduino lib
	configure( "AP", 2, 1 );

	// Configure RTS/CTS if enabled
	configure( "D7", ( CtsPin < 0 ? 0 : 1 ), 1 );
	configure( "D6", ( RtsPin < 0 ? 0 : 1 ), 1 );
}

void XbeeEndDevice::join( uint64_t pan_id, uint16_t channel )
{
	configure( "ID", pan_id, sizeof( pan_id ), true );
	configure( "SC", channel, sizeof( channel ), true );
}

bool XbeeEndDevice::joined( void ) const
{
	return AssociatedFlag;
}

void XbeeEndDevice::tick( void )
{
	XbeeDev.readPacket();
	
	if( XbeeDev.getResponse( ).isAvailable( ) )
	{
		CurrentApiId = XbeeDev.getResponse( ).getApiId( );
		if( CurrentApiId == ZB_RX_RESPONSE )
		{
			XbeeDev.getResponse( ).getZBRxResponse( XbeeRxIndication );
		}
		else if( CurrentApiId == AT_COMMAND_RESPONSE )
		{
			XbeeDev.getResponse( ).getAtCommandResponse( XbeeAtCommandResp );	
		}
		else if( CurrentApiId == MODEM_STATUS_RESPONSE ) 
		{
			XbeeDev.getResponse( ).getModemStatusResponse( XbeeModemStatus );
			if( XbeeModemStatus.getStatus( ) == ASSOCIATED )
			{
				AssociatedFlag = true;
			}
			else if( XbeeModemStatus.getStatus( ) == DISASSOCIATED )
			{
				AssociatedFlag = false;
			}
			else if( XbeeModemStatus.getStatus( ) == COORDINATOR_STARTED )
			{
				AssociatedFlag = true;
			}
		}
		else if( CurrentApiId == ZB_TX_STATUS_RESPONSE )
		{
			XbeeDev.getResponse( ).getZBTxStatusResponse( XbeeTxResponse );		
		}		
	}
}

int XbeeEndDevice::send( const uint8_t* buffer, int buffer_size )
{
	int bytes  = 0;
	
	if( !buffer )
		return 0;
	
	if( !waitForCts( 200 ) )
		return XBEE_CTS_TIMEOUT;
	
	XbeeTxRequest.setPayload( ( uint8_t* ) buffer );
	XbeeTxRequest.setPayloadLength( buffer_size );
	XBeeAddress64 coordinator( 0, 0 );
	XbeeTxRequest.setAddress64( coordinator  );

	XbeeDev.send( XbeeTxRequest );

	if( waitForMessage( ZB_TX_STATUS_RESPONSE, 2000 )  )
	{
		if( XbeeTxResponse.getDeliveryStatus( ) == SUCCESS )
			return buffer_size;
		else
			return XBEE_NOT_DELIVERED;
	}

	return XBEE_RESPONSE_TIMEOUT; // response timeout
}

int XbeeEndDevice::available( void )
{
	if( CurrentApiId == ZB_RX_RESPONSE )
	{
		CurrentApiId = -1;
		return XbeeRxIndication.getDataLength( );
	}
        
	return 0;
}

int XbeeEndDevice::receive( uint8_t* buffer, int buffer_size )
{
	int bytes = XbeeRxIndication.getDataLength( );
	
	if( bytes > buffer_size )
		bytes = buffer_size;
	
	memcpy( buffer, XbeeRxIndication.getData( ), bytes );
	
	return bytes;
}

int XbeeEndDevice::configure( const char* command, uint64_t value, int valueLen, bool commit )
{
	uint64_t currentValue = 0;
	uint64_t temp = 0;
	bool commitNeeded = false;

	int error = getATCommand( command, &temp, valueLen );
	swapBytes( &currentValue, &temp, valueLen );	// change endianess
	if( !error && currentValue != value )
	{
  		swapBytes( &temp, &value, valueLen );	// change endianess
  		error = setATCommand( command, &temp, valueLen );
  		commitNeeded = true;
	}

	if( commit && commitNeeded )
		error = setATCommand( "WR" );

	return error;
}

int XbeeEndDevice::setATCommand( const char* command, const void* value, int value_len )
{
	XbeeAtCommandReq.setCommand( ( uint8_t* )command );
	XbeeAtCommandReq.clearCommandValue( );
        
	if( value )
		XbeeAtCommandReq.setCommandValue( ( uint8_t* )value );

	if( value_len )
		XbeeAtCommandReq.setCommandValueLength( ( uint8_t )value_len );
	
	// Transmit command.
	XbeeDev.send( XbeeAtCommandReq );

	if( waitForMessage( AT_COMMAND_RESPONSE, 2000 ) )
	{
		if( XbeeAtCommandResp.isOk( ) )
			return XBEE_NO_ERROR;
		else if( XbeeAtCommandResp.getStatus( ) == AT_INVALID_COMMAND )
			return XBEE_AT_CMD_ERROR;
		else
			return XBEE_AT_PARAM_ERROR;
	}

	return XBEE_RESPONSE_TIMEOUT;
}

int XbeeEndDevice::getATCommand( const char* command, void* value, int value_len )
{	
	XbeeAtCommandReq.setCommand( ( uint8_t* )command );
	XbeeAtCommandReq.clearCommandValue( );

	// Transmit command.
	XbeeDev.send( XbeeAtCommandReq );

	if( waitForMessage( AT_COMMAND_RESPONSE, 2000 ) )
	{
		if( XbeeAtCommandResp.isOk( ) )
		{
			uint8_t size = XbeeAtCommandResp.getValueLength( );
			if( size > value_len )
				size = value_len;
			
			memcpy( value, XbeeAtCommandResp.getValue( ), size );
			return size;
		}
		else
		{
			return XBEE_AT_CMD_ERROR;
		}
	}
	
	return XBEE_RESPONSE_TIMEOUT;
}

uint64_t XbeeEndDevice::getAddress64( void )
{
	uint32_t msw = 0;
	getATCommand( "SH", &msw, sizeof( msw ) );
	msw = networkToHostLong( msw );

	uint32_t lsw = 0;
	getATCommand( "SL", &lsw, sizeof( lsw ) );
	lsw = networkToHostLong( lsw );

	return ( ( ( uint64_t )msw << 32 ) | lsw );
}

uint16_t XbeeEndDevice::getAddress16( void )
{
	uint16_t address = 0;
	getATCommand( "MY", &address, sizeof( address ) );
	return networkToHostShort( address );
}

uint16_t XbeeEndDevice::getOperatingPanId( void )
{
	uint16_t panID = 0;
	getATCommand( "OI", &panID, sizeof( panID ) );
	return networkToHostShort( panID );
}
	
void XbeeEndDevice::hardReset( void )
{
	if( ResetPin != -1 )
	{
		digitalWrite( ResetPin, LOW );
		digitalWrite( ResetPin, HIGH ); 
		delay( 2 );
		digitalWrite( ResetPin, LOW );

		// process modem status indication
		// after reset.
		for( int i=0; i<50; i++ )
		{
			tick( );
			delay( 10 );
		}
	}
}

void XbeeEndDevice::assertRts( void )
{
	if( RtsPin != -1 )
		digitalWrite( RtsPin, LOW );
}

void XbeeEndDevice::deassertRts( void )
{
	if( RtsPin != -1 )
		digitalWrite( RtsPin, HIGH );
}
	
bool XbeeEndDevice::waitForCts( int timeout )
{
	if( CtsPin != -1 )
	{
		long start = millis( );
		
		while( digitalRead( CtsPin ) == HIGH )
		{
			if( timeout && ( millis( ) - start >= timeout ) )
			{
				// timeout
				return false;
			} 
		}
	}
	
	return true;
}

bool XbeeEndDevice::waitForMessage( int api_id, int timeout )
{
	long start = millis( );
	
	// reset last received message.
	CurrentApiId = -1;
	
	while( CurrentApiId != api_id )
	{
		tick( );
		
		if( timeout && ( millis( ) - start >= timeout ) )
		{
			// timeout
			return false;
		}
	}
	
	return true;
}


#include "XbeeCoordinator.h"
#include "Util.h"
#include <XBee.h>

#define CTS_TIMEOUT_MS       ( 300 )
#define RESPONSE_TIMEOUT_MS  ( 2000 )

XbeeCoordinator::XbeeCoordinator( )
	: XbeeDev( ), CtsPin( -1 ), RtsPin( -1 ), ResetPin( -1 ), CoordinatorStarted( false ), MessageReceivedFlag( false ), CurrentApiId( -1 ), CurrentFrameId( -1 )
{
}

void XbeeCoordinator::begin( Stream& stream, int cts, int rts, int reset )
{	
  	CtsPin = cts;
	RtsPin = rts;
	ResetPin = reset;
	CoordinatorStarted = false;
	CurrentApiId = -1;
	CurrentFrameId = -1;

	// Setup hardware
	if( CtsPin != -1 )
		pinMode( CtsPin, INPUT );

	if( RtsPin != -1 )
		pinMode( RtsPin, OUTPUT );
		
	if( ResetPin != -1 )
		pinMode( ResetPin, OUTPUT );
		
	// Setup XBee
	XbeeDev.setSerial( stream );		
	hardReset( );

	// Enable API mode with character escaping.
	// Needed by XBee-Arduino lib
	configure( "AP", 2, 1 );

	// Configure RTS/CTS if enabled
	configure( "D7", ( CtsPin < 0 ? 0 : 1 ), 1 );
	configure( "D6", ( RtsPin < 0 ? 0 : 1 ), 1 );
}

void XbeeCoordinator::tick( void )
{
	assertRts( );

	XbeeDev.readPacket();
	
	if( XbeeDev.getResponse( ).isAvailable( ) )
	{
		CurrentApiId = XbeeDev.getResponse( ).getApiId( );
		if( CurrentApiId == ZB_RX_RESPONSE )
		{
			XbeeDev.getResponse( ).getZBRxResponse( XbeeRxIndication );
			if( !JoinedNodeCache.find( XbeeRxIndication.getRemoteAddress16( ) ) )
			{
  			  JoinedNodeCache.add( XbeeRxIndication.getRemoteAddress16( ), 0 );
  			}
			MessageReceivedFlag = true;
		}
		else if( CurrentApiId == AT_COMMAND_RESPONSE )
		{
			XbeeDev.getResponse( ).getAtCommandResponse( XbeeAtCommandResp );	
			CurrentFrameId = XbeeAtCommandResp.getFrameId( );
		}
		else if( CurrentApiId == MODEM_STATUS_RESPONSE ) 
		{
			XbeeDev.getResponse( ).getModemStatusResponse( XbeeModemStatus );
			if( XbeeModemStatus.getStatus( ) == COORDINATOR_STARTED )
			{
				CoordinatorStarted = true;
			}
		}
		else if( CurrentApiId == ZB_TX_STATUS_RESPONSE )
		{
			XbeeDev.getResponse( ).getZBTxStatusResponse( XbeeTxResponse );		
			CurrentFrameId = XbeeTxResponse.getFrameId( );
		}		
		else if( CurrentApiId == ZB_IO_NODE_IDENTIFIER_RESPONSE  )
		{
			uint8_t* payload = XbeeDev.getResponse( ).getFrameData( );
			uint8_t size = XbeeDev.getResponse( ).getFrameDataLength( );
			
			ReceivedNodeID.parse( payload, size );
			if( !JoinedNodeCache.find( ReceivedNodeID.Address16 ) )
			{
				JoinedNodeCache.add( ReceivedNodeID.Address16, ReceivedNodeID );
				//JoinNotificationCallback( ReceivedNodeID.Address16 );
			}
		}
	}
	
	deassertRts( );
}

void XbeeCoordinator::start( uint64_t pan_id, uint16_t channel, uint16_t securityKey )
{
	CoordinatorStarted = false;
	configure( "ID", pan_id, sizeof( pan_id ) );
	configure( "SC", channel, sizeof( channel ) );	
	configure( "EE", ( securityKey == NO_SECURITY ?  0 : 1 ) , 1 );
	configure( "NK", 0, 1 );
	configure( "KY", ( securityKey == NO_SECURITY ?  0 : securityKey ), sizeof( securityKey ) );
}

bool XbeeCoordinator::started( void )
{
	if( !CoordinatorStarted )
	{
		uint8_t ai = 0;
		int error = getATCommand( "AI", &ai, sizeof( uint8_t ) );
		if( error > 0 && !ai )
			CoordinatorStarted = true;
	}
	return CoordinatorStarted;
}

uint64_t XbeeCoordinator::getAddress64( void )
{
	uint32_t msw = 0;
	getATCommand( "SH", &msw, sizeof( msw ) );
	msw = networkToHostLong( msw );

	uint32_t lsw = 0;
	getATCommand( "SL", &lsw, sizeof( lsw ) );
	lsw = networkToHostLong( lsw );

	return ( ( ( uint64_t )msw << 32 ) | lsw );
}

uint16_t XbeeCoordinator::getAddress16( void )
{
	uint16_t address = 0;
	getATCommand( "MY", &address, sizeof( address ) );
	return networkToHostShort( address );
}

uint16_t XbeeCoordinator::getOperatingPanId( void )
{
	uint16_t panID = 0;
	getATCommand( "OI", &panID, sizeof( panID ) );
	return networkToHostShort( panID );
}

uint8_t XbeeCoordinator::getRSSI( void )
{
	// in -dBm, range for Xbee: 0x1A - 0x5C (pg.134)
	uint8_t rssi = 0;
	getATCommand( "DB", &rssi, sizeof( rssi ) );
	return rssi;
}
/*
int XbeeCoordinator::setNodeID( const char* id )
{
	int len = strlen( id );
	if( len > 20 )
		len = 20;
	return setATCommand( "NI", id, len );
}

int XbeeCoordinator::getNodeID( char* buffer, int len )
{		
	return getATCommand( "NI", buffer, len );
}

int XbeeCoordinator::resolveNodeID( const char* id, uint64_t* address64, uint16_t* address16 )
{
	uint8_t buffer[10]; // 16-bit + 64-bit addresses.

	int error = setATCommand( "DN", buffer, 
}
*/
int XbeeCoordinator::discoverNodes( void )
{
  return 0; 
}

int XbeeCoordinator::getNumberOfJoinedNodes( void )
{
  return JoinedNodeCache.size( ); 
}

XbeeNodeID* XbeeCoordinator::getNodeByIndex( int index )
{
  return JoinedNodeCache.at( index ); 
}

XbeeNodeID* XbeeCoordinator::getNodeByAddress( uint16_t address16 )
{
  return JoinedNodeCache.find( address16 ); 
}

int XbeeCoordinator::send( const uint8_t* buffer, int buffer_size, uint16_t destination )
{
	if( !buffer )
		return 0;

	if( !waitForCts( CTS_TIMEOUT_MS ) )
		return XBEE_CTS_TIMEOUT;

	int bytes  = 0;		
	int frameId = XbeeDev.getNextFrameId( );
	XbeeTxRequest.setFrameId( frameId );
	XbeeTxRequest.setPayload( ( uint8_t* ) buffer );
	XbeeTxRequest.setPayloadLength( buffer_size );
	XBeeAddress64 coordinator( 0, 0 );
	XbeeTxRequest.setAddress64( coordinator  );

	XbeeDev.send( XbeeTxRequest );

	if( waitForResponse( ZB_TX_STATUS_RESPONSE, frameId, RESPONSE_TIMEOUT_MS )  )
	{
		if( XbeeTxResponse.getDeliveryStatus( ) == SUCCESS )
			return buffer_size;
		else
			return XBEE_NOT_DELIVERED;
	}

	return XBEE_RESPONSE_TIMEOUT;
}

int XbeeCoordinator::available( void )
{
	if( MessageReceivedFlag )
	{
		MessageReceivedFlag = false;
		return XbeeRxIndication.getDataLength( );
	}
        
	return 0;
}

int XbeeCoordinator::receive( uint8_t* buffer, int buffer_size, uint16_t* source )
{
	int bytes = XbeeRxIndication.getDataLength( );
	
	if( bytes > buffer_size )
		bytes = buffer_size;
	
	memcpy( buffer, XbeeRxIndication.getData( ), bytes );
	
	if( source )
		*source = XbeeRxIndication.getRemoteAddress16( );
		
	return bytes;
}

void XbeeCoordinator::hardReset( void )
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

int XbeeCoordinator::configure( const char* command, uint64_t value, int valueLen, bool commit )
{
	uint64_t currentValue = 0;
	uint64_t temp = 0;
	bool commitNeeded = false;

	int error = getATCommand( command, &temp, valueLen );
	swapBytes( &currentValue, &temp, valueLen );	// change endianess
	if( error >= 0 && currentValue != value )
	{
		swapBytes( &temp, &value, valueLen );	// change endianess
  		error = setATCommand( command, &temp, valueLen );
  		commitNeeded = true;
	}

	if( commit && commitNeeded )
		error = setATCommand( "WR" );

	return error;
}

int XbeeCoordinator::setATCommand( const char* command, const void* value, int value_len )
{
	if( !waitForCts( CTS_TIMEOUT_MS ) )
		return XBEE_CTS_TIMEOUT;
		
	int frameId = XbeeDev.getNextFrameId( );
	XbeeAtCommandReq.setFrameId( frameId );
	XbeeAtCommandReq.setCommand( ( uint8_t* )command );
	XbeeAtCommandReq.clearCommandValue( );
        
	if( value )
		XbeeAtCommandReq.setCommandValue( ( uint8_t* )value );

	if( value && value_len )
		XbeeAtCommandReq.setCommandValueLength( ( uint8_t )value_len );
	
	// Transmit command.
	XbeeDev.send( XbeeAtCommandReq );

	if( waitForResponse( AT_COMMAND_RESPONSE, frameId, RESPONSE_TIMEOUT_MS ) )
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

int XbeeCoordinator::getATCommand( const char* command, void* value, int value_len )
{	
	if( !waitForCts( CTS_TIMEOUT_MS ) )
		return XBEE_CTS_TIMEOUT;
		
	int frameId = XbeeDev.getNextFrameId( );
	XbeeAtCommandReq.setFrameId( frameId );
	XbeeAtCommandReq.setCommand( ( uint8_t* )command );
	XbeeAtCommandReq.clearCommandValue( );

	// Transmit command.
	XbeeDev.send( XbeeAtCommandReq );

	if( waitForResponse( AT_COMMAND_RESPONSE, frameId, RESPONSE_TIMEOUT_MS ) )
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

void XbeeCoordinator::assertRts( void )
{
	if( RtsPin != -1 )
	{
		digitalWrite( RtsPin, LOW );	
		// allow UART buffer to receive some data.
		delay( 100 );
	}
}

void XbeeCoordinator::deassertRts( void )
{
	if( RtsPin != -1 )
		digitalWrite( RtsPin, HIGH );
}
	
bool XbeeCoordinator::waitForCts( int timeout )
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

bool XbeeCoordinator::waitForResponse( int api_id, int frame_id, int timeout )
{
	long start = millis( );
	
	// reset last received response.
	CurrentApiId = -1;
	CurrentFrameId = -1;

	while( CurrentApiId != api_id &&
		   CurrentFrameId != frame_id )
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

////////////////////////////////////////////////////////////////////////////////////////////
// XbeeNodeID
////////////////////////////////////////////////////////////////////////////////////////////

XbeeNodeID::XbeeNodeID( )
{
	reset( );
}

XbeeNodeID::XbeeNodeID( uint16_t address16, uint16_t address64 )
{
	reset( );
	Address16 = address16;
	Address64 = address64;
}

XbeeNodeID::XbeeNodeID( uint8_t* buffer, int len )
{
	reset( );
	parse( buffer, len );
}

void XbeeNodeID::parse( uint8_t* buffer, int len )
{
Serial1.print("NODEID len=");
Serial1.println(len);
for( int i=0; i<len; i++)
{
  Serial1.print( buffer[i], HEX );
  Serial1.print( ' ' );
}
Serial1.println();

	if( len < 30 )
		return;

	// Source 16-bit address;
	swapBytes( &Address16, &buffer[11], sizeof( Address16 ) );
	// Source 64-bit address;
	swapBytes( &Address64, &buffer[13], sizeof( Address64 ) );
Serial1.print("16=");
Serial1.print( Address16, HEX );
Serial1.print(",64=");
Serial1.print( ( uint32_t )( Address64 >> 32 ), HEX );
Serial1.print( ( uint32_t )Address64, HEX );

	//int str_len = len - 33 - 1;
	int str_len = strlen( ( const char* )&buffer[21] );
	if( str_len <= 20 )
		strcpy( ID, ( const char* )&buffer[21] );

Serial1.print(",str_len=");
Serial1.print(str_len);
Serial1.print(",id=");
Serial1.print(ID);

	DeviceType = buffer[21+str_len+1+2];
Serial1.print(",t=");
Serial1.println(DeviceType);

}

void XbeeNodeID::reset( void )
{
	Address16 = 0;
	Address64 = 0;
	DeviceType = 0;
	*ID = '\0';
}


////////////////////////////////////////////////////////////////////////////////////////////
// XbeeNodeCache
////////////////////////////////////////////////////////////////////////////////////////////
XbeeNodeCache::XbeeNodeCache( void )
{
	for( int i=0; i<10; i++ )
		Cache[i].free = true;
}
	
void XbeeNodeCache::add( uint16_t address16, const XbeeNodeID& node )
{
	for( int i=0; i<10; i++ )
	{
		if( Cache[i].free )
		{
			Cache[i].node = node;
			Cache[i].free = false;
			return;
		}
	}
}

void XbeeNodeCache::add( uint16_t address16, const uint64_t& address64 )
{
	for( int i=0; i<10; i++ )
	{
		if( Cache[i].free )
		{
  			Cache[i].node.reset( );
  			Cache[i].node.Address16 = address16;
  			Cache[i].node.Address64 = address64;  
			Cache[i].free = false;
			return;
		}
	}
}

void XbeeNodeCache::remove( uint16_t address )
{
	for( int i=0; i<10; i++ )
	{
		if( !Cache[i].free && 
			Cache[i].node.getAddress16( ) == address )
		{
			Cache[i].free = true;
			return;
		}
	}
}

XbeeNodeID* XbeeNodeCache::at( int index )
{
	int j = 0;

	for( int i=0; i<10; i++ )
	{
		if( !Cache[i].free && j++ == index)
		{
  			return &Cache[i].node;
		}
	}
	
	return 0;
}
	
XbeeNodeID* XbeeNodeCache::find( uint16_t address )
{
	for( int i=0; i<10; i++ )
	{
		if( !Cache[i].free &&  Cache[i].node.getAddress16( ) == address )
			return &Cache[i].node;
	}
	
	return 0;
}
	
int XbeeNodeCache::size( void )
{
	int size = 0;
	for( int i=0; i<10; i++ )
	{
		if( !Cache[i].free )
			size++;
	}
	return size;
}	

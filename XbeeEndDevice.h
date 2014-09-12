#ifndef XBEE_WRAPPER_H
#define XBEE_WRAPPER_H

#include <avr/stdint.h>
#include <XBee.h>

// Default values
#define PAN_ID_ANY			( 0 )
#define ALL_CHANNELS 		( 0xFFFF )
#define NO_SECURITY			( 0 )

// Error codes
#define XBEE_NO_ERROR  			( 0 )
#define XBEE_RESPONSE_TIMEOUT 		( -1 )
#define XBEE_CTS_TIMEOUT		( -2 )
#define XBEE_NOT_DELIVERED		( -3 )
#define XBEE_AT_PARAM_ERROR		( -4 )
#define XBEE_AT_CMD_ERROR		( -5 )

class XbeeEndDevice
{
public:
	XbeeEndDevice( );
	void begin( Stream& stream, int cts = -1, int rts = -1, int reset = -1 );
	void tick( void );

	void join( uint64_t pan_id = PAN_ID_ANY, uint16_t channel = ALL_CHANNELS, securityKey = NO_SECURITY );
	bool joined( void );	
	uint64_t getAddress64( void );
	uint16_t getAddress16( void );
	uint16_t getOperatingPanId( void );
	uint8_t getRSSI( void );
	
	int send( const uint8_t* buffer, int buffer_size );
	int available( void );
	int receive( uint8_t* buffer, int buffer_size );
	
	void hardReset( void );

private:
	int configure( const char* command, uint64_t value, int valueLen, bool commit = false );
	int setATCommand( const char* command, const void* value = NULL, int value_len = 0 );
	int getATCommand( const char* command, void* value, int value_len );
	void assertRts( void );
	void deassertRts( void );
	bool waitForCts( int timeout = 0 );
	bool waitForResponse( int api_id, int frame_id, int timeout = 0 );

	XBee XbeeDev;
	ZBTxRequest XbeeTxRequest;
	ZBTxStatusResponse XbeeTxResponse;	
	ZBRxResponse XbeeRxIndication;
	ModemStatusResponse XbeeModemStatus;
	AtCommandRequest XbeeAtCommandReq;
	AtCommandResponse XbeeAtCommandResp;
	int CtsPin, RtsPin, ResetPin;
	bool AssociatedFlag;
	bool MessageReceivedFlag;
	int CurrentApiId;
	int CurrentFrameId;
};

#endif

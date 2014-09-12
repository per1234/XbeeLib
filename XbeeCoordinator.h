#ifndef XBEE_COORDINATOR_H
#define XBEE_COORDINATOR_H

#include <avr/stdint.h>
#include <XBee.h>

// Default values
#define PAN_ID_ANY			( 0 )
#define ALL_CHANNELS 		( 0xFFFF )
#define NO_SECURITY			( 0 )

// Error codes
#define XBEE_NO_ERROR  			( 0 )
#define XBEE_RESPONSE_TIMEOUT 	( -1 )
#define XBEE_CTS_TIMEOUT		( -2 )
#define XBEE_NOT_DELIVERED		( -3 )
#define XBEE_AT_PARAM_ERROR		( -4 )
#define XBEE_AT_CMD_ERROR		( -5 )


class XbeeNodeID 
{
public:
	XbeeNodeID( );
	XbeeNodeID( uint16_t address16, uint16_t address64 );
	XbeeNodeID( uint8_t* buffer, int len );
	
	uint16_t getAddress16( void ) const { return Address16; }
	uint64_t getAddress64( void ) const { return Address64; }
	uint8_t getDeviceType( void ) const  { return DeviceType; }
	const char* getIDString( void ) const { return ID; }

private:
	void parse( uint8_t* buffer, int len );
	void reset( void );
	
	uint16_t Address16;
	uint64_t Address64;
	uint8_t DeviceType;
	char ID[20+1];

	friend class XbeeCoordinator;
	friend class XbeeNodeCache;
};

class XbeeNodeCache
{
public:
	XbeeNodeCache( void );
	void add( uint16_t address16, const XbeeNodeID& node );
	void add( uint16_t address16, const uint64_t& address64 );
	void remove( uint16_t address );
	XbeeNodeID* at( int index );	
	XbeeNodeID* find( uint16_t address );	
	int size( void );

private:
	struct {
		bool free;
		XbeeNodeID node;
	} Cache[10];
};

class XbeeCoordinator
{
public:
	XbeeCoordinator( );
	void begin( Stream& stream, int cts = -1, int rts = -1, int reset = -1 );
	void tick( void );

	void start( uint64_t pan_id = PAN_ID_ANY, uint16_t channel = ALL_CHANNELS, uint16_t securityKey = NO_SECURITY );
	bool started( void );	
	uint64_t getAddress64( void );
	uint16_t getAddress16( void );
	uint16_t getOperatingPanId( void );
	uint8_t getRSSI( void );
	
	int discoverNodes( void );
	int getNumberOfJoinedNodes( void );
	XbeeNodeID* getNodeByIndex( int index );
	XbeeNodeID* getNodeByAddress( uint16_t address16 );
	
	int send( const uint8_t* buffer, int buffer_size, uint16_t destination = 0xFFFE );
	int available( void );
	int receive( uint8_t* buffer, int buffer_size, uint16_t* source = 0 );
	
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
	bool CoordinatorStarted;
	bool MessageReceivedFlag;
	int CurrentApiId;
	int CurrentFrameId;
int DiscoveryFrameId;
	XbeeNodeCache JoinedNodeCache;
	XbeeNodeID ReceivedNodeID;
};


#endif

#include <XBee.h>
//#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include "XbeeCoordinator.h"

XbeeCoordinator Coordinator;
//SoftwareSerial SoftSerial( 8, 9 );
#define SoftSerial  Serial1

void setup()
{
	Serial.begin( 9600 );
	Coordinator.begin( Serial );

	SoftSerial.begin( 9600 );
	SoftSerial.println( "XbeeCoordinator started." );
        SoftSerial.print( "MAC Address : 0x" );
        SoftSerial.print( ( uint32_t )( Coordinator.getAddress64( ) >> 32 ), HEX );
        SoftSerial.println( ( uint32_t ) Coordinator.getAddress64( ) , HEX );        

	SoftSerial.print( "Attempting to start a network." );
	Coordinator.start( );
	while( !Coordinator.started( ) )
	{
		Coordinator.tick( );
		delay( 500 );
		SoftSerial.print( '.' );
	}
	SoftSerial.println( "Started network!" );
	SoftSerial.print( "Network PAN ID : " );
        SoftSerial.println( Coordinator.getOperatingPanId( ), HEX );
        SoftSerial.print( "Network Address : " );
        SoftSerial.println( Coordinator.getAddress16( ), HEX );      
}

void loop() 
{   
	Coordinator.tick( );
	if( Coordinator.available( ) )
	{
		char buffer[40+1];
                uint16_t sourceAddress = 0;
		Coordinator.receive( ( uint8_t* )buffer, 40, &sourceAddress );
		buffer[40] = '\0';
		
		SoftSerial.print( "Received from 0x" );
		SoftSerial.print( sourceAddress, HEX );
		SoftSerial.print( ": " );
		SoftSerial.println( buffer );
	}

        static long start = millis( );
	if( Coordinator.started( ) && millis( ) - start >= 5000 )
	{
                int nodes_number = Coordinator.getNumberOfJoinedNodes( );
                SoftSerial.print( "Discovered Nodes=" );
                SoftSerial.println( nodes_number );  
                for( int i=0; i<nodes_number; i++ )
                {
        		XbeeNodeID* node = Coordinator.getNodeByIndex( i );

                        SoftSerial.print( "Node[" );
                        SoftSerial.print( i );
                        SoftSerial.print( "]: Address16=0x" );
                        SoftSerial.print( node->getAddress16( ), HEX );
                        SoftSerial.print( ", Address64=0x" );
                        SoftSerial.print( ( uint32_t )( node->getAddress64( ) >> 32 ), HEX );
                        SoftSerial.print( ( uint32_t )node->getAddress64( ), HEX );                        
                        SoftSerial.print( ", Type=" );
                        SoftSerial.print( node->getDeviceType( ), HEX );                
                        SoftSerial.print( ", ID=" );
                        SoftSerial.println( node->getIDString( ) );
                        //SoftSerial.print( "Sending " );
        		//const char* message = "Hello from Coordinator!";
        		//int bytes = Coordinator.send( ( const uint8_t* )message, strlen( message ), node->getAddress16( ) );
                        //SoftSerial.println( bytes );       
                }

		start = millis( );
	}
	
	if( !Coordinator.started( ) )
	{
		SoftSerial.println( "Network down." );
                SoftSerial.println( "Attempting to restart network" );
		delay( 1000 );
	}
	
	delay( 5 );
}

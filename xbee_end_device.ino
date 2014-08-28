#include <XBee.h>
#include <SoftwareSerial.h>
#include "XbeeEndDevice.h"

XbeeEndDevice EndDevice;
SoftwareSerial SoftSerial( 8, 9 );

void setup()
{
	Serial.begin( 9600 );
	EndDevice.begin( Serial, A4, A5, A2 );

	SoftSerial.begin( 9600 );
	SoftSerial.println( "XbeeEndDevice started." );
        SoftSerial.print( "MAC Address : 0x" );
        SoftSerial.print( ( uint32_t )( EndDevice.getAddress64( ) >> 32 ), HEX );
        SoftSerial.println( ( uint32_t ) EndDevice.getAddress64( ) , HEX );        

	SoftSerial.print( "Attempting to join a network." );
	EndDevice.join( );
	while( !EndDevice.joined( ) )
	{
		EndDevice.tick( );
		delay( 500 );
		SoftSerial.print( '.' );
	}

	SoftSerial.println( );
	SoftSerial.println( "Joined network!" );
	SoftSerial.print( "Network PAN ID : " );
        SoftSerial.println( EndDevice.getOperatingPanId( ), HEX );
        SoftSerial.print( "Network Address : " );
        SoftSerial.println( EndDevice.getAddress16( ), HEX );        
}

void loop() 
{   
	EndDevice.tick( );
	if( EndDevice.available( ) )
	{
		char buffer[20+1];
		EndDevice.receive( ( uint8_t* )buffer, 20 );
		buffer[20] = '\0';
		
		SoftSerial.print("Received: ");
		SoftSerial.println( buffer );	
	}

	static long start = 0;
	if( EndDevice.joined( ) && millis( ) - start >= 20000 )
	{
                SoftSerial.print( "Sending buffer " );
		const char* message = "Hello from ED!";
		int bytes = EndDevice.send( ( const uint8_t* )message, strlen( message ) );
                SoftSerial.println( bytes );
		start = millis( );
	}
	
	if( !EndDevice.joined( ) )
	{
		SoftSerial.println( "Disassociated." );
		delay( 900 );
	}
	
	delay( 100 );
}

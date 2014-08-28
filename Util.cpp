#include "Util.h"

void swapBytes( void* dest, void* src, int size )
{
	for( int i=0; i<size; i++ )
		*( ( unsigned char* )dest + i ) = *( ( unsigned char* )src + ( size - i - 1 ) );
}
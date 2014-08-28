#ifndef UTIL_H
#define UTIL_H

#if 0
inline unsigned long long hostToNetworkLongLong( unsigned long long word )
{
	return ( unsigned long long)( ( ( word >> 56 ) & 0x00000000000000ffLL ) |
			 ( ( word >> 32 ) & 0x000000000000ff00LL ) |
			 ( ( word >> 24 ) & 0x0000000000ff0000LL ) |
			 ( ( word >>  8 ) & 0x00000000ff000000LL )  |
			 ( ( word <<  8 ) & 0x000000ff00000000LL ) |
			 ( ( word << 24 ) & 0x0000ff0000000000LL ) |
			 ( ( word << 32 ) & 0x00ff000000000000LL ) |
			 ( ( word << 56 ) & 0xff00000000000000LL ) );
}

inline unsigned long long networkToHostLongLong( unsigned long long word )
{
	return ( unsigned long long)( ( ( word >> 56 ) & 0x00000000000000ffLL ) |
			 ( ( word >> 32 ) & 0x000000000000ff00LL ) |
			 ( ( word >> 24 ) & 0x0000000000ff0000LL ) |
			 ( ( word >>  8 ) & 0x00000000ff000000LL )  |
			 ( ( word <<  8 ) & 0x000000ff00000000LL ) |
			 ( ( word << 24 ) & 0x0000ff0000000000LL ) |
			 ( ( word << 32 ) & 0x00ff000000000000LL ) |
			 ( ( word << 56 ) & 0xff00000000000000LL ) );
}

inline unsigned long hostToNetworkLong( unsigned long word )
{
	return ( ( ( word >> 24 ) & 0x000000ff ) |
             ( ( word <<  8 ) & 0x00ff0000 ) |
			 ( ( word >>  8 ) & 0x0000ff00 ) |
			 ( ( word << 24 ) & 0xff000000 ) );
}

inline unsigned long networkToHostLong( unsigned long word )
{
	return ( ( ( word >> 24 ) & 0x000000ff ) |
             ( ( word <<  8 ) & 0x00ff0000 ) |
			 ( ( word >>  8 ) & 0x0000ff00 ) |
			 ( ( word << 24 ) & 0xff000000 ) );
}

inline unsigned short hostToNetworkLong( unsigned short word )
{
	return ( ( ( word <<  8 ) & 0xff00 ) |
			 ( ( word >>  8 ) & 0x00ff ) );
}

inline unsigned short networkToHostShort( unsigned short word )
{
	return ( ( ( word <<  8 ) & 0xff00 ) |
			 ( ( word >>  8 ) & 0x00ff ) );
}

#else
void swapBytes( void* dest, void* src, int size );

inline unsigned long long hostToNetworkLongLong( unsigned long long word )
{
	unsigned long long result = 0;
	swapBytes( &result, &word, sizeof( unsigned long long ) );
	return result;
}

inline unsigned long long networkToHostLongLong( unsigned long word )
{
	unsigned long long result = 0;
	swapBytes( &result, &word, sizeof( unsigned long long ) );
	return result;
}

inline unsigned long hostToNetworkLong( unsigned long word )
{
	unsigned long result = 0;
	swapBytes( &result, &word, sizeof( unsigned long ) );
	return result;
}

inline unsigned long networkToHostLong( unsigned long word )
{
	unsigned long result = 0;
	swapBytes( &result, &word, sizeof( unsigned long ) );
	return result;
}

inline unsigned short hostToNetworkLong( unsigned short word )
{
	unsigned short result = 0;
	swapBytes( &result, &word, sizeof( unsigned short ) );
	return result;
}

inline unsigned short networkToHostShort( unsigned short word )
{
	unsigned short result = 0;
	swapBytes( &result, &word, sizeof( unsigned short ) );
	return result;
}

#endif

#endif

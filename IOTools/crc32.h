#ifndef __TUM3D_CRC32_H_
#define __TUM3D_CRC32_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#ifndef DWORD
#define DWORD unsigned int	// works for Amiga and Windows -- other systems may ensure that sizeof(unsigned int)=4.
#endif

#ifndef bool
#define bool  char
#define true  1
#define false 0
#endif
*/
/*
 * CRC32 (802.3) implementation using table lookups
 */
class CRC32 {
public:
	CRC32(DWORD dwPoly=0x04C11DB7) {
		for (unsigned int ui=0; ui<256; ui++) {
			DWORD dwR = reflect(ui);
			for (int i=0; i<8; i++) dwR = ((dwR&0x80000000) ? (dwR<<1) ^ dwPoly : (dwR<<1));
			m_dwTable[ui]=reflect(dwR);
		}
	}
	~CRC32(void) {}
	inline DWORD get(const unsigned char *message, size_t stLength, DWORD start=0x0) const {
		DWORD dwR = start^0xFFFFFFFF;
		for (size_t st=0; st<stLength; st++) dwR = (dwR >> 8) ^ m_dwTable[(dwR&0xFF) ^ message[st]];
		return (dwR^0xFFFFFFFF);
	}
	inline DWORD get(const char *message, size_t stLength) const {
		return get((unsigned char*)(message),stLength);
	}	
private:
	inline DWORD reflect(DWORD dw) {
		for (unsigned int ui=0; ui<16; ui++) {
			DWORD dwR=(dw&(1<<ui));			// right bit
			DWORD dwL=(dw&(1<<(31-ui)));	// left bit
			dw^=dwR^dwL^(dwR<<(32-2*ui-1))^(dwL>>(32-2*ui-1));	// swap bits
		}
		return dw;
	}
	DWORD m_dwTable[256];
};

#endif

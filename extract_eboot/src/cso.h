
#ifndef _CSO_H_
#define _CSO_H_

#include "iso.h"
#include <zlib.h>


/*
 *
 * CSO decompress algorithm ported from Virtuous Flame's ciso.py's decompress_cso code :
 * http://code.google.com/p/procfw/downloads/detail?name=ciso.py
 *
 * CSO header struct taken from procfw source code :
 * http://code.google.com/p/procfw/source/browse/Vshctrl/isoreader.c
 *
*/


typedef struct _CISOHeader {
	uint8_t magic[4];			/* +00 : 'C','I','S','O'                           */
	uint32_t header_size;
	uint64_t total_bytes;	/* +08 : number of original data size              */
	uint32_t block_size;		/* +10 : number of compressed block size           */
	uint8_t ver;				/* +14 : version 01                                */
	uint8_t align;			/* +15 : align of index (offset = index[n]<<align) */
	uint8_t rsv_06[2];		/* +16 : reserved                                  */
} __attribute__ ((packed)) CISOHeader;


class CSO : public ISO
{

private:
	CISOHeader mHead;
	unsigned mTotalBlock;
	
	bool initDecompress();
	virtual int readSector( char *destBuf, unsigned sector );
	
	
public:
	CSO( std::string csoPath );
	~CSO();
	
	static bool isCSO ( std::string filePath );
};


#endif

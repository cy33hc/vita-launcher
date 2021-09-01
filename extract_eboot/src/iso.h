
#ifndef _ISO_H_
#define _ISO_H_

#include <string>
#include <vector>
#include <fstream>
#include "iso9660.h"

class ISO
{

private:
	

protected:
	std::ifstream mFin;
	std::vector<PathTableRecord*> mPathTable;
	
	virtual bool open( std::string path );
	virtual int readSector( char *destBuf, unsigned sector );
	void* read( uint32_t sector, uint32_t len );
	void write( uint32_t sector, uint32_t len, std::string file_path);
	virtual void close();
	
	void processPathTable( PathTableRecord* pathTable, uint32_t pathTableSize );
	
	uint16_t findDirPathTable( std::string dirPath, uint16_t parent = 1 );
	DirectoryRecord* findFile( std::string fileName, DirectoryRecord* dir );
	
	std::vector<DirectoryRecord*>* getDir( DirectoryRecord* dir );
	void* getFile( DirectoryRecord* fileRecord );
	
	static uint32_t lba2Pos( uint32_t lba );
	
public:
	static const uint32_t SECTOR_SIZE;

	ISO( std::string isoPath );
	virtual ~ISO();
	
	void Extract(std::string sfo_path ,std::string icon_path);

	static bool isISO ( std::string filePath );
};


#endif

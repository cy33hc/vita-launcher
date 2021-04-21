#include <psp2/kernel/iofilemgr.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <cstring>
#include <ios>
#include "iso.h"

const uint32_t ISO::SECTOR_SIZE = 0x800;

ISO::ISO( std::string isoPath )
{
	this->open( isoPath );
}

ISO::~ISO()
{
}

bool ISO::isISO ( std::string filePath )
{
	// Result (Not ISO)
	bool result = false;
	
	// Open File
	SceUID fd = sceIoOpen(filePath.c_str(), SCE_O_RDONLY, 0777);
	
	// Opened File
	if(fd >= 0)
	{
		// Move to ISO Header
		sceIoLseek32(fd, 0x8000, SCE_SEEK_SET);
		
		// Header Buffer
		unsigned char header[8];
		
		// Read Header
		if(sizeof(header) == sceIoRead(fd, header, sizeof(header)))
		{
			// ISO Header Magic
			unsigned char isoFlags[8] = {
				0x01, 0x43, 0x44, 0x30, 0x30, 0x31, 0x01, 0x00
			};
			
			// Valid Magic
			if( !memcmp(header, isoFlags, sizeof(header)) )
			{
				// ISO File
				result = true;
			}
		}
		
		// Close File
		sceIoClose(fd);
	}
	
	// Return Result
	return result;
}

bool ISO::open( std::string path )
{
	mFin.open(path.c_str(),std::ios::binary);
	
	return mFin.is_open();
}

int ISO::readSector( char *destBuf, unsigned sector )
{
	mFin.seekg(lba2Pos(sector), std::ios::beg);
	mFin.read(destBuf, ISO::SECTOR_SIZE);
	
	return ISO::SECTOR_SIZE;
}

void ISO::close()
{
	mFin.close();
}

void* ISO::read( uint32_t sector, uint32_t len )
{
	uint32_t bufSize = ((len / ISO::SECTOR_SIZE) + 1)*ISO::SECTOR_SIZE;
	void* data = malloc(bufSize);
	uint32_t sizeRead = 0;
	uint32_t curSector = sector;
	
	while ( sizeRead < len )
	{
		int ret = this->readSector((char*)((uint32_t)data+sizeRead), curSector );
		
		if ( ret < 0 )	break;
		else
		{
			sizeRead+= ret;
			++curSector;
		}
	}
	
	return data;
}

void ISO::write( uint32_t sector, uint32_t len, std::string file_path)
{
	uint32_t bufSize = ((len / ISO::SECTOR_SIZE) + 1)*ISO::SECTOR_SIZE;
	void* data = malloc(bufSize);
	uint32_t sizeRead = 0;
	uint32_t curSector = sector;
	
	while ( sizeRead < len )
	{
		int ret = this->readSector((char*)((uint32_t)data+sizeRead), curSector );
		
		if ( ret < 0 )	break;
		else
		{
			sizeRead+= ret;
			++curSector;
		}
	}
	
	SceUID fd = sceIoOpen(file_path.c_str(), SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	int write = sceIoWrite(fd, data, bufSize);
	sceIoClose(fd);

	free(data);
}

void ISO::processPathTable( PathTableRecord* pathTable, uint32_t pathTableSize )
{
	PathTableRecord* curRecord = pathTable;
	uint32_t recordSize;
	
	while ( (uint32_t)curRecord < (uint32_t)pathTable+pathTableSize )
	{
		mPathTable.push_back( curRecord );
		
		recordSize = sizeof(PathTableRecord) + curRecord->nameSize;
		if ( recordSize%2 )	++recordSize;
		
		curRecord = (PathTableRecord*) ((uint32_t)curRecord + recordSize);
	}
}


uint16_t ISO::findDirPathTable( std::string dirPath, uint16_t parent )
{
	uint32_t curIdx = -1;
	bool found = false;
	
	// If path contains at least one directory, search
	if ( dirPath.find('/') != std::string::npos )
	{
		while ( !found &&  ++curIdx < mPathTable.size() )
		{
			if ( mPathTable[curIdx]->parentIdx == parent )
				if ( !strncmp(mPathTable[curIdx]->name, dirPath.c_str(), mPathTable[curIdx]->nameSize) )
					found = true;
		}
	}
	
	if (found)
	{
		return this->findDirPathTable( dirPath.substr( dirPath.find('/')+1), curIdx+1 );
	}
	else	return parent;
}

std::vector<DirectoryRecord*>* ISO::getDir( DirectoryRecord* dir )
{
	DirectoryRecord* curRecord = dir;
	std::vector<DirectoryRecord*>* dirList = new std::vector<DirectoryRecord*>();
	
	while ( curRecord->size != 0 )
	{
		dirList->push_back( curRecord );
		
		curRecord = (DirectoryRecord*) ((uint32_t)curRecord + curRecord->size);
	}
	
	return dirList;
}

DirectoryRecord* ISO::findFile( std::string fileName, DirectoryRecord* dir )
{
	uint32_t curIdx = -1;
	bool found = false;
	std::vector<DirectoryRecord*>* dirList = this->getDir(dir);
	DirectoryRecord* ret = NULL;
	
	while ( !found &&  ++curIdx < dirList->size() )
	{
		if ( !strncmp((*dirList)[curIdx]->name, fileName.c_str(), (*dirList)[curIdx]->nameSize) )	found = true;
	}
	
	if ( found )	ret = (*dirList)[curIdx];
	
	delete dirList;
	
	return ret;
}

void* ISO::getFile( DirectoryRecord* fileRecord )
{
	return this->read( fileRecord->lba.LE, fileRecord->fileSize.LE );
}

void ISO::Extract(std::string sfo_path ,std::string icon_path)
{
	if ( mFin.is_open() )
	{
		char *sectorBuf = (char*)malloc( ISO::SECTOR_SIZE );

		int err;
		err = this->readSector( sectorBuf, 16);
		if ( err >= 0 )
		{
			PrimaryVolumeDescriptor* pvd = (PrimaryVolumeDescriptor*)sectorBuf;
			uint32_t lbaPathTableL = pvd->lbaPathTableL;
			uint32_t pathTableSize = pvd->pathTableSize.LE;
			
			void *pathTableBuf = this->read(lbaPathTableL, pathTableSize);
			this->processPathTable( (PathTableRecord*)pathTableBuf, pathTableSize );

			uint16_t dirId;
			if ( (dirId = this->findDirPathTable( "PSP_GAME/" )) > 1 )
			{
				this->readSector( sectorBuf, mPathTable[dirId-1]->lba);
				
				DirectoryRecord* dir = (DirectoryRecord*)sectorBuf;
				DirectoryRecord* icon0 = findFile( "ICON0.PNG", dir );
				DirectoryRecord* sfo = findFile( "PARAM.SFO", dir );

				this->write(icon0->lba.LE, icon0->fileSize.LE, icon_path);
				this->write(sfo->lba.LE, sfo->fileSize.LE, sfo_path);
			}
			
			free(pathTableBuf);
			mPathTable.clear();
		}
		
		free(sectorBuf);
		
		this->close();
	}
}

inline uint32_t ISO::lba2Pos( uint32_t lba )
{
	return lba*ISO::SECTOR_SIZE;
}

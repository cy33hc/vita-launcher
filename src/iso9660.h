
#ifndef _ISO9660_H_
#define _ISO9660_H_

typedef struct __attribute__((packed)) lbe32_ {
	uint32_t LE;
	uint32_t BE;
} lbe32;

typedef struct __attribute__((packed)) lbe16_ {
	uint16_t LE;
	uint16_t BE;
} lbe16;

typedef struct  __attribute__((packed)) PrimVolDateTime_ {
	char year[4];
	char month[2];
	char day[2];
	char hour[2];
	char minute[2];
	char second[2];
	char centiSeconds[2];
	uint8_t gmtOffset;
} PrimVolDateTime;

typedef struct  __attribute__((packed)) PrimaryVolumeDescriptor_ {
	uint8_t typeCode;	// Always 1 for a Primary Volume Descriptor
	char id[5];	// Always CD001
	uint8_t version;	// Always 1
	uint8_t unused0;	// Always 0
	char systmId[0x20];
	char volId[0x20];
	uint8_t unused1[8];
	lbe32 isoSize;	// Unit = sectorSize
	uint8_t unused2[0x20];
	lbe16 volSetSize;
	lbe16 volSeqNb;
	lbe16 sectorSize;
	lbe32 pathTableSize;
	uint32_t lbaPathTableL;
	uint32_t lbaPathTableBackupL;
	uint32_t lbaPathTableM;
	uint32_t lbaPathTableBackupM;
	uint8_t rootDirRecord[0x22];	// DirectoryRecord with 0 sized name, so always 34 bytes
	char volSetId[0x80];
	char publisherId[0x80];
	char dataPreparerId[0x80];
	char appId[0x80];
	char copyrightFileId[0x26];
	char abstractFileId[0x24];
	char bibliographicFileId[0x25];
	PrimVolDateTime volCreatDate;
	PrimVolDateTime volModDate;
	PrimVolDateTime volExpDate;
	PrimVolDateTime volEffDate;	// Volume Effective Date and Time : Date and time from which the volume should be used
	uint8_t structVersion;	// Always 1 : directory records and path table version
	uint8_t unused3;	// Always 0
	uint8_t appUsed[0x200];	// Not defined by ISO 9660
	uint8_t reserved[0x28D];	// Reserved by ISO
} PrimaryVolumeDescriptor;


typedef struct  __attribute__((packed)) PathTableRecord_ {
	uint8_t nameSize;
	uint8_t xarSize;
	uint32_t lba;
	uint16_t parentIdx;
	char name[];	// If nameSize odd, followed by pad byte
} PathTableRecord;


typedef struct  __attribute__((packed)) DirRecDateTime_ {
	uint8_t	year;
	uint8_t	month;
	uint8_t	day;
	uint8_t	hour;
	uint8_t	minute;
	uint8_t	second;
	uint8_t	gmtOffset;
} DirRecDateTime;


enum ISO9660FileFlags {
	HIDDEN = 0, DIRECTORY = 1, ASSOCIATED = 2, XAR_FORMAT = 3, XAR_PERM = 4, RESERVED_0 = 5, RESERVED_1 = 6, NOT_FINAL_DIR = 7
};


typedef struct __attribute__((packed))
{
	uint8_t size;
	uint8_t xarSize;		//	Extended Attribute Record length
	lbe32 lba;
	lbe32 fileSize;
	DirRecDateTime time;
	uint8_t fileFlags;	// One of enum ISO9660FileFlags
	uint8_t interleaveUnit;
	uint8_t interleaveSkip;
	lbe16 volSeqNb;
	uint8_t nameSize;
	char name[];	// If nameSize odd, followed by pad byte
} DirectoryRecord;

#endif


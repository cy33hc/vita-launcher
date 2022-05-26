#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <psp2/net/net.h>
#include <time.h>
#include <string>
#include <vector>
#include <regex>

#define FTP_CLIENT_MAX_FILENAME_LEN 128

typedef int (*FtpCallbackXfer)(int64_t xfered, void *arg);

struct ftphandle {
	char *cput,*cget;
	int handle;
	int cavail,cleft;
	char *buf;
	int dir;
	ftphandle *ctrl;
	int cmode;
	int64_t xfered;
	int64_t xfered1;
	int64_t cbbytes;
	char response[512];
	int64_t offset;
	bool correctpasv;
	FtpCallbackXfer xfercb;
	void *cbarg;
};

/**
  * @brief Date and time representation
**/
  
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t dayOfWeek;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint16_t milliseconds;
} DateTime;

/**
  * @brief Directory entry
  **/
typedef struct
{
    char name[FTP_CLIENT_MAX_FILENAME_LEN + 1];
    bool isDir;
    uint32_t size;
    DateTime modified;
} FtpDirEntry;

class FtpClient {
public:
	enum accesstype
	{
		dir = 1,
		dirverbose,
		fileread,
		filewrite,
		filereadappend,
		filewriteappend
	};

	enum transfermode
	{
		ascii = 'A',
		image = 'I'
	};

	enum connmode
	{
		pasv = 1,
		port
	};
	
	enum attributes
	{
       directory = 1,
       readonly = 2
	};

    FtpClient();
    ~FtpClient();
    int Connect(const char *host, unsigned short port);
	void SetConnmode(connmode mode);
	int FtpSendCmd(const char *cmd, char expected_resp, ftphandle *nControl);
	int Login(const char *user, const char *pass);
	int Site(const char *cmd);
	int Raw(const char *cmd);
	int SysType(char *buf, int max);
	int Mkdir(const char *path);
	int Chdir(const char *path);
	int Cdup();
	int Rmdir(const char *path);
	int Pwd(char *path, int max);
	int Size(const char *path, int64_t *size, transfermode mode);
	int ModDate(const char *path, char *dt, int max);
	int Get(const char *outputfile, const char *path, transfermode mode, int64_t offset = 0);
	int Put(const char *inputfile, const char *path, transfermode mode, int64_t offset = 0);
	int Rename(const char *src, const char *dst);
	int Delete(const char *path);
	ftphandle* RawOpen(const char *path, accesstype type, transfermode mode);
	int RawClose(ftphandle* handle);
	int RawWrite(void* buf, int len, ftphandle* handle);
	int RawRead(void* buf, int max, ftphandle* handle);
	std::vector<std::string> ListFiles(const char *path, std::regex *regexpr, bool includeSubDir=false);
	std::vector<FtpDirEntry> ListDir(const char *path);
	void SetCallbackXferFunction(FtpCallbackXfer pointer);
	void SetCallbackArg(void *arg);
	void SetCallbackBytes(int64_t bytes);
	char* LastResponse();
	int Quit();

private:
	ftphandle* mp_ftphandle;

	int ReadResponse(char c, ftphandle *nControl);
	int Readline(char *buf, int max, ftphandle *nControl);
	int Writeline(char *buf, int len, ftphandle *nData);
	void ClearHandle();
	int FtpOpenPasv(ftphandle *nControl, ftphandle **nData, transfermode mode, int dir, char *cmd);
	int FtpOpenPort(ftphandle *nControl, ftphandle **nData, transfermode mode, int dir, char *cmd);
	int FtpAcceptConnection(ftphandle *nData, ftphandle *nControl);
	int CorrectPasvResponse(int *v);
	int FtpAccess(const char *path, accesstype type, transfermode mode, ftphandle *nControl, ftphandle **nData);
	int FtpXfer(const char *localfile, const char *path, ftphandle *nControl, accesstype type, transfermode mode);
	int FtpWrite(void *buf, int len, ftphandle *nData);
	int FtpRead(void *buf, int max, ftphandle *nData);
	int FtpClose(ftphandle *nData);
	int ParseDirEntry(char *line, FtpDirEntry *dirEntry);
};

#endif
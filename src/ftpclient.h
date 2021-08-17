#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <psp2/net/net.h>

#define FTPLIB_BUFSIZ 1024
#define ACCEPT_TIMEOUT 30

/* io types */
#define FTPLIB_CONTROL 0
#define FTPLIB_READ 1
#define FTPLIB_WRITE 2

struct ftphandle {
	char *cput,*cget;
	int handle;
	int cavail,cleft;
	char *buf;
	int dir;
	ftphandle *ctrl;
	int cmode;
	int64_t xfered;
	int64_t cbbytes;
	int64_t xfered1;
	char response[FTPLIB_BUFSIZ];
	int64_t offset;
	struct timeval idletime;
	bool correctpasv;
};

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
	
    FtpClient();
    ~FtpClient();
    int Connect(const char *host, unsigned short port);
	void SetConnmode(connmode mode);
	int FtpSendCmd(const char *cmd, char expected_resp, ftphandle *nControl);
	int Login(const char *user, const char *pass);
	int Nlst(const char *outputfile, const char *path);
	int Dir(const char *outputfile, const char *path);
	ftphandle* RawOpen(const char *path, accesstype type, transfermode mode);
	int RawClose(ftphandle* handle);
	int RawWrite(void* buf, int len, ftphandle* handle);
	int RawRead(void* buf, int max, ftphandle* handle);
	int Quit();

private:
	ftphandle* mp_ftphandle;

	int ReadResponse(char c, ftphandle *nControl);
	int Readline(char *buf, int max, ftphandle *nControl);
	int Writeline(char *buf, int len, ftphandle *nData);
	char* LastResponse();
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

};

#endif
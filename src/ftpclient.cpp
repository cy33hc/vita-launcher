#include <psp2/net/net.h>
#include <cstring>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ftpclient.h>
#include <errno.h>

#define FTP_CLIENT_BUFSIZ 1048576
#define ACCEPT_TIMEOUT 30

/* io types */
#define FTP_CLIENT_CONTROL 0
#define FTP_CLIENT_READ 1
#define FTP_CLIENT_WRITE 2

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

FtpClient::FtpClient()
{
    mp_ftphandle = static_cast<ftphandle *>(calloc(1,sizeof(ftphandle)));
	if (mp_ftphandle == NULL) perror("calloc");
	mp_ftphandle->buf = static_cast<char *>(malloc(FTP_CLIENT_BUFSIZ));
	if (mp_ftphandle->buf == NULL)
	{
		perror("calloc");
		free(mp_ftphandle);
	}
    ClearHandle();
}

FtpClient::~FtpClient()
{
    free(mp_ftphandle->buf);
    free(mp_ftphandle);
}

int FtpClient::Connect(const char *host, unsigned short port)
{
    int sControl;
    SceNetInAddr dst_addr; /* destination address */
    SceNetSockaddrIn server_addr;
    int on = 1; /* used in Setsockopt function */
    int32_t retval; /* return value */

	mp_ftphandle->dir = FTP_CLIENT_CONTROL;
	mp_ftphandle->ctrl = NULL;
	mp_ftphandle->xfered = 0;
	mp_ftphandle->xfered1 = 0;
	mp_ftphandle->offset = 0;
	mp_ftphandle->handle = 0;
	memset(&mp_ftphandle->response, 0, sizeof(mp_ftphandle->response));

    memset(&server_addr, 0, sizeof(server_addr));
    sceNetInetPton(SCE_NET_AF_INET, host, (void*)&dst_addr);
    server_addr.sin_addr = dst_addr;
    server_addr.sin_port = sceNetHtons(port);
    server_addr.sin_family = SCE_NET_AF_INET;

    sControl = sceNetSocket("ftp_control", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, SCE_NET_IPPROTO_TCP);
    if (mp_ftphandle->handle < 0)
    {
        return 0;
    }

    retval = sceNetSetsockopt(sControl, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, (const void*)&on, sizeof(on));
    if (retval == -1)
    {
        return 0;
    }

    retval = sceNetConnect(sControl, (SceNetSockaddr *)&server_addr, sizeof(server_addr));
    if (retval == -1)
    {
		sprintf(mp_ftphandle->response, "Connection timeout\n");
        sceNetSocketClose(sControl);
        return 0;
    }
    mp_ftphandle->handle = sControl;

    if (ReadResponse('2', mp_ftphandle) == 0)
    {
        sceNetSocketClose(mp_ftphandle->handle);
        mp_ftphandle->handle = 0;
        return 0;
    }

    return 1;
}

/*
 * FtpSendCmd - send a command and wait for expected response
 *
 * return 1 if proper response received, 0 otherwise
 */
int FtpClient::FtpSendCmd(const char *cmd, char expected_resp, ftphandle *nControl)
{
	char buf[512];
	int x;

	if (!nControl->handle) return 0;
    if (nControl->dir != FTP_CLIENT_CONTROL) return 0;

	sprintf(buf, "%s\r\n", cmd);
    x = sceNetSend(nControl->handle, buf, strlen(buf), 0);
	if (x <= 0)
	{
		return 0;
	}
	
	return ReadResponse(expected_resp, nControl);
}

/*
 * read a response from the server
 *
 * return 0 if first char doesn't match
 * return 1 if first char matches
 */
int FtpClient::ReadResponse(char c, ftphandle *nControl)
{
	char match[5];
	
	if (Readline(nControl->response, 512, nControl) == -1)
	{
		return 0;
	}
	
	if (nControl->response[3] == '-')
	{
		strncpy(match, nControl->response, 3);
		match[3] = ' ';
		match[4] = '\0';
		do
		{
			if (Readline(nControl->response, 512, nControl) == -1)
			{
				return 0;
			}
		} while (strncmp(nControl->response, match, 4));
	}

	if (nControl->response[0] == c) return 1;
	return 0;
}

/*
 * read a line of text
 *
 * return -1 on error or bytecount
 */
int FtpClient::Readline(char *buf, int max, ftphandle *nControl)
{
	int x, retval = 0;
	char *end, *bp = buf;
	int eof = 0;

	if (max == 0) return 0;

	do
	{
		if (nControl->cavail > 0)
		{
			x = (max >= nControl->cavail) ? nControl->cavail : max-1;
			end = static_cast<char*>(memccpy(bp, nControl->cget, '\n', x));
			if (end != NULL) x = end - bp;
			retval += x;
			bp += x;
			*bp = '\0';
			max -= x;
			nControl->cget += x;
			nControl->cavail -= x;
			if (end != NULL)
			{
				bp -= 2;
				if (strcmp(bp, "\r\n") == 0)
				{
					*bp++ = '\n';
					*bp++ = '\0';
					--retval;
				}
				break;
			}
		}

		if (max == 1)
		{
			*buf = '\0';
			break;
		}

		if (nControl->cput == nControl->cget)
		{
			nControl->cput = nControl->cget = nControl->buf;
			nControl->cavail = 0;
			nControl->cleft = FTP_CLIENT_BUFSIZ;
		}

		if (eof)
		{
			if (retval == 0) retval = -1;
			break;
		}

		x = sceNetRecv(nControl->handle, nControl->cput, nControl->cleft, 0);		

		if ( x == -1)
		{
			retval = -1;
			break;
		}

		if (x == 0) eof = 1;
		nControl->cleft -= x;
		nControl->cavail += x;
		nControl->cput += x;
	} while (1);
	return retval;
}

/*
 * FtpLogin - log in to remote server
 *
 * return 1 if logged in, 0 otherwise
 */
int FtpClient::Login(const char *user, const char *pass)
{
	char tempbuf[128];

	if (((strlen(user) + 7) > sizeof(tempbuf)) || ((strlen(pass) + 7) > sizeof(tempbuf))) return 0;
	sprintf(tempbuf, "USER %s", user);
	if (!FtpSendCmd(tempbuf, '3', mp_ftphandle))
	{
		if (mp_ftphandle->ctrl != NULL) return 1;
		if (*LastResponse() == '2') return 1;
		return 0;
	}
	sprintf(tempbuf,"PASS %s",pass);
	return FtpSendCmd(tempbuf, '2', mp_ftphandle);
}

/*
 * FtpLastResponse - return a pointer to the last response received
 */
char* FtpClient::LastResponse()
{
	if ((mp_ftphandle) && (mp_ftphandle->dir == FTP_CLIENT_CONTROL)) return mp_ftphandle->response;
	return NULL;
}

void FtpClient::ClearHandle()
{
	mp_ftphandle->dir = FTP_CLIENT_CONTROL;
	mp_ftphandle->ctrl = NULL;
	mp_ftphandle->cmode = FtpClient::pasv;
	mp_ftphandle->cbarg = NULL;
	mp_ftphandle->cbbytes = 0;
	mp_ftphandle->xfered = 0;
	mp_ftphandle->xfered1 = 0;
	mp_ftphandle->offset = 0;
	mp_ftphandle->handle = 0;
	mp_ftphandle->xfercb = NULL;
	mp_ftphandle->correctpasv = false;
	memset(&mp_ftphandle->response, 0, sizeof(mp_ftphandle->response));
}

void FtpClient::SetConnmode(connmode mode)
{
	mp_ftphandle->cmode = mode;
}

/*
 * FtpAccess - return a handle for a data stream
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::FtpAccess(const char *path, accesstype type, transfermode mode, ftphandle *nControl, ftphandle **nData)
{
	char buf[512];
	int dir;

	if ((path == NULL) && ((type == FtpClient::filewrite)
		|| (type == FtpClient::fileread)
		|| (type == FtpClient::filereadappend)
		|| (type == FtpClient::filewriteappend)))
	{
		sprintf(nControl->response,"Missing path argument for file transfer\n");
		return 0;
	}
	sprintf(buf, "TYPE %c", mode);
	if (!FtpSendCmd(buf, '2', nControl)) return 0;

	switch (type)
	{
	case FtpClient::dir:
		strcpy(buf,"NLST");
		dir = FTP_CLIENT_READ;
		break;
	case FtpClient::dirverbose:
		strcpy(buf,"LIST");
		dir = FTP_CLIENT_READ;
		break;
	case FtpClient::filereadappend:
	case FtpClient::fileread:
		strcpy(buf,"RETR");
		dir = FTP_CLIENT_READ;
		break;
    case FtpClient::filewriteappend:
	case FtpClient::filewrite:
		strcpy(buf,"STOR");
		dir = FTP_CLIENT_WRITE;
		break;
	default:
		sprintf(nControl->response, "Invalid open type %d\n", type);
		return 0;
	}
	if (path != NULL)
	{
		int i = strlen(buf);
		buf[i++] = ' ';
		if ((strlen(path) + i) >= sizeof(buf)) return 0;
		strcpy(&buf[i],path);
	}

	if (nControl->cmode == FtpClient::pasv)
	{
		if (FtpOpenPasv(nControl, nData, mode, dir, buf) == -1) return 0;
	}

    if (nControl->cmode == FtpClient::port)
	{
		if (FtpOpenPort(nControl, nData, mode, dir, buf) == -1) return 0;
		if (!FtpAcceptConnection(*nData,nControl))
		{
			FtpClose(*nData);
			*nData = NULL;
			return 0;
		}
	}

	return 1;
}

/*
 * FtpAcceptConnection - accept connection from server
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::FtpAcceptConnection(ftphandle *nData, ftphandle *nControl)
{
	int sData;
	SceNetSockaddr addr;
	uint32_t l;
	int i;
	struct timeval tv;
	fd_set mask;
	int rv = 0;

	FD_ZERO(&mask);
	FD_SET(nControl->handle, &mask);
	FD_SET(nData->handle, &mask);
	tv.tv_usec = 0;
	tv.tv_sec = ACCEPT_TIMEOUT;
	i = nControl->handle;
	if (i < nData->handle) i = nData->handle;

    if (FD_ISSET(nData->handle, &mask))
    {
        l = sizeof(addr);
        sData = sceNetAccept(nData->handle, &addr, &l);
        i = errno;
        sceNetSocketClose(nData->handle);
        if (sData > 0)
        {
            rv = 1;
            nData->handle = sData;
            nData->ctrl = nControl;
        }
        else
        {
            strncpy(nControl->response, strerror(i), sizeof(nControl->response));
            nData->handle = 0;
            rv = 0;
        }
    }
    else if (FD_ISSET(nControl->handle, &mask))
    {
        sceNetSocketClose(nData->handle);
        nData->handle = 0;
        ReadResponse('2', nControl);
        rv = 0;
    }

	return rv;
}

/*
 * FtpOpenPasv - Establishes a PASV connection for data transfer
 *
 * return 1 if successful, -1 otherwise
 */
int FtpClient::FtpOpenPasv(ftphandle *nControl, ftphandle **nData, transfermode mode, int dir, char *cmd)
{
	int sData;
	union {
	  SceNetSockaddr sa;
	  SceNetSockaddrIn in;
	} sin;
    SceNetLinger lng = {0, 0};
    unsigned int l;
	int on=1;
	ftphandle *ctrl;
	char *cp;
	int v[6];
	int ret;

	if (nControl->dir != FTP_CLIENT_CONTROL) return -1;
	if ((dir != FTP_CLIENT_READ) && (dir != FTP_CLIENT_WRITE))
	{
		sprintf(nControl->response, "Invalid direction %d\n", dir);
		return -1;
	}
	if ((mode != FtpClient::ascii) && (mode != FtpClient::image))
	{
		sprintf(nControl->response, "Invalid mode %c\n", mode);
		return -1;
	}
    l = sizeof(sin);

	memset(&sin, 0, l);
	sin.in.sin_family = SCE_NET_AF_INET;
	if (!FtpSendCmd("PASV", '2' , nControl)) return -1;
	cp = strchr(nControl->response,'(');
	if (cp == NULL) return -1;
	cp++;
	sscanf(cp, "%u,%u,%u,%u,%u,%u", &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]);
	if (nControl->correctpasv) if (!CorrectPasvResponse(v)) return -1;
	sin.sa.sa_data[2] = v[2];
	sin.sa.sa_data[3] = v[3];
	sin.sa.sa_data[4] = v[4];
	sin.sa.sa_data[5] = v[5];
	sin.sa.sa_data[0] = v[0];
	sin.sa.sa_data[1] = v[1];

	if (mp_ftphandle->offset != 0)
	{
		char buf[512];
        sprintf(buf, "REST %lld", mp_ftphandle->offset);
		if (!FtpSendCmd(buf,'3',nControl)) return 0;
	}

	sData = sceNetSocket("ftp_data", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, SCE_NET_IPPROTO_TCP);
	if (sData == -1)
	{
		return -1;
	}

	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, (const void*)&on, sizeof(on)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}

	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_LINGER, &lng, sizeof(lng)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}

	int const size = FTP_CLIENT_BUFSIZ;
	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVBUF, &size, sizeof (size)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}

	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_SNDBUF, &size, sizeof (size)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}

	if (nControl->dir != FTP_CLIENT_CONTROL) return -1;
	memcpy(cmd + strlen(cmd), "\r\n\0", 3);
	ret = sceNetSend(nControl->handle, cmd, strlen(cmd), 0);
	if (ret <= 0)
	{
		return -1;
	}

	if (sceNetConnect(sData, &sin.sa, sizeof(sin.sa)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}
    
	if (!ReadResponse('1', nControl))
	{
		sceNetSocketClose(sData);
		return -1;
	}
	ctrl = static_cast<ftphandle*>(calloc(1,sizeof(ftphandle)));
	if (ctrl == NULL)
	{
		sceNetSocketClose(sData);
		return -1;
	}
	if ((mode == 'A') && ((ctrl->buf = static_cast<char*>(malloc(FTP_CLIENT_BUFSIZ))) == NULL))
	{
		sceNetSocketClose(sData);
		free(ctrl);
		return -1;
	}
	ctrl->handle = sData;
	ctrl->dir = dir;
	ctrl->ctrl = (nControl->cmode == FtpClient::pasv) ? nControl : NULL;
	ctrl->xfered = 0;
	ctrl->xfered1 = 0;
	ctrl->cbarg = nControl->cbarg;
	ctrl->cbbytes = nControl->cbbytes;
	if (ctrl->cbbytes)
	{
		ctrl->xfercb = nControl->xfercb;
	}
	else
	{
		ctrl->xfercb = NULL;
	}
	*nData = ctrl;

	return 1;
}

/*
 * FtpOpenPort - Establishes a PORT connection for data transfer
 *
 * return 1 if successful, -1 otherwise
 */
int FtpClient::FtpOpenPort(ftphandle *nControl, ftphandle **nData, transfermode mode, int dir, char *cmd)
{
	int sData;
	union {
	  SceNetSockaddr sa;
	  SceNetSockaddrIn in;
	} sin;
	SceNetLinger lng = { 0, 0 };
	uint32_t l;
	int on=1;
	ftphandle *ctrl;
	char buf[512];

	if (nControl->dir != FTP_CLIENT_CONTROL) return -1;
	if ((dir != FTP_CLIENT_READ) && (dir != FTP_CLIENT_WRITE))
	{
		sprintf(nControl->response, "Invalid direction %d\n", dir);
		return -1;
	}
	if ((mode != FtpClient::ascii) && (mode != FtpClient::image))
	{
		sprintf(nControl->response, "Invalid mode %c\n", mode);
		return -1;
	}
	l = sizeof(sin.sa);

	if (sceNetGetsockname(nControl->handle, &sin.sa, &l) < 0)
	{
		return -1;
	}

	sData = sceNetSocket("ftp_data", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, SCE_NET_IPPROTO_TCP);
	if (sData == -1)
	{
		return -1;
	}
	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, (const void*)&on, sizeof(on)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}

	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_LINGER, &lng, sizeof(lng)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}

	int const size = FTP_CLIENT_BUFSIZ;
	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVBUF, &size, sizeof (size)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}

	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_SNDBUF, &size, sizeof (size)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}

	sin.in.sin_port = 0;
	if (sceNetBind(sData, &sin.sa, sizeof(sin)) == -1)
	{
		sceNetSocketClose(sData);
		return -1;
	}
	if (sceNetListen(sData, 1) < 0)
	{
		sceNetSocketClose(sData);
		return -1;
	}
	if (sceNetGetsockname(sData, &sin.sa, &l) < 0) return 0;
	sprintf(buf, "PORT %hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
		(unsigned char) sin.sa.sa_data[2],
		(unsigned char) sin.sa.sa_data[3],
		(unsigned char) sin.sa.sa_data[4],
		(unsigned char) sin.sa.sa_data[5],
		(unsigned char) sin.sa.sa_data[0],
		(unsigned char) sin.sa.sa_data[1]);
	if (!FtpSendCmd(buf, '2', nControl))
	{
		sceNetSocketClose(sData);
		return -1;
	}

	if (mp_ftphandle->offset != 0)
	{
	char buf[512];
    sprintf(buf, "REST %lld", mp_ftphandle->offset);
	if (!FtpSendCmd(buf,'3',nControl))
	{
		sceNetSocketClose(sData);
		return 0;
	}
	}

	ctrl = static_cast<ftphandle*>(calloc(1,sizeof(ftphandle)));
	if (ctrl == NULL)
	{
		sceNetSocketClose(sData);
		return -1;
	}
	if ((mode == 'A') && ((ctrl->buf = static_cast<char*>(malloc(FTP_CLIENT_BUFSIZ))) == NULL))
	{
		sceNetSocketClose(sData);
		free(ctrl);
		return -1;
	}

	if (!FtpSendCmd(cmd, '1', nControl))
	{
		FtpClose(*nData);
		*nData = NULL;
		return -1;
	}

	ctrl->handle = sData;
	ctrl->dir = dir;
	ctrl->ctrl = (nControl->cmode == FtpClient::pasv) ? nControl : NULL;
	ctrl->xfered = 0;
	ctrl->xfered1 = 0;
	ctrl->cbarg = nControl->cbarg;
	ctrl->cbbytes = nControl->cbbytes;
	if (ctrl->cbbytes)
	{
		ctrl->xfercb = nControl->xfercb;
	}
	else
	{
		ctrl->xfercb = NULL;
	}
	*nData = ctrl;

	return 1;
}

int FtpClient::CorrectPasvResponse(int *v)
{
	SceNetSockaddr ipholder;
	uint32_t ipholder_size = sizeof(ipholder);

	if (sceNetGetpeername(mp_ftphandle->handle, &ipholder, &ipholder_size) == -1)
	{
		sceNetSocketClose(mp_ftphandle->handle);
		return 0;
	}
	
	for (int i = 2; i < 6; i++) v[i] = ipholder.sa_data[i];
	
	return 1;
}

/*
 * FtpXfer - issue a command and transfer data
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::FtpXfer(const char *localfile, const char *path, ftphandle *nControl, accesstype type, transfermode mode)
{
	int l,c;
	char *dbuf;
	FILE *local = NULL;
	ftphandle *nData;

	if (localfile != NULL)
	{
		char ac[3] = "  ";
		if ((type == FtpClient::dir) || (type == FtpClient::dirverbose)) { ac[0] = 'w'; ac[1] = '\0'; }
		if (type == FtpClient::fileread) { ac[0] = 'w'; ac[1] = '\0'; }
		if (type == FtpClient::filewriteappend) { ac[0] = 'r'; ac[1] = '\0'; }
		if (type == FtpClient::filereadappend) { ac[0] = 'a'; ac[1] = '\0'; }
		if (type == FtpClient::filewrite) { ac[0] = 'r'; ac[1] = '\0'; }
		if (mode == FtpClient::image) ac[1] = 'b';

		local = fopen(localfile, ac);
		if (local == NULL)
		{
			strncpy(nControl->response, strerror(errno), sizeof(nControl->response));
			return 0;
		}
		if (type == FtpClient::filewriteappend) fseek(local,mp_ftphandle->offset,SEEK_SET);
	}
	if (local == NULL) local = ((type == FtpClient::filewrite)
		|| (type == FtpClient::filewriteappend)) ? stdin : stdout;
	if (!FtpAccess(path, type, mode, nControl, &nData)) {
        if (localfile != NULL) fclose(local);
        return 0;
    }

	dbuf = static_cast<char*>(malloc(FTP_CLIENT_BUFSIZ));
	if ((type == FtpClient::filewrite) || (type == FtpClient::filewriteappend))
	{
		while ((l = fread(dbuf, 1, FTP_CLIENT_BUFSIZ, local)) > 0)
		{
			if ((c = FtpWrite(dbuf, l, nData)) < l)
			{
				break;
			}
		}
	}
	else
	{
		while ((l = FtpRead(dbuf, FTP_CLIENT_BUFSIZ, nData)) > 0)
		{
			if (fwrite(dbuf, 1, l, local) <= 0)
			{
				break;
			}
		}
	}
	free(dbuf);
	fflush(local);
	if (localfile != NULL) fclose(local);
	return FtpClose(nData);
}

/*
 * FtpWrite - write to a data connection
 */
int FtpClient::FtpWrite(void *buf, int len, ftphandle *nData)
{
	int i;

	if (nData->dir != FTP_CLIENT_WRITE) return 0;
	if (nData->buf) i = Writeline(static_cast<char*>(buf), len, nData);
	else
	{
		i = sceNetSend(nData->handle, buf, len, 0);
	}
	if (i == -1) return 0;
	nData->xfered += i;

	if (nData->xfercb && nData->cbbytes)
	{
		nData->xfered1 += i;
		if (nData->xfered1 >= nData->cbbytes)
		{
			if (nData->xfercb(nData->xfered, nData->cbarg) == 0) return 0;
			nData->xfered1 = 0;
		}
	}

	return i;
}

/*
 * FtpRead - read from a data connection
 */
int FtpClient::FtpRead(void *buf, int max, ftphandle *nData)
{
	int i;

	if (nData->dir != FTP_CLIENT_READ)
	return 0;
	if (nData->buf) i = Readline(static_cast<char*>(buf), max, nData);
	else
	{
		i = sceNetRecv(nData->handle, buf, max, 0);
	}
	if (i == -1) return 0;
	nData->xfered += i;
	if (nData->xfercb && nData->cbbytes)
	{
		nData->xfered1 += i;
		if (nData->xfered1 >= nData->cbbytes)
		{
			if (nData->xfercb(nData->xfered, nData->cbarg) == 0) return 0;
			nData->xfered1 = 0;
		}
	}
	return i;
}

/*
 * write lines of text
 *
 * return -1 on error or bytecount
 */
int FtpClient::Writeline(char *buf, int len, ftphandle *nData)
{
	int x, nb=0, w;
	char *ubp = buf, *nbp;
	char lc=0;

	if (nData->dir != FTP_CLIENT_WRITE)
	return -1;
	nbp = nData->buf;
	for (x=0; x < len; x++)
	{
		if ((*ubp == '\n') && (lc != '\r'))
		{
			if (nb == FTP_CLIENT_BUFSIZ)
			{
				w = sceNetSend(nData->handle, nbp, FTP_CLIENT_BUFSIZ, 0);
				if (w != FTP_CLIENT_BUFSIZ)
				{
					return(-1);
				}
				nb = 0;
			}
			nbp[nb++] = '\r';
		}
		if (nb == FTP_CLIENT_BUFSIZ)
		{
			w = sceNetSend(nData->handle, nbp, FTP_CLIENT_BUFSIZ, 0);
			if (w != FTP_CLIENT_BUFSIZ)
			{
				return(-1);
			}
			nb = 0;
		}
		nbp[nb++] = lc = *ubp++;
	}
	if (nb)
	{
		w = sceNetSend(nData->handle, nbp, nb, 0);	
		if (w != nb)
		{
			return(-1);
		}
	}
	return len;
}

/*
 * FtpClose - close a data connection
 */
int FtpClient::FtpClose(ftphandle *nData)
{
	ftphandle *ctrl;

	if (nData->dir == FTP_CLIENT_WRITE)
	{
		if (nData->buf != NULL) Writeline(NULL, 0, nData);
	}
	else if (nData->dir != FTP_CLIENT_READ) return 0;
	if (nData->buf) free(nData->buf);
	sceNetShutdown(nData->handle, 2);
	sceNetSocketClose(nData->handle);

	ctrl = nData->ctrl;
	free(nData);
	if (ctrl) return ReadResponse('2', ctrl);
	return 1;
}

/*
 * FtpQuit - disconnect from remote
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Quit()
{
	if (mp_ftphandle->handle == 0)
	{
		strcpy(mp_ftphandle->response, "error: no anwser from server\n");
		return 0;
	}
	FtpSendCmd("QUIT", '2' , mp_ftphandle);
	sceNetSocketClose(mp_ftphandle->handle);

	return 1;
}

ftphandle* FtpClient::RawOpen(const char *path, accesstype type, transfermode mode)
{
	int ret;
	ftphandle* datahandle;
	ret = FtpAccess(path, type, mode, mp_ftphandle, &datahandle); 
	if (ret) return datahandle;
	else return NULL;
}

int FtpClient::RawClose(ftphandle* handle)
{
	return FtpClose(handle);
} 

int FtpClient::RawWrite(void* buf, int len, ftphandle* handle)
{
	return FtpWrite(buf, len, handle);
}

int FtpClient::RawRead(void* buf, int max, ftphandle* handle)
{
	return FtpRead(buf, max, handle);
}

/*
 * FtpSite - send a SITE command
 *
 * return 1 if command successful, 0 otherwise
 */
int FtpClient::Site(const char *cmd)
{
	char buf[512];

	if ((strlen(cmd) + 7) > sizeof(buf)) return 0;
	sprintf(buf,"SITE %s",cmd);
	if (!FtpSendCmd(buf,'2',mp_ftphandle)) return 0;
	return 1;
}

/*
 * FtpRaw - send a raw string string
 *
 * return 1 if command successful, 0 otherwise
 */

int FtpClient::Raw(const char *cmd)
{
	char buf[512];
	strncpy(buf, cmd, 512);
	if (!FtpSendCmd(buf,'2',mp_ftphandle)) return 0;
	return 1;
}

/*
 * FtpSysType - send a SYST command
 *
 * Fills in the user buffer with the remote system type.  If more
 * information from the response is required, the user can parse
 * it out of the response buffer returned by FtpLastResponse().
 *
 * return 1 if command successful, 0 otherwise
 */
int FtpClient::SysType(char *buf, int max)
{
	int l = max;
	char *b = buf;
	char *s;
	if (!FtpSendCmd("SYST",'2',mp_ftphandle)) return 0;
	s = &mp_ftphandle->response[4];
	while ((--l) && (*s != ' ')) *b++ = *s++;
	*b++ = '\0';
	return 1;
}

/*
 * FtpMkdir - create a directory at server
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Mkdir(const char *path)
{
	char buf[512];

	if ((strlen(path) + 6) > sizeof(buf)) return 0;
	sprintf(buf,"MKD %s",path);
	if (!FtpSendCmd(buf,'2', mp_ftphandle)) return 0;
	return 1;
}

/*
 * FtpChdir - change path at remote
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Chdir(const char *path)
{
	char buf[512];

	if ((strlen(path) + 6) > sizeof(buf)) return 0;
	sprintf(buf,"CWD %s",path);
	if (!FtpSendCmd(buf,'2',mp_ftphandle)) return 0;
	return 1;
}

/*
 * FtpCDUp - move to parent directory at remote
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Cdup()
{
	if (!FtpSendCmd("CDUP",'2',mp_ftphandle)) return 0;
	return 1;
}

/*
 * FtpRmdir - remove directory at remote
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Rmdir(const char *path)
{
	char buf[512];

	if ((strlen(path) + 6) > sizeof(buf)) return 0;
	sprintf(buf,"RMD %s",path);
	if (!FtpSendCmd(buf,'2',mp_ftphandle)) return 0;
	return 1;
}

/*
 * FtpPwd - get working directory at remote
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Pwd(char *path, int max)
{
	int l = max;
	char *b = path;
	char *s;

 	if (!FtpSendCmd("PWD",'2',mp_ftphandle)) return 0;
	s = strchr(mp_ftphandle->response, '"');
	if (s == NULL) return 0;
   s++;
	while ((--l) && (*s) && (*s != '"')) *b++ = *s++;
	*b = '\0';
	return 1;
}

/*
 * FtpSize - determine the size of a remote file
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Size(const char *path, int64_t *size, transfermode mode)
{
   char cmd[512];
   int resp,sz,rv=1;

   if ((strlen(path) + 7) > sizeof(cmd)) return 0;

	sprintf(cmd, "TYPE %c", mode);
   if (!FtpSendCmd(cmd, '2', mp_ftphandle)) return 0;

	sprintf(cmd,"SIZE %s",path);
   if (!FtpSendCmd(cmd,'2',mp_ftphandle)) rv = 0;
   else
   {
		if (sscanf(mp_ftphandle->response, "%d %d", &resp, &sz) == 2) *size = sz;
		else rv = 0;
   }
   return rv;
}

/*
 * FtpModDate - determine the modification date of a remote file
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::ModDate(const char *path, char *dt, int max)
{
	char buf[512];
	int rv = 1;

	if ((strlen(path) + 7) > sizeof(buf)) return 0;
	sprintf(buf,"MDTM %s",path);
   if (!FtpSendCmd(buf,'2',mp_ftphandle)) rv = 0;
	else strncpy(dt, &mp_ftphandle->response[4], max);
	return rv;
}

/*
 * FtpGet - issue a GET command and write received data to output
 *
 * return 1 if successful, 0 otherwise
 */

int FtpClient::Get(const char *outputfile, const char *path, transfermode mode, int64_t offset)
{
	mp_ftphandle->offset = offset;
	if (offset == 0) return FtpXfer(outputfile, path, mp_ftphandle, FtpClient::fileread, mode);
	else return FtpXfer(outputfile, path, mp_ftphandle, FtpClient::filereadappend, mode);
}

/*
 * FtpPut - issue a PUT command and send data from input
 *
 * return 1 if successful, 0 otherwise
 */

int FtpClient::Put(const char *inputfile, const char *path, transfermode mode, int64_t offset)
{
	mp_ftphandle->offset = offset;
	if (offset == 0) return FtpXfer(inputfile, path, mp_ftphandle, FtpClient::filewrite, mode);
	else return FtpXfer(inputfile, path, mp_ftphandle, FtpClient::filewriteappend, mode);
}


int FtpClient::Rename(const char *src, const char *dst)
{
	char cmd[512];

	if (((strlen(src) + 7) > sizeof(cmd)) || ((strlen(dst) + 7) > sizeof(cmd))) return 0;
	sprintf(cmd,"RNFR %s",src);
	if (!FtpSendCmd(cmd,'3',mp_ftphandle)) return 0;
	sprintf(cmd,"RNTO %s",dst);
	if (!FtpSendCmd(cmd,'2',mp_ftphandle)) return 0;

	return 1;
}

int FtpClient::Delete(const char *path)
{
	char cmd[512];

	if ((strlen(path) + 7) > sizeof(cmd)) return 0;
   	sprintf(cmd,"DELE %s",path);
   	if (!FtpSendCmd(cmd,'2', mp_ftphandle)) return 0;
   	return 1;
}

/**
  * @brief Parse directory entry
  * @param[in] line NULL-terminated string
  * @param[out] dirEntry Pointer to a directory entry
  * @return -1 on error or 1 on success
  **/
  
int FtpClient::ParseDirEntry(char *line, FtpDirEntry *dirEntry)
{
    unsigned int i;
    size_t n;
    char *p;
    char *token;
  
    //Abbreviated months
    static const char months[13][4] =
    {
       "   ",
       "Jan",
       "Feb",
       "Mar",
       "Apr",
       "May",
       "Jun",
       "Jul",
       "Aug",
       "Sep",
       "Oct",
       "Nov",
       "Dec"
    };
  
    //Read first field
    token = strtok_r(line, " \t", &p);

    //Invalid directory entry?
    if(token == NULL)
       return -1;
  
    //MS-DOS listing format?
    if(isdigit(token[0]))
    {
       //Check modification date format
       if(strlen(token) == 8 && token[2] == '-' && token[5] == '-')
       {
          //The format of the date is mm-dd-yy
          dirEntry->modified.month = (uint8_t) strtoul(token, NULL, 10);
          dirEntry->modified.day = (uint8_t) strtoul(token + 3, NULL, 10);
          dirEntry->modified.year = (uint16_t) strtoul(token + 6, NULL, 10) + 2000;
       }
       else if(strlen(token) == 10 && token[2] == '/' && token[5] == '/')
       {
          //The format of the date is mm/dd/yyyy
          dirEntry->modified.month = (uint8_t) strtoul(token, NULL, 10);
          dirEntry->modified.day = (uint8_t) strtoul(token + 3, NULL, 10);
          dirEntry->modified.year = (uint16_t) strtoul(token + 6, NULL, 10);
       }
       else
       {
          //Invalid time format
          return -1;
       }
  
       //Read modification time
       token = strtok_r(NULL, " ", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Check modification time format
       if(strlen(token) >= 5 && token[2] == ':')
       {
          //The format of the time hh:mm
          dirEntry->modified.hours = (uint8_t) strtoul(token, NULL, 10);
          dirEntry->modified.minutes = (uint8_t) strtoul(token + 3, NULL, 10);
  
          //The PM period covers the 12 hours from noon to midnight
          if(strstr(token, "PM") != NULL)
          {
             dirEntry->modified.hours += 12;
          }
       }
       else
       {
          //Invalid time format
          return -1;
       }
  
       //Read next field
       token = strtok_r(NULL, " ", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Check whether the current entry is a directory
       if(!strcmp(token, "<DIR>"))
       {
          //Update attributes
          dirEntry->isDir |= true;
       }
       else
       {
          //Save the size of the file
          dirEntry->size = strtoul(token, NULL, 10);
       }
  
       //Read filename field
       token = strtok_r(NULL, "\r\n", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Retrieve the length of the filename
       n = strlen(token);
       //Limit the number of characters to copy
       n = MIN(n, FTP_CLIENT_MAX_FILENAME_LEN);
  
       //Copy the filename
       strncpy(dirEntry->name, token, n);
       //Properly terminate the string with a NULL character
       dirEntry->name[n] = '\0';
    }
    //Unix listing format?
    else
    {
       //Check file permissions
       if(strchr(token, 'd') != NULL)
       {
          dirEntry->isDir = true;
       }
  
       //Read next field
       token = strtok_r(NULL, " ", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Discard owner field
       token = strtok_r(NULL, " ", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Discard group field
       token = strtok_r(NULL, " ", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Read size field
       token = strtok_r(NULL, " ", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Save the size of the file
       dirEntry->size = strtoul(token, NULL, 10);
  
       //Read modification time (month)
       token = strtok_r(NULL, " ", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Decode the 3-letter month name
       for(i = 1; i <= 12; i++)
       {
          //Compare month name
          if(!strcmp(token, months[i]))
          {
             //Save month number
             dirEntry->modified.month = i;
             break;
          }
       }
  
       //Read modification time (day)
       token = strtok_r(NULL, " ", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Save day number
       dirEntry->modified.day = (uint8_t) strtoul(token, NULL, 10);
  
       //Read next field
       token = strtok_r(NULL, " ", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Check modification time format
       if(strlen(token) == 4)
       {
          //The format of the year is yyyy
          dirEntry->modified.year = (uint16_t) strtoul(token, NULL, 10);
  
       }
       else if(strlen(token) == 5)
       {
          //The format of the time hh:mm
          token[2] = '\0';
          dirEntry->modified.hours = (uint8_t) strtoul(token, NULL, 10);
          dirEntry->modified.minutes = (uint8_t) strtoul(token + 3, NULL, 10);
       }
       else
       {
          //Invalid time format
          return -1;
       }
  
       //Read filename field
       token = strtok_r(NULL, "\r\n", &p);
       //Invalid directory entry?
       if(token == NULL)
          return -1;
  
       //Retrieve the length of the filename
       n = strlen(token);
       //Limit the number of characters to copy
       n = MIN(n, FTP_CLIENT_MAX_FILENAME_LEN);
  
       //Copy the filename
       strncpy(dirEntry->name, token, n);
       //Properly terminate the string with a NULL character
       dirEntry->name[n] = '\0';
    }
  
    //The directory entry is valid
    return 1;
 }

std::vector<std::string> FtpClient::ListFiles(const char *path, std::regex *regexpr, bool includeSubDir)
{
	std::vector<std::string> out;
	std::vector<FtpDirEntry> list = ListDir(path);
	for (int i=0; i<list.size(); i++)
	{
		if (list[i].isDir && includeSubDir)
		{
			std::string new_path = std::string(path) + "/" + list[i].name;
			std::vector<std::string> files = ListFiles(new_path.c_str(), regexpr, includeSubDir);
			for (std::vector<std::string>::iterator it=files.begin(); it!=files.end(); )
			{
				if (regexpr != nullptr)
				{
					if (std::regex_search(*it, *regexpr))
					{
						out.push_back(std::string(list[i].name) + "/" + *it);
					}
				}
				else
				{
					out.push_back(std::string(list[i].name) + "/" + *it);
				}
				++it;
			}
		}
		else
		{
			if (regexpr != nullptr)
			{
				if (std::regex_search(list[i].name, *regexpr))
				{
					out.push_back(std::string(list[i].name));
				}
			}
			else
			{
				out.push_back(std::string(list[i].name));
			}
		}
	}
	return out;
}

std::vector<FtpDirEntry> FtpClient::ListDir(const char *path)
{
	std::vector<FtpDirEntry> out;
	ftphandle *nData;
	char buf[1024];
	int ret;
	mp_ftphandle->offset = 0;

	nData = RawOpen(path, FtpClient::dirverbose, FtpClient::ascii);
	if (nData != NULL)
	{
		ret = FtpRead(buf, 1024, nData);
		while (ret > 0)
		{
			FtpDirEntry entry;
			memset(&entry, 0, sizeof(entry));
			if (ParseDirEntry(buf, &entry) > 0)
			{
				out.push_back(entry);
			}
			ret = FtpRead(buf, 1024, nData);
		}
		FtpClose(nData);
	}

	return out;
}

void FtpClient::SetCallbackXferFunction(FtpCallbackXfer pointer)
{
	mp_ftphandle->xfercb = pointer;
}

void FtpClient::SetCallbackArg(void *arg)
{
	mp_ftphandle->cbarg = arg;
}

void FtpClient::SetCallbackBytes(int64_t bytes)
{
	mp_ftphandle->cbbytes = bytes;
}
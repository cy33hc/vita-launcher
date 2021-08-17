#include <psp2/net/net.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ftpclient.h>
#include <debugnet.h>
#include <errno.h>

FtpClient::FtpClient()
{
    mp_ftphandle = static_cast<ftphandle *>(calloc(1,sizeof(ftphandle)));
	if (mp_ftphandle == NULL) perror("calloc");
	mp_ftphandle->buf = static_cast<char *>(malloc(FTPLIB_BUFSIZ));
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

    mp_ftphandle->dir = FTPLIB_CONTROL;
    mp_ftphandle->ctrl = NULL;
    mp_ftphandle->xfered = 0;
    mp_ftphandle->xfered1 = 0;
    mp_ftphandle->offset = 0;
    mp_ftphandle->handle = 0;

    memset(&server_addr, 0, sizeof(server_addr));
    sceNetInetPton(SCE_NET_AF_INET, host, (void*)&dst_addr);
    server_addr.sin_addr = dst_addr;
    server_addr.sin_port = sceNetHtons(port);
    server_addr.sin_family = SCE_NET_AF_INET;

    sControl = sceNetSocket("ftp_control", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, SCE_NET_IPPROTO_TCP);
    if (mp_ftphandle->handle < 0)
    {
        debugNetPrintf(ERROR, "sceNetSocket error\n");
        return 0;
    }

    retval = sceNetSetsockopt(sControl, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, (const void*)&on, sizeof(on));
    if (retval == -1)
    {
        debugNetPrintf(ERROR, "sceNetSetsockopt error\n");
        return 0;
    }

    retval = sceNetConnect(sControl, (SceNetSockaddr *)&server_addr, sizeof(server_addr));
    if (retval == -1)
    {
        debugNetPrintf(ERROR, "sceNetConnect error\n");
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
	char buf[FTPLIB_BUFSIZ];
	int x;

	if (!nControl->handle) return 0;
    if (nControl->dir != FTPLIB_CONTROL) return 0;

	sprintf(buf, "%s\r\n", cmd);
    x = sceNetSend(nControl->handle, buf, strlen(buf), 0);
	if (x <= 0)
	{
		debugNetPrintf(ERROR, "sceNetSend error\n");
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
	
	if (Readline(nControl->response, FTPLIB_BUFSIZ, nControl) == -1)
	{
		debugNetPrintf(ERROR,"Readline error\n");
		return 0;
	}
	
	if (nControl->response[3] == '-')
	{
		strncpy(match, nControl->response, 3);
		match[3] = ' ';
		match[4] = '\0';
		do
		{
			if (Readline(nControl->response, FTPLIB_BUFSIZ, nControl) == -1)
			{
				debugNetPrintf(ERROR,"Readline error\n");
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
			nControl->cleft = FTPLIB_BUFSIZ;
		}

		if (eof)
		{
			if (retval == 0) retval = -1;
			break;
		}

		x = sceNetRecv(nControl->handle, nControl->cput, nControl->cleft, 0);		

		if ( x == -1)
		{
			debugNetPrintf(ERROR,"sceNetRecv error\n");
			retval = -1;
			break;
		}

		if (x == 0) eof = 1;
		nControl->cleft -= x;
		nControl->cavail += x;
		nControl->cput += x;
	} while (1);
    debugNetPrintf(DEBUG,"Response = %s\n", buf);
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
	if ((mp_ftphandle) && (mp_ftphandle->dir == FTPLIB_CONTROL)) return mp_ftphandle->response;
	return NULL;
}

void FtpClient::ClearHandle()
{
	mp_ftphandle->dir = FTPLIB_CONTROL;
	mp_ftphandle->ctrl = NULL;
	mp_ftphandle->cmode = FtpClient::pasv;
    mp_ftphandle->idletime.tv_sec = mp_ftphandle->idletime.tv_usec = 0;
	mp_ftphandle->xfered = 0;
	mp_ftphandle->xfered1 = 0;
	mp_ftphandle->cbbytes = 0;
	mp_ftphandle->offset = 0;
	mp_ftphandle->handle = 0;
	mp_ftphandle->correctpasv = false;
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
	char buf[FTPLIB_BUFSIZ];
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
		dir = FTPLIB_READ;
		break;
	case FtpClient::dirverbose:
		strcpy(buf,"LIST -aL");
		dir = FTPLIB_READ;
		break;
	case FtpClient::filereadappend:
	case FtpClient::fileread:
		strcpy(buf,"RETR");
		dir = FTPLIB_READ;
		break;
    case FtpClient::filewriteappend:
	case FtpClient::filewrite:
		strcpy(buf,"STOR");
		dir = FTPLIB_WRITE;
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

	if (nControl->dir != FTPLIB_CONTROL) return -1;
	if ((dir != FTPLIB_READ) && (dir != FTPLIB_WRITE))
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
		char buf[FTPLIB_BUFSIZ];
        sprintf(buf, "REST %lld", mp_ftphandle->offset);
		if (!FtpSendCmd(buf,'3',nControl)) return 0;
	}

	sData = sceNetSocket("ftp_data", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, SCE_NET_IPPROTO_TCP);
	if (sData == -1)
	{
		debugNetPrintf(ERROR, "sceNetSocket data error\n");
		return -1;
	}

	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, (const void*)&on, sizeof(on)) == -1)
	{
		debugNetPrintf(ERROR, "sceNetSetsockopt SCE_NET_SO_REUSEADDR error\n");
		sceNetSocketClose(sData);
		return -1;
	}

	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_LINGER, &lng, sizeof(lng)) == -1)
	{
        debugNetPrintf(ERROR, "sceNetSetsockopt data SCE_NET_SO_LINGER error\n");
		sceNetSocketClose(sData);
		return -1;
	}

	if (nControl->dir != FTPLIB_CONTROL) return -1;
	memcpy(cmd + strlen(cmd), "\r\n\0", 3);
	ret = sceNetSend(nControl->handle, cmd, strlen(cmd), 0);
	if (ret <= 0)
	{
		debugNetPrintf(ERROR, "sceNetSend error\n");
		return -1;
	}

    debugNetPrintf(DEBUG, "Start  sceNetConnect\n");
	if (sceNetConnect(sData, &sin.sa, sizeof(sin.sa)) == -1)
	{
		debugNetPrintf(ERROR, "sceNetConnect data error\n");
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
		debugNetPrintf(ERROR, "calloc ctrl error\n");
		sceNetSocketClose(sData);
		return -1;
	}
	if ((mode == 'A') && ((ctrl->buf = static_cast<char*>(malloc(FTPLIB_BUFSIZ))) == NULL))
	{
		debugNetPrintf(ERROR, "calloc ctrl-buf error\n");
		sceNetSocketClose(sData);
		free(ctrl);
		return -1;
	}
	ctrl->handle = sData;
	ctrl->dir = dir;
	ctrl->ctrl = (nControl->cmode == FtpClient::pasv) ? nControl : NULL;
    ctrl->idletime = nControl->idletime;
	ctrl->xfered = 0;
	ctrl->xfered1 = 0;
	ctrl->cbbytes = nControl->cbbytes;
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
	char buf[256];

	if (nControl->dir != FTPLIB_CONTROL) return -1;
	if ((dir != FTPLIB_READ) && (dir != FTPLIB_WRITE))
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
		debugNetPrintf(ERROR, "sceNetGetsockname error\n");
		return -1;
	}

	sData = sceNetSocket("ftp_data", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, SCE_NET_IPPROTO_TCP);
	if (sData == -1)
	{
		debugNetPrintf(ERROR, "sceNetSocket error\n");
		return -1;
	}
	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, (const void*)&on, sizeof(on)) == -1)
	{
		debugNetPrintf(ERROR, "sceNetSetsockopt SCE_NET_SO_REUSEADDR error\n");
		sceNetSocketClose(sData);
		return -1;
	}

	if (sceNetSetsockopt(sData, SCE_NET_SOL_SOCKET, SCE_NET_SO_LINGER, &lng, sizeof(lng)) == -1)
	{
        debugNetPrintf(ERROR, "sceNetSetsockopt data SCE_NET_SO_LINGER error\n");
		sceNetSocketClose(sData);
		return -1;
	}

	sin.in.sin_port = 0;
	if (sceNetBind(sData, &sin.sa, sizeof(sin)) == -1)
	{
		debugNetPrintf(ERROR, "sceNetBind data error\n");
		sceNetSocketClose(sData);
		return -1;
	}
	if (sceNetListen(sData, 1) < 0)
	{
		debugNetPrintf(ERROR, "sceNetListen data error\n");
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
	char buf[FTPLIB_BUFSIZ];
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
		debugNetPrintf(ERROR, "calloc ctrl error\n");
		sceNetSocketClose(sData);
		return -1;
	}
	if ((mode == 'A') && ((ctrl->buf = static_cast<char*>(malloc(FTPLIB_BUFSIZ))) == NULL))
	{
		debugNetPrintf(ERROR, "calloc buf error\n");
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
	ctrl->idletime = nControl->idletime;
	ctrl->xfered = 0;
	ctrl->xfered1 = 0;
	ctrl->cbbytes = nControl->cbbytes;
	*nData = ctrl;

	return 1;
}

int FtpClient::CorrectPasvResponse(int *v)
{
	SceNetSockaddr ipholder;
	uint32_t ipholder_size = sizeof(ipholder);

	if (sceNetGetpeername(mp_ftphandle->handle, &ipholder, &ipholder_size) == -1)
	{
		debugNetPrintf(ERROR, "sceNetGetpeername error\n");
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

	dbuf = static_cast<char*>(malloc(FTPLIB_BUFSIZ));
	if ((type == FtpClient::filewrite) || (type == FtpClient::filewriteappend))
	{
		while ((l = fread(dbuf, 1, FTPLIB_BUFSIZ, local)) > 0)
		{
			if ((c = FtpWrite(dbuf, l, nData)) < l)
			{
				debugNetPrintf(ERROR, "short write: passed %d, wrote %d\n", l, c);
				break;
			}
		}
	}
	else
	{
		while ((l = FtpRead(dbuf, FTPLIB_BUFSIZ, nData)) > 0)
		{
			if (fwrite(dbuf, 1, l, local) <= 0)
			{
				debugNetPrintf(ERROR, "localfile write\n");
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

	if (nData->dir != FTPLIB_WRITE) return 0;
	if (nData->buf) i = Writeline(static_cast<char*>(buf), len, nData);
	else
	{
		i = sceNetSend(nData->handle, buf, len, 0);
	}
	if (i == -1) return 0;
	nData->xfered += i;

	return i;
}

/*
 * FtpRead - read from a data connection
 */
int FtpClient::FtpRead(void *buf, int max, ftphandle *nData)
{
	int i;

	if (nData->dir != FTPLIB_READ)
	return 0;
	if (nData->buf) i = Readline(static_cast<char*>(buf), max, nData);
	else
	{
		i = sceNetRecv(nData->handle, buf, max, 0);
	}
	if (i == -1) return 0;
	nData->xfered += i;
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

	if (nData->dir != FTPLIB_WRITE)
	return -1;
	nbp = nData->buf;
	for (x=0; x < len; x++)
	{
		if ((*ubp == '\n') && (lc != '\r'))
		{
			if (nb == FTPLIB_BUFSIZ)
			{
				w = sceNetSend(nData->handle, nbp, FTPLIB_BUFSIZ, 0);
				if (w != FTPLIB_BUFSIZ)
				{
					debugNetPrintf(ERROR, "write(1) returned %d, errno = %d\n", w, errno);
					return(-1);
				}
				nb = 0;
			}
			nbp[nb++] = '\r';
		}
		if (nb == FTPLIB_BUFSIZ)
		{
			w = sceNetSend(nData->handle, nbp, FTPLIB_BUFSIZ, 0);
			if (w != FTPLIB_BUFSIZ)
			{
				debugNetPrintf(ERROR, "write(2) returned %d, errno = %d\n", w, errno);
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
			debugNetPrintf(ERROR, "write(3) returned %d, errno = %d\n", w, errno);
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

	if (nData->dir == FTPLIB_WRITE)
	{
		if (nData->buf != NULL) Writeline(NULL, 0, nData);
	}
	else if (nData->dir != FTPLIB_READ) return 0;
	if (nData->buf) free(nData->buf);
	sceNetShutdown(nData->handle, 2);
	sceNetSocketClose(nData->handle);

	ctrl = nData->ctrl;
	free(nData);
	if (ctrl) return ReadResponse('2', ctrl);
	return 1;
}

/*
 * FtpNlst - issue an NLST command and write response to output
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Nlst(const char *outputfile, const char *path)
{
	mp_ftphandle->offset = 0;
	return FtpXfer(outputfile, path, mp_ftphandle, FtpClient::dir, FtpClient::ascii);
}

/*
 * FtpDir - issue a LIST command and write response to output
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Dir(const char *outputfile, const char *path)
{
	mp_ftphandle->offset = 0;
	return FtpXfer(outputfile, path, mp_ftphandle, FtpClient::dirverbose, FtpClient::ascii);
}

/*
 * FtpQuit - disconnect from remote
 *
 * return 1 if successful, 0 otherwise
 */
int FtpClient::Quit()
{
	if (mp_ftphandle->dir != FTPLIB_CONTROL) return 0;
	if (mp_ftphandle->handle == 0)
	{
		strcpy(mp_ftphandle->response, "error: no anwser from server\n");
		return 0;
	}
	if (!FtpSendCmd("QUIT", '2' , mp_ftphandle))
	{
		sceNetSocketClose(mp_ftphandle->handle);
		return 0;
	}
	else
	{
		sceNetSocketClose(mp_ftphandle->handle);
		return 1;
	}
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
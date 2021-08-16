#include <psp2/net/net.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ftpclient.h>
#include <debugnet.h>

FtpClient::FtpClient()
{
    this->buf = static_cast<char *>(malloc(FTPLIB_BUFSIZ));
    this->cput = this->cget = this->buf;
    this->cavail = this->cleft = 0;
}

FtpClient::~FtpClient()
{
    free(this->buf);
}

int FtpClient::Connect(const char *host, unsigned short port)
{
    SceNetInAddr dst_addr; /* destination address */
    int on = 1; /* used in Setsockopt function */
    int32_t retval; /* return value */

    memset(&this->server_addr, 0, sizeof(this->server_addr));
    sceNetInetPton(SCE_NET_AF_INET, host, (void*)&dst_addr);
    this->server_addr.sin_addr = dst_addr;
    this->server_addr.sin_port = sceNetHtons(port);
    this->server_addr.sin_family = SCE_NET_AF_INET;

    this->ctrl_sockfd = sceNetSocket("ftp", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, SCE_NET_IPPROTO_TCP);
    if (this->ctrl_sockfd < 0)
    {
        return 0;
    }

    retval = sceNetSetsockopt(this->ctrl_sockfd, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, (const void*)&on, sizeof(on));
    if (retval == -1)
    {
        return 0;
    }

    retval = sceNetConnect(this->ctrl_sockfd, (SceNetSockaddr *)&this->server_addr, sizeof(this->server_addr));
    if (retval == -1)
    {
        return 0;
    }

    //char buf[1024];
    //int x = sceNetRecv(this->ctrl_sockfd, buf, 1024, 0);
    //debugNetPrintf(DEBUG,"Response = %s\n", buf);

    if (ReadResponse('2') == 0)
    {
        sceNetSocketClose(this->ctrl_sockfd);
        this->ctrl_sockfd = 0;
        return 0;
    }

    return 1;
}

/*
 * FtpSendCmd - send a command and wait for expected response
 *
 * return 1 if proper response received, 0 otherwise
 */
int FtpClient::FtpSendCmd(const char *cmd, char expected_resp)
{
	char buf[FTPLIB_BUFSIZ];
	int x;

	if (!this->ctrl_sockfd) return 0;

	sprintf(buf, "%s\r\n", cmd);
    x = sceNetSend(this->ctrl_sockfd, buf, strlen(buf), 0);
	if (x <= 0)
	{
		debugNetPrintf(ERROR, "sceNetSend error\n");
		return 0;
	}
	
	return 1;
}

/*
 * read a response from the server
 *
 * return 0 if first char doesn't match
 * return 1 if first char matches
 */
int FtpClient::ReadResponse(char c)
{
	char match[5];
	
	if (Readline(this->response, FTPLIB_BUFSIZ) == -1)
	{
		debugNetPrintf(ERROR,"Readline error\n");
		return 0;
	}
	
	if (this->response[3] == '-')
	{
		strncpy(match, this->response, 3);
		match[3] = ' ';
		match[4] = '\0';
		do
		{
			if (Readline(this->response, FTPLIB_BUFSIZ) == -1)
			{
				debugNetPrintf(ERROR,"Readline error\n");
				return 0;
			}
		} while (strncmp(this->response, match, 4));
	}

	if (this->response[0] == c) return 1;
	return 0;
}

/*
 * read a line of text
 *
 * return -1 on error or bytecount
 */
int FtpClient::Readline(char *buf, int max)
{
	int x, retval = 0;
	char *end, *bp = buf;
	int eof = 0;

	if (max == 0) return 0;

	do
	{
		if (this->cavail > 0)
		{
			x = (max >= this->cavail) ? this->cavail : max-1;
			end = static_cast<char*>(memccpy(bp, this->cget, '\n', x));
			if (end != NULL) x = end - bp;
			retval += x;
			bp += x;
			*bp = '\0';
			max -= x;
			this->cget += x;
			this->cavail -= x;
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

		if (this->cput == this->cget)
		{
			this->cput = this->cget = this->buf;
			this->cavail = 0;
			this->cleft = FTPLIB_BUFSIZ;
		}

		if (eof)
		{
			if (retval == 0) retval = -1;
			break;
		}

		x = sceNetRecv(this->ctrl_sockfd, this->cput, this->cleft, 0);		

		if ( x == -1)
		{
			debugNetPrintf(ERROR,"sceNetRecv error\n");
			retval = -1;
			break;
		}

		if (x == 0) eof = 1;
		this->cleft -= x;
		this->cavail += x;
		this->cput += x;
	} while (1);
    debugNetPrintf(DEBUG,"Response = %s\n", buf);
	return retval;
}
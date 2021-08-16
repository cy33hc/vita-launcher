#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <psp2/net/net.h>

#define FTPLIB_BUFSIZ 1024

class FtpClient {
private:
	/* Control connection socket FD */
	int ctrl_sockfd;
	/* Data connection attributes */
	int data_sockfd;
	SceNetSockaddrIn data_sockaddr;
	/* PASV mode client socket */
	SceNetSockaddrIn pasv_sockaddr;
	int pasv_sockfd;
	/* Remote server net info */
	SceNetSockaddrIn server_addr;
	char *cput, *cget;
	int cavail, cleft;
	char *buf;
	char response[FTPLIB_BUFSIZ];

	int ReadResponse(char c);
	int Readline(char *buf, int max);

public:
    FtpClient();
    ~FtpClient();
    int Connect(const char *host, unsigned short port);
	int FtpSendCmd(const char *cmd, char expected_resp);
};

#endif
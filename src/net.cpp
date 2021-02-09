/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <vitasdk.h>
#include <cstdlib>
#include "certs.h"
//#include "debugnet.h"

#define VITALAUNCHER_USER_AGENT "VitaLauncher/1.00 libhttp/1.1"
#define NET_MEMORY_SIZE (4 * 1024 * 1024)

char *net_memory = nullptr;
static SceHttpsData* caList[4];

namespace Net
{
  void Init()
  {
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);
    sceSysmoduleLoadModule(SCE_SYSMODULE_PSPNET_ADHOC);
    net_memory = (char*)malloc(NET_MEMORY_SIZE);

    SceNetInitParam param;
    param.memory = net_memory;
    param.size = NET_MEMORY_SIZE;
    param.flags = 0;

    sceNetInit(&param);
    sceNetCtlInit();

    sceSslInit(300 * 1024);
    sceHttpInit(40 * 1024);

    sceHttpsDisableOption(SCE_HTTPS_FLAG_SERVER_VERIFY);

    sceNetAdhocInit();

    SceNetAdhocctlAdhocId adhocId;
    memset(&adhocId, 0, sizeof(SceNetAdhocctlAdhocId));
    adhocId.type = SCE_NET_ADHOCCTL_ADHOCTYPE_RESERVED;
    memcpy(&adhocId.data[0], "SMLA00001", SCE_NET_ADHOCCTL_ADHOCID_LEN);
    sceNetAdhocctlInit(&adhocId);

    caList[0] = (SceHttpsData*) malloc(sizeof(SceHttpsData));
    caList[1] = (SceHttpsData*) malloc(sizeof(SceHttpsData));
    caList[2] = (SceHttpsData*) malloc(sizeof(SceHttpsData));
    caList[3] = (SceHttpsData*) malloc(sizeof(SceHttpsData));
    caList[0]->ptr = DigiCert_High_Assurance_EV_Root_CA_pem;
    caList[0]->size = DigiCert_High_Assurance_EV_Root_CA_pem_size;
    caList[1]->ptr = DigiCert_SHA2_High_Assurance_Server_CA_pem;
    caList[1]->size = DigiCert_SHA2_High_Assurance_Server_CA_pem_size;
    caList[2]->ptr = Baltimore_CyberTrust_Root_pem;
    caList[2]->size = Baltimore_CyberTrust_Root_pem_size;
    caList[3]->ptr = Cloudflare_Inc_ECC_CA_3_pem;
    caList[3]->size = Cloudflare_Inc_ECC_CA_3_pem_size;
    int res = sceHttpsLoadCert(4, (const SceHttpsData**)caList, NULL, NULL);

  }

  void Exit()
  {
    sceNetAdhocctlTerm();
    sceNetAdhocTerm();
    sceSslTerm();
    sceHttpTerm();
    sceNetCtlTerm();
    sceNetTerm();
    free(net_memory);
    free(caList[0]);
    free(caList[1]);
    free(caList[2]);
    free(caList[3]);
    sceSysmoduleUnloadModule(SCE_SYSMODULE_PSPNET_ADHOC);
    sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
    sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
  }

  int GetDownloadFileSize(const char *src, uint64_t *size)
  {
    int res;
    int statusCode;
    int tmplId = -1, connId = -1, reqId = -1;


    res = sceHttpCreateTemplate(VITALAUNCHER_USER_AGENT, SCE_HTTP_VERSION_1_1, SCE_TRUE);
    if (res < 0)
      goto ERROR_EXIT;

    tmplId = res;

    res = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
    if (res < 0)
      goto ERROR_EXIT;

    connId = res;

    res = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src, 0);
    if (res < 0)
      goto ERROR_EXIT;

    reqId = res;

    res = sceHttpSendRequest(reqId, NULL, 0);
    if (res < 0)
      goto ERROR_EXIT;

    res = sceHttpGetStatusCode(reqId, &statusCode);
    if (res < 0)
      goto ERROR_EXIT;

    if (statusCode == 200) {
      res = sceHttpGetResponseContentLength(reqId, size);
    }

  ERROR_EXIT:
    if (reqId >= 0)
      sceHttpDeleteRequest(reqId);

    if (connId >= 0)
      sceHttpDeleteConnection(connId);

    if (tmplId >= 0)
      sceHttpDeleteTemplate(tmplId);

    return res;
  }

  int GetFieldFromHeader(const char *src, const char *field, const char **data, unsigned int *valueLen)
  {
    int res;
    char *header;
    unsigned int headerSize;
    int tmplId = -1, connId = -1, reqId = -1;

    res = sceHttpCreateTemplate(VITALAUNCHER_USER_AGENT, SCE_HTTP_VERSION_1_1, SCE_TRUE);
    if (res < 0)
      goto ERROR_EXIT;

    tmplId = res;

    res = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
    if (res < 0)
      goto ERROR_EXIT;

    connId = res;
    
    res = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src,  0);
    if (res < 0)
      goto ERROR_EXIT;
    
    reqId = res;
    
    res = sceHttpSendRequest(reqId, NULL, 0);
    if (res < 0)
      goto ERROR_EXIT;

    res = sceHttpGetAllResponseHeaders(reqId, &header, &headerSize);
    if (res < 0)
      goto ERROR_EXIT;

    res = sceHttpParseResponseHeader(header, headerSize, field, data, valueLen);
    if (res < 0) {
      *data = "";
      *valueLen = 0;
      res = 0;
    }
    
  ERROR_EXIT:
    if (reqId >= 0)
      sceHttpDeleteRequest(reqId);

    if (connId >= 0)
      sceHttpDeleteConnection(connId);

    if (tmplId >= 0)
      sceHttpDeleteTemplate(tmplId);

    return res;
  }

  int DownloadFile(const char *src, const char *dst)
  {
    int res;
    int statusCode;
    int tmplId = -1, connId = -1, reqId = -1;
    SceUID fd = -1;
    int ret = 1;

    res = sceHttpCreateTemplate(VITALAUNCHER_USER_AGENT, SCE_HTTP_VERSION_1_1, SCE_TRUE);
    if (res < 0)
      goto ERROR_EXIT;

    tmplId = res;

    res = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
    if (res < 0)
      goto ERROR_EXIT;

    connId = res;

    res = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src, 0);
    if (res < 0)
      goto ERROR_EXIT;

    reqId = res;

    res = sceHttpSendRequest(reqId, NULL, 0);
    if (res < 0)
      goto ERROR_EXIT;

    res = sceHttpGetStatusCode(reqId, &statusCode);
    if (res < 0)
      goto ERROR_EXIT;

    if (statusCode == 200) {    
      res = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
      if (res < 0)
        goto ERROR_EXIT;

      fd = res;

      uint8_t buf[4096];

      while (1) {
        int read = sceHttpReadData(reqId, buf, sizeof(buf));
        
        if (read < 0) {
          res = read;
          break;
        }
        
        if (read == 0)
          break;

        int written = sceIoWrite(fd, buf, read);
        
        if (written < 0) {
          res = written;
          break;
        }
      }
    }
    else
    {
      res = -1;
    }
    

  ERROR_EXIT:
    if (fd >= 0)
      sceIoClose(fd);

    if (reqId >= 0)
      sceHttpDeleteRequest(reqId);

    if (connId >= 0)
      sceHttpDeleteConnection(connId);

    if (tmplId >= 0)
      sceHttpDeleteTemplate(tmplId);

    if (res < 0)
      return res;

    return ret;
  }
}
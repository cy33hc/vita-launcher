#include <vitasdk.h>
#include <malloc.h>
#include <curl/curl.h>
#include "net.h"

CURL *curl;
int plFD;

static size_t write_data_to_disk(void *ptr, size_t size, size_t nmemb, void *stream){
	size_t written = sceIoWrite( *(int*) stream, ptr, size*nmemb);
	return written;
}

namespace NET {
    void InitNet()
    {
        sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

        SceNetInitParam netInitParam;
        int size = NET_INIT_SIZE;
        netInitParam.memory = malloc(size);
        netInitParam.size = size;
        netInitParam.flags = 0;
        sceNetInit(&netInitParam);

        sceNetCtlInit();
    }

    void InitHttp()
    {
        sceSysmoduleLoadModule(SCE_SYSMODULE_SSL);
        sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);
        sceHttpInit(NET_INIT_SIZE);
        sceSslInit(NET_INIT_SIZE);
    }

    void ExitNet()
    {
        sceNetCtlTerm();
        sceNetTerm();
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
    }

    void ExitHttp()
    {
        sceSslTerm();
        sceHttpTerm();
        sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
        sceSysmoduleUnloadModule(SCE_SYSMODULE_SSL);
    }

    void Download(const char *url, const char *dest)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        if (curl) {
            curl_easy_reset(curl);
            CURLcode ret;
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
            ret = curl_easy_perform(curl);

            if(ret == CURLE_OK) {
                int plFD = sceIoOpen( dest , SCE_O_WRONLY | SCE_O_CREAT, 0777);
                curl_easy_reset(curl);
                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_to_disk);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &plFD);
                curl_easy_perform(curl);
                sceIoClose(plFD);
            }

        }
        
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }
}

#ifndef LAUNCHER_NET_H
#define LAUNCHER_NET_H

#define NET_INIT_SIZE 1*1024*1024

namespace NET {
    void InitNet();
    void InitHttp();
    void ExitNet();
    void ExitHttp();
    void Download(const char *url, const char *dest);
}

#endif

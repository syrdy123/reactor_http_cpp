#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "tcp_server.h"

int main(int argc,char* argv[])
{

#if 0
    if (argc < 3) {
        printf("./a.out port path \n");
        return -1;
    }

    unsigned short port = (unsigned short)atoi(argv[1]);
    chdir(argv[2]);
#else

    unsigned short port = 10000;
    chdir("/home/ubuntu/wurusai");

#endif
    //启动服务器实例
    TcpServer* server = new TcpServer(port, 4);
    server->run();

    return 0;
}
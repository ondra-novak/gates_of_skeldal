#include "sse_receiver.h"

#include <atomic>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <functional>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET sock_t;
  #define CLOSESOCK closesocket
  #define sock_init() { WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa); }
  #define sock_cleanup() WSACleanup()
  #define SHUT_RD SD_RECEIVE
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <fcntl.h>
  typedef int sock_t;
  #define INVALID_SOCKET -1
  #define CLOSESOCK close
  #define sock_init()
  #define sock_cleanup()
#endif

#define BUFFER_SIZE 1024


void sse_client_loop(const char *host, const char *port, std::function<void(const char *)> callback, std::stop_token tkn) {
    sock_init();

    struct addrinfo hints = {}, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return;
    }

    sock_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
        perror("socket");
        freeaddrinfo(res);
        return;
    }



    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        perror("connect");
        CLOSESOCK(sock);
        freeaddrinfo(res);
        return;
    }


    freeaddrinfo(res);

    {
        std::stop_callback _cb(tkn, [&]{
            shutdown(sock, SHUT_RD);
        });

        // Send HTTP GET request
        char req[512];
        snprintf(req, sizeof(req),
                 "GET /command HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Accept: text/event-stream\r\n"
                 "Connection: keep-alive\r\n\r\n", host);
        send(sock, req, strlen(req), 0);

        // Read response and extract "data: " lines
        char buffer[BUFFER_SIZE];
        int buf_len = 0;

        while (!tkn.stop_requested()) {
            int n = recv(sock, buffer + buf_len, BUFFER_SIZE - buf_len - 1, 0);
            if (n <= 0) {
                break;
            }

            buf_len += n;
            buffer[buf_len] = '\0';

            char *line_start = buffer;
            while (1) {
                char *newline = strstr(line_start, "\n");
                if (!newline) break;

                *newline = '\0';

                if (strncmp(line_start, "data: ", 6) == 0) {
                    callback(line_start + 6);
                }

                line_start = newline + 1;
            }

            // Move leftover data to start
            buf_len = strlen(line_start);
            memmove(buffer, line_start, buf_len);
        }

    }
    CLOSESOCK(sock);
    sock_cleanup();
    return;
}




typedef struct tag_sse_receiver {
    std::jthread thr;
}
SSE_RECEIVER;

SSE_RECEIVER *sse_receiver_install(MTQUEUE *q, const char *host, const char *port) {
    SSE_RECEIVER *sse = new SSE_RECEIVER;
    sse->thr = std::jthread([sse, q, host = std::string(host), port = std::string(port)](std::stop_token tkn){
        while (!tkn.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            sse_client_loop(host.c_str(), port.c_str(), [q](const char *msg){
                mtqueue_push(q, msg);
            }, tkn);
        }
    });
    return sse;
}
void sse_receiver_stop(SSE_RECEIVER *inst) {
    delete inst;
}


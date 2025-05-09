// sse_receiver.c
#include "sse_receiver.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET sock_t;
  #define CLOSESOCK closesocket
  #define sock_init() { WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa); }
  #define sock_cleanup() WSACleanup()
#else
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
  typedef int sock_t;
  #define INVALID_SOCKET -1
  #define CLOSESOCK close
  #define sock_init()
  #define sock_cleanup()
#endif

#define BUFFER_SIZE 4096

struct tag_sse_receiver {
    char host[256];
    char port[16];
    sock_t sock;
    int connected;
    char buffer[BUFFER_SIZE+1];
    size_t buf_len;
    size_t line_len;
    time_t next_attempt;
};

static void set_nonblocking(sock_t sock) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
#endif
}

static int connect_and_handshake(SSE_RECEIVER *sse) {
    struct addrinfo hints = {0}, *res = NULL;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(sse->host, sse->port, &hints, &res) != 0) {
        return 0;
    }

    sock_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(res);
        return 0;
    }

    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) != 0) {
        CLOSESOCK(sock);
        freeaddrinfo(res);
        return 0;
    }

    freeaddrinfo(res);

    const char *req_fmt =
        "GET /command HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Accept: text/event-stream\r\n"
        "Connection: keep-alive\r\n\r\n";

    char req[512];
    snprintf(req, sizeof(req), req_fmt, sse->host);
    send(sock, req, (int)strlen(req), 0);

    set_nonblocking(sock);
    sse->sock = sock;
    sse->buf_len = 0;
    sse->line_len = 0;
    sse->connected = 1;

    return 1;
}

SSE_RECEIVER *sse_receiver_create(const char *host, const char *port) {
    sock_init();
    SSE_RECEIVER *sse = calloc(1, sizeof(SSE_RECEIVER));
    strncpy(sse->host, host, sizeof(sse->host) - 1);
    strncpy(sse->port, port, sizeof(sse->port) - 1);
    sse->sock = INVALID_SOCKET;
    sse->connected = 0;
    sse->buf_len = 0;
    sse->line_len = 0;
    sse->next_attempt = 0;
    return sse;
}

size_t find_sep(const char *buffer, size_t size, char sep) {
    for (size_t i = 0; i < size; ++i) {
        if (buffer[i] == sep) return i;
    }
    return size;
}

const char *sse_receiver_receive(SSE_RECEIVER *sse) {
    while(1) {
        size_t rm = sse->buf_len-sse->line_len;
        if (rm) {
            const char *r = sse->buffer+sse->line_len;
            size_t sep = find_sep(r, rm, '\n');
            if (sep < rm) {
                sep += sse->line_len;
                sse->buffer[sep] = 0;
                sse->line_len = sep+1;

                if (strncmp(r,"data:",5) == 0) {
                    r+=5;
                    while (*r && isspace(*r)) ++r;
                    if (*r) {
                        return r;
                    }
                }
                continue;
            }
            if (r != sse->buffer) memmove(sse->buffer, r, rm);
        }
        sse->line_len = 0;
        sse->buf_len = rm;

        if (!sse->connected) {
            time_t t = time(NULL);
            if (t < sse->next_attempt) {
                return NULL;
            }
            if (!connect_and_handshake(sse)) {
                snprintf(sse->buffer, sizeof(sse->buffer) - 1,
                            "MESSAGE Failed to connect the command server %s:%s ",
                            sse->host, sse->port);
                sse->next_attempt = t+5;
                return sse->buffer;
            } else {
                return "MESSAGE Connected to command server";
            }
        }

        int n = recv(sse->sock, sse->buffer + sse->buf_len, (int)(BUFFER_SIZE - sse->buf_len), 0);
        if (n <= 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS)
#else
            if (errno != EWOULDBLOCK && errno != EAGAIN)
#endif
            {
                CLOSESOCK(sse->sock);
                sse->connected = 0;
            }
            return NULL;
        }

        sse->buf_len += n;
    }
}

void sse_receiver_destroy(SSE_RECEIVER *sse) {
    if (sse->connected) {
        CLOSESOCK(sse->sock);
    }
    free(sse);
    sock_cleanup();
}

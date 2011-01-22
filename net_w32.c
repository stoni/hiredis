/* Extracted from anet.c to work properly with Hiredis error reporting.
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "hiredis_w32.h"
#include "fmacros.h"
#include "hiredis.h"
#include "sds.h"

/* Forward declaration */
void __redisSetError(redisContext *c, int type, sds err);

static int redisCreateSocket(redisContext *c, int type) {
	SOCKET s;
	WSADATA wsa_data;
    int res;
	
	res = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (res != NO_ERROR) {
		__redisSetError(c,REDIS_ERR_IO,"Error at WSAStartup()\n");
		return REDIS_ERR;
	}

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		__redisSetError(c,REDIS_ERR_IO,
            sdscatprintf(sdsempty(), "Error at socket(): %ld\n", WSAGetLastError()));
		WSACleanup();
		return REDIS_ERR;
	}
    
    return s;
}

static int redisSetNonBlock(redisContext *c, int fd) {
    SOCKET s = fd;
	int res;
	long i_mode = c->flags & REDIS_BLOCK;
	
	res = ioctlsocket(s, FIONBIO, &i_mode);
	if (res != NO_ERROR) {
		__redisSetError(c,REDIS_ERR_IO,
            sdscatprintf(sdsempty(), "ioctlsocket failed with error: %ld\n", WSAGetLastError()));
		closesocket(c->fd);
		WSACleanup();
		return REDIS_ERR;
	}
	
    return REDIS_OK;
}

static int redisSetTcpNoDelay(redisContext *c, int fd) {
    SOCKET s = fd;
	BOOL yes = TRUE;
	
    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(BOOL)) == SOCKET_ERROR) {
        __redisSetError(c,REDIS_ERR_IO,
            sdscatprintf(sdsempty(), "setsockopt(TCP_NODELAY): %s", strerror(WSAGetLastError())));
        return REDIS_ERR;
    }

    return REDIS_OK;
}

int redisContextConnectTcp(redisContext *c, const char *addr, int port) {
    int s;
    int blocking = (c->flags & REDIS_BLOCK);
    struct sockaddr_in sa;

    if ((s = redisCreateSocket(c,AF_INET)) == REDIS_ERR)
        return REDIS_ERR;
    if (!blocking && redisSetNonBlock(c,s) == REDIS_ERR)
        return REDIS_ERR;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_aton(addr, &sa.sin_addr) == 0) {
        struct hostent *he;

        he = gethostbyname(addr);
        if (he == NULL) {
            __redisSetError(c,REDIS_ERR_OTHER,
                sdscatprintf(sdsempty(),"Can't resolve: %s",addr));
            closesocket(c->fd);
			WSACleanup();
            return REDIS_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }

    if (connect(s, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        if (WSAGetLastError() == WSAEINPROGRESS && !blocking) {
            /* This is ok. */
        } else {
            __redisSetError(c,REDIS_ERR_IO,NULL);
            closesocket(c->fd);
			WSACleanup();
            return REDIS_ERR;
        }
    }

    if (redisSetTcpNoDelay(c,s) != REDIS_OK) {
        closesocket(c->fd);
		WSACleanup();
        return REDIS_ERR;
    }

    c->fd = s;
    c->flags |= REDIS_CONNECTED;
    return REDIS_OK;
}

int redisContextConnectUnix(redisContext *c, const char *path) {
    __redisSetError(c,REDIS_ERR_IO,"redisContextConnectUnix() not supported with Windows");
	closesocket(c->fd);
	WSACleanup();
	return REDIS_ERR;
}

int redisContextSetTimeout(redisContext *c, struct timeval tv) {
	int timeout = tv.tv_usec/1000;
	if (setsockopt(c->fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(int)) != 0) {
        __redisSetError(c,REDIS_ERR_IO,
            sdscatprintf(sdsempty(), "setsockopt(SO_RCVTIMEO): %s", strerror(errno)));
        return REDIS_ERR;
    }
    if (setsockopt(c->fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(int)) != 0) {
        __redisSetError(c,REDIS_ERR_IO,
            sdscatprintf(sdsempty(), "setsockopt(SO_SNDTIMEO): %s", strerror(errno)));
        return REDIS_ERR;
    }
    return REDIS_OK;
}

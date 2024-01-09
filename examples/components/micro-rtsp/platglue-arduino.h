#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

typedef WiFiClient *SOCKET;
typedef WiFiUDP *UDPSOCKET;
typedef IPAddress IPADDRESS; // On linux use uint32_t in network byte order (per getpeername)
typedef uint16_t IPPORT; // on linux use network byte order

#define NULLSOCKET NULL

inline void tcpclosesocket(SOCKET s) {
    DEBUG_PRINT("closing TCP socket\n");

    if(s) {
        s->stop();
        // delete s; TDP WiFiClients are never on the heap in arduino land
    }
}

#define getRandom() random(0, INT_MAX)

inline void socketpeeraddr(SOCKET s, IPADDRESS *addr, IPPORT *port) {
    *addr = s->remoteIP();
    *port = s->remotePort();
}

inline void udpsocketclose(UDPSOCKET s) {
    DEBUG_PRINT("closing UDP socket\n");
    if(s) {
        s->stop();
        delete s;
    }
}

inline UDPSOCKET udpsocketcreate(unsigned short portNum)
{
    UDPSOCKET s = new WiFiUDP();

    if(!s->begin(portNum)) {
        ERROR_PRINT("Can't bind port %d\n", portNum);
        delete s;
        return NULL;
    }

    return s;
}

// TCP sending
inline ssize_t tcpsocketsend(SOCKET sockfd, const void *buf, size_t len)
{
    ssize_t sent = sockfd->write((uint8_t *) buf, len);

    return sent;
}

inline ssize_t udpsocketsend(UDPSOCKET sockfd, const void *buf, size_t len,
                             IPADDRESS destaddr, IPPORT destport)
{
    sockfd->beginPacket(destaddr, destport);
    sockfd->write((const uint8_t *)  buf, len);
    if(!sockfd->endPacket())
    {
        yield(); //give lwip time to free some buffers
        sockfd->beginPacket(destaddr, destport);
        sockfd->write((const uint8_t *)  buf, len);
        if(!sockfd->endPacket())
        {
            ERROR_PRINT("error sending udp packet\n");
        }
    }

    return len;
}

/**
   Read from a socket with a timeout.

   Return 0=socket was closed by client, -1=timeout, >0 number of bytes read
 */
inline int tcpsocketread(SOCKET sock, char *buf, size_t buflen, int timeoutmsec)
{
    if(!sock->connected()) {
        DEBUG_PRINT("client has closed the socket\n");
        return 0;
    }

    int numAvail = sock->available();
    if(numAvail == 0 && timeoutmsec != 0) {
        // sleep and hope for more
        delay(timeoutmsec);
        numAvail = sock->available();
    }

    if(numAvail == 0) {
        DEBUG_PRINT("timeout on read\n");
        return -1;
    }
    else {
        // int numRead = sock->readBytesUntil('\n', buf, buflen);
        int numRead = sock->readBytes(buf, buflen);
        DEBUG_PRINT("bytes avail %d, read %d: %s", numAvail, numRead, buf);
        return numRead;
    }
}

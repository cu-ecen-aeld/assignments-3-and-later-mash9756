/**
 *  @file       aesdsocket.c
 *  @author     Mark Sherman
 *  @version    1.0
 *  @date       02/19/2023
 * 
 *  @brief      
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <syslog.h>
#include <errno.h>

/* port string for socket setup */
#define PORT            ("9000")
/* arbitrary max allows pending connections allowed before socket refuses */
#define BACKLOG         (5)
/* max receive buffer size in bytes */
#define MAX_PACKET_SIZE (64)

/**
 * @name    cleanup
 * @brief   closes socket fd's, as well as output file. Free's malloc'd socket address info
 * 
 * @param   fd1     -   socket fd
 * @param   fd2     -   socket fd
 * @param   file    -   output file
 * @param   addr    -   socket address info
 *  
 * @return  VOID
*/
void cleanup(int fd1, int fd2, FILE *file, struct addrinfo *addr)
{
    close(fd1);
    close(fd2);
    fclose(file);
    freeaddrinfo(addr);
}

int main(int argc, char *argv[])
{
    printf("\n\nAESD Socket\n\n");
/* open log for debug and error messages */
    openlog(NULL, 0, LOG_USER);
 
    printf("\nfile setup\n");
/* setup file to write results into */
    FILE *fptr;
    fptr = fopen("/var/tmp/aesdsocketdata", "w");

    printf("\nsocket address setup\n");
/* setup to hold socket address info from getaddrinfo */
    struct addrinfo *addrRes;
    int socketFD = 0;

    printf("\ngetaddrinfo setup\n");
/* setup addrinfo for socket */
    struct addrinfo hints;
    /* clear struct memory space */
    memset(&hints, 0, sizeof(struct addrinfo));
    /* set addrinfo defaults */
    hints.ai_flags     = AI_PASSIVE;
    hints.ai_family    = AF_INET;
    hints.ai_socktype  = SOCK_STREAM;

    printf("\nclient setup\n");
/* client info for connections */
    struct sockaddr clientAddr;
    /* size stored for accept call */
    socklen_t clientSize = sizeof(clientAddr);
    int clientFD = 0;
    /* received data buffer */
    char recvBuf[MAX_PACKET_SIZE];

    printf("\ngetaddrinfo\n");
/* returns malloc'd socket addrinfo in final parameter*/
    int res = getaddrinfo(NULL, PORT, &hints, &addrRes);
    if(res != 0)
    {
        syslog(LOG_ERR, "getaddrinfo failed, error code %d", res);
        cleanup(socketFD, clientFD, fptr, addrRes);
        return -1;
    }

    printf("\nsocket\n");
/* create socket with IPv4 addressing, streaming type, with default options */
    socketFD = socket(addrRes->ai_family, addrRes->ai_socktype, addrRes->ai_protocol);
    if(socketFD == -1)
    {
        syslog(LOG_ERR, "socket failed to create new socket fd");
        cleanup(socketFD, clientFD, fptr, addrRes);
        return -1;
    }

    printf("\nbind\n");
/* bind created socket ID to generated socket address */
    res = bind(socketFD, addrRes->ai_addr, sizeof(struct sockaddr));
    if(res != 0)
    {
        syslog(LOG_ERR, "bind failed, see errno for details");
        cleanup(socketFD, clientFD, fptr, addrRes);
        return -1;
    }
    
    printf("\nlisten\n");
/* listen for connections on created socket */
    res = listen(socketFD, BACKLOG);
    if(res != 0)
    {
        syslog(LOG_ERR, "listen failed, see errno for details");
        cleanup(socketFD, clientFD, fptr, addrRes);
        return -1;
    }

    printf("\naccept\n");
/* accecpt incoming connection and save client info */
    res = accept(socketFD, &clientAddr, &clientSize);
    if(res == -1)
    {
        syslog(LOG_ERR, "accept failed, see errno for details");
        cleanup(socketFD, clientFD, fptr, addrRes);
        return -1;
    }
    /* save connected client's FD for send/recv later */
    clientFD = res;
    /* log accepted connection and client's IP address */
    syslog(LOG_DEBUG, "Accepted connection from %s", clientAddr.sa_data);

    printf("\nrecv\n");
/* receive packets from connected client */
    res = recv(clientFD, recvBuf, sizeof(recvBuf), 0);
    if(res == -1)
    {
        syslog(LOG_ERR, "recv failed, see errno for details");
        cleanup(socketFD, clientFD, fptr, addrRes);
        return -1;
    }
    syslog(LOG_DEBUG, "%d bytes received from %s", res, clientAddr.sa_data);

    printf("\nprint to file\n");
    //fprintf(fptr, "\n%d bytes received from %s", res, clientAddr.sa_data);
    fprintf(fptr, "%s", recvBuf);

    printf("\ncleanup and close\n");
    cleanup(socketFD, clientFD, fptr, addrRes);
    return 0;
}
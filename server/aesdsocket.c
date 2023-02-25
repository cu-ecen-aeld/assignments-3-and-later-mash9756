/**
 *  @file       aesdsocket.c
 *  @author     Mark Sherman
 *  @version    1.1
 *  @date       02/25/2023
 * 
 *  @brief      
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sys/wait.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <syslog.h>
#include <errno.h>

#include <linux/fs.h>
#include <fcntl.h>

/* port string for socket setup */
#define PORT            ("9000")
/* arbitrary max allows pending connections allowed before socket refuses */
#define BACKLOG         (10)
/* max receive buffer size in bytes */
#define MAX_BUF_SIZE    (65535)
/* Output file path definition */
#define OUTPUT_FILE     ("/var/tmp/aesdsocketdata")

/* global for signal handler access */
bool cleanExit  = false;
int socketFD    = 0;

/**
 * @name    get_in_addr
 * 
 * @brief   extracts IPv4 or IPv6 IP address from passed socket  
 *          taken from https://beej.us/guide/bgnet/html/#a-simple-stream-server
 * 
 * @param   sa  - socket
 * 
 * @return  VOID    
*/
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
        return &(((struct sockaddr_in*)sa)->sin_addr);

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void signal_handler(int sig_num)
{
    if(sig_num == SIGINT || sig_num == SIGTERM)
    {
        syslog(LOG_DEBUG, "\nSignal received, starting clean exit...\n");
        shutdown(socketFD, SHUT_RDWR);
        cleanExit = true;
    }
}

int main(int argc, char *argv[])
{
    printf("\n\nAESD Socket\n\n");

    FILE *fptr;

/* -------------------------- setup signal handlers -------------------------- */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    sleep(1);

/* -------------------------- check if daemon option selected -------------------------- */
    pid_t daemonPid = 0;
    pid_t daemonSid = 0;
    bool isDaemon   = false;
    /* argc is always at least 1, and argv[0] is program name */
    if(argc >= 2 && strcmp(argv[1], "-d") == 0)
        isDaemon = true;

/* -------------------------- open log for debug and error messages -------------------------- */
    openlog(NULL, 0, LOG_USER);

/* -------------------------- setup addrinfo for socket -------------------------- */
    struct addrinfo hints;
    /* clear struct memory space */
    memset(&hints, 0, sizeof(hints));
    /* set addrinfo defaults */
    hints.ai_flags     = AI_PASSIVE;
    hints.ai_family    = AF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM;

/* -------------------------- client info for connections -------------------------- */
    struct sockaddr_storage clientAddr;
    /* size stored for accept call */
    socklen_t clientSize;
    int clientFD = 0;
    /* received data buffer */
    char buf[MAX_BUF_SIZE];
    long recvIndex = 0;
    char clientIP[INET6_ADDRSTRLEN];

/* -------------------------- recv setup -------------------------- */
    bool recvDone   = false;
    long recvBytes   = 0;
    long totalBytes  = 0;
    char recvBuf[MAX_BUF_SIZE];

/* -------------------------- setup to hold socket address info from getaddrinfo -------------------------- */
    struct addrinfo *addrRes;
    int yes = 1;
    long i = 0;

/* -------------------------- returns malloc'd socket addrinfo in final parameter -------------------------- */
    int res = getaddrinfo(NULL, PORT, &hints, &addrRes);
    if(res != 0)
    {
        syslog(LOG_ERR, "getaddrinfo failed, error code %d", res);
        printf("getaddrinfo failed, error code %d", res);
        freeaddrinfo(addrRes);
        exit(-1);
    }

/* -------------------------- create socket with IPv4 addressing, streaming type, with default options -------------------------- */
    socketFD = socket(addrRes->ai_family, addrRes->ai_socktype, addrRes->ai_protocol);
    if(socketFD == -1)
    {
        syslog(LOG_ERR, "socket failed to create new socket fd");
        printf("socket failed to create new socket fd");
        freeaddrinfo(addrRes);
        exit(-1);
    }

/* -------------------------- use setsockopt to allow for socket address reuse -------------------------- */
    if(setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        syslog(LOG_ERR, "setsockopt failed to config address reuse");
        printf("setsockopt failed to config address reuse");
        close(socketFD);
        exit(-1);
    }


/* -------------------------- bind created socket ID to generated socket address -------------------------- */
    res = bind(socketFD, addrRes->ai_addr, addrRes->ai_addrlen);
    if(res != 0)
    {
        syslog(LOG_ERR, "bind failed, see errno for details");
        printf("bind failed, errno = %d", errno);
        close(socketFD);
        freeaddrinfo(addrRes);
        exit(-1);
    }
    
    /* free stuct, no longer needed after bind step */
    freeaddrinfo(addrRes);

    /* reference daemon creation process from https://netzmafia.ee.hm.edu/skripten/unix/linux-daemon-howto.html */
    if(isDaemon == true)
    {
        printf("\nDaemon Mode: %d\n", isDaemon);
        daemonPid = fork();
        if(daemonPid == -1)
        {
            syslog(LOG_ERR, "fork failed, see errno for details");
            printf("fork failed, errno = %d", errno);
            close(socketFD);
            return -1;
        }
        /* terminate parent process */
        if(daemonPid != 0)
            exit(0);

        /* allows daemon to access files it creates */
        umask(0);

        /* create new session for daemon to run in */
        daemonSid = setsid();
        if(daemonSid == -1)
        {
            syslog(LOG_ERR, "setsid failed, see errno for details");
            printf("setsid failed, errno = %d", errno);
            close(socketFD);
            exit(-1);
        }

        /* change working directory to root */
        int dirRes = chdir("/");
        if(dirRes == -1)
        {
            syslog(LOG_ERR, "chdir failed, see errno for details");
            printf("chdir failed, errno = %d", errno);
            close(socketFD);
            exit(-1);
        }
        
        /* close std fd's as daemon cannot access terminal */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }
/* -------------------------- listen for connections on created socket -------------------------- */
    res = listen(socketFD, BACKLOG);
    if(res != 0)
    {
        syslog(LOG_ERR, "listen failed, see errno for details");
        printf("listen failed, see errno for details");
        close(socketFD);
        exit(-1);
    }

/* -------------------------- Loop for accepting connections and receiving packets -------------------------- */
    while(!cleanExit)
    {
    /* accept incoming connection and save client info */
        clientSize = sizeof(clientAddr);
        res = accept(socketFD, (struct sockaddr *)&clientAddr, &clientSize);
        if(res == -1)
        {
            syslog(LOG_ERR, "accept failed, see errno for details");
            continue;
        }
    /* save connected client's FD for send/recv later */
        clientFD = res;
    /* log accepted connection and client's IP address */
        inet_ntop(clientAddr.ss_family, get_in_addr((struct sockaddr *)&clientAddr), clientIP, sizeof(clientIP));
        syslog(LOG_DEBUG, "Accepted connection from %s", clientIP);
        printf("\nAccepted connection from %s", clientIP);

    /* clear last recv data */
        recvDone    = false;
        totalBytes  = 0;
        recvIndex   = 0;

        while(!recvDone)
        {
        /* receive packet from connected client */
            recvBytes = recv(clientFD, recvBuf, MAX_BUF_SIZE-1, 0);
            if(recvBytes == -1)
            {
                syslog(LOG_ERR, "recv failed, see errno for details");
                syslog(LOG_DEBUG, "closing connection from %s", clientIP);
                close(clientFD);
                exit(-1);
            }

        /* track total bytes received */
            totalBytes += recvBytes;
        /* load recv buffer data into second buffer for file write */
            for(i = 0; i < recvBytes; i++)
                buf[recvIndex + i] = recvBuf[i];
        
        /* track buffer index from previous write */
            recvIndex += recvBytes;
        /* check for end of packet, \n terminated */
            if(recvBuf[recvIndex - 1] == '\n')
                recvDone = true;
        }

        printf("%ld bytes received from %s", recvBytes, clientIP);
        syslog(LOG_DEBUG, "%ld bytes received from %s", recvBytes, clientIP);

    /* null terminate buffer to write to file as string */
        buf[totalBytes] = '\0';
        totalBytes += 1;

    /* setup file to write results into */
        fptr = fopen(OUTPUT_FILE, "a+");
        fputs(buf, fptr);

    /* clear buf for file readback */
        memset(buf, 0, MAX_BUF_SIZE * sizeof(buf[0]));
    /* readback and send contents of file back */
        fseek(fptr, 0, SEEK_SET);
        while(fgets(buf, MAX_BUF_SIZE, fptr) != NULL)
        {
            res = send(clientFD, buf, strlen(buf), 0);
            if(res == -1)
            {
                syslog(LOG_ERR, "send failed, see errno for details");
            }
        }

        syslog(LOG_DEBUG, "closing connection from %s", clientIP);
        fclose(fptr);
        close(clientFD);
        printf("\nClosed connection from %s\n", clientIP);
    }

    while(cleanExit)
    {
        printf("\nClean exit interrupt received, closing...\n");
        syslog(LOG_DEBUG, "closing connection from %s", clientIP);
        close(socketFD);
        close(clientFD);
        remove(OUTPUT_FILE);
        printf("\nCleanup Complete.\n");
        exit(0);
    }
}
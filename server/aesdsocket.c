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

#include <pthread.h>
#include "queue.h"

/* include ioctl header for struct and function defs */
#include "aesd_ioctl.h"

/* Assignment 8 Flag */
#define USE_AESD_CHAR_DEVICE    (1)

/* port string for socket setup */
#define PORT            ("9000")
/* arbitrary max allows pending connections allowed before socket refuses */
#define BACKLOG         (10)
/* max receive buffer size in bytes */
#define MAX_BUF_SIZE    (65535)
/* additional byte for null-ternminator */
#define NULL_TERM_BYTE  (1)
/* timestamp interval, every 10s */
#define TIMESTAMP_INTV  (10)
/* max length of timestamp string */
#define TS_STR_LEN      (64)
/* Output file path definition */
#ifdef USE_AESD_CHAR_DEVICE
    #define OUTPUT_FILE     ("/dev/aesdchar")
#else
    #define OUTPUT_FILE     ("/var/tmp/aesdsocketdata")
#endif

#define CHAR_TO_INT         (48)

#define IOCTL_STRING_SIZE   (20)

const char IOCTL_STRING[IOCTL_STRING_SIZE] = "AESDCHAR_IOCSEEKTO:";

/* global for signal handler access */
bool cleanExit      = false;
int socketFD        = 0;
long total_len      = 0;
long total_pkt_cnt  = 0;

int fd = 0;

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

/** TODO
 *  @name
 *  @brief
 * 
 *  @param
 * 
 *  @return
*/
void signal_handler(int sig_num)
{
    if(sig_num == SIGINT || sig_num == SIGTERM)
    {
        syslog(LOG_DEBUG, "\nSignal received, starting clean exit...\n");
        shutdown(socketFD, SHUT_RDWR);
        cleanExit = true;
    }
    else
    {
        syslog(LOG_ERR, "\nUnknown signal received\n");
        printf("\nUnknown signal received\n");
        exit(-1);
    }
}

/** TODO
 *  @name
 *  @brief
 * 
 *  @param
 * 
 *  @return
*/
struct thread_data
{
    /* thread ID */
    pthread_t thread_ID;

    /* mutex for thread */
    pthread_mutex_t *mutex;

    /* thread connection FD */
    int clientFD;

    /* thread completion status */
    bool thread_complete_success;

    /* linked list */
    SLIST_ENTRY(thread_data) entries;
};

/**
 *  @name
 *  @brief  process received packet, write to log file
 *          read back from file, send back to client
 * 
 * 
*/
static int process_recv_pkt(char **pkt, int clientFD, pthread_mutex_t *mutex, long len)
{
    if(pkt == NULL)
    {
        syslog(LOG_ERR, "Passed Packet returned NULL pointer");
        return -1;
    }

    printf("\n\t\tOpening device...");
    fd = open(OUTPUT_FILE, O_RDWR|O_CREAT|O_APPEND, S_IRWXU|S_IRWXG|S_IRWXO);

    printf("\n\t\t%ld bytes received: %s", len, *pkt);

/* ioctl seekto struct */
    struct aesd_seekto seekto;
    char wr_cmd;
    char wr_cmd_ofs;

    char find_seek[IOCTL_STRING_SIZE];
    memcpy(find_seek, *pkt, IOCTL_STRING_SIZE - 1);
    find_seek[IOCTL_STRING_SIZE] = '\0';

    if(strcmp(find_seek, IOCTL_STRING) == 0)
    {
        printf("\n\t\tIOCTL Command Received. Parsing values...");
        memcpy(find_seek, *pkt, len);

        wr_cmd = find_seek[IOCTL_STRING_SIZE - 1];
        wr_cmd_ofs = find_seek[IOCTL_STRING_SIZE + 1];
        printf("\n\t\twr_cmd: %d", wr_cmd);
        printf("\n\t\twr_cmd_ofs: %d", wr_cmd_ofs);

        /* get X and Y, bytes 19 and 21 */
        printf("\n\t\tconverting to ints...");
        seekto.write_cmd = wr_cmd - CHAR_TO_INT;
        seekto.write_cmd_offset = wr_cmd_ofs - CHAR_TO_INT;
        printf("\n\t\twrite_cmd: %d", seekto.write_cmd);
        printf("\n\t\twrite_cmd_offset: %d", seekto.write_cmd_offset);

        /* call ioctl command */
        ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto);
        //close(fd);
        goto exit;
    }

/* index for byte-by-byte readback */
    long i = 0;
/* memory to hold full readback contents */
    char *readback  = (char *)malloc(total_len);

    //printf("\nlocking\n");
    if(pthread_mutex_lock(mutex))
    {
        printf("\nfailed to lock");
        free(readback);
        //free(find_seek);
        close(fd);
        return -1;
    }
    
    //printf("\nwriting\n");
    write(fd, *pkt, len);
    //printf("\nunlocking\n");
    pthread_mutex_unlock(mutex);

    //printf("\nSeeking to start of file\n");
/* seek to file start for readback */
    lseek(fd, 0, SEEK_SET);
/* readback one byte at a time until we reach EOF */
    printf("\n\t\tReading back file and sending...");
    while(read(fd, &readback[i++], 1) != 0);

    //printf("\nsending read back to client\n");
    if(send(clientFD, readback, total_len, 0) < 0)
    {
        printf("\nsend failure\n");
        if(errno == EINTR)
        {
            free(readback);
            //free(find_seek);
            close(fd);
            return -1;
        }    
        syslog(LOG_ERR, "send failed, see errno for details");
    }

    printf("\n\t\tsend complete!\n");
    free(readback);
exit:
    if(pkt != NULL)
    {
        //printf("\nfreeing pkt\n");
        free(*pkt);
        *pkt = NULL;
    }
    //close(fd);
    return 0;
}

/**
 *  @name   timer_func
 *  @brief  timestamp thread function, writes current date and time
 *          to output file evert 10s
 * 
 *  @param  thread_param    thread parameters
 * 
 *  @return VOID
*/
void *timer_func(void *thread_param)
{
    if(thread_param == NULL)
        return NULL;

    int res = 0;

    struct thread_data *timerThread = thread_param;

    printf("\nEntered Timer Thread %ld", timerThread->thread_ID);

    FILE *fptr = NULL;

    time_t rawTime;
    struct tm *localTime;
    struct timespec ts;
    char tsStr[TS_STR_LEN];

    while(!cleanExit)
    {
        res = clock_gettime(CLOCK_MONOTONIC, &ts);
        if(res == -1)
        {
            /* TODO: gettime failed */
            syslog(LOG_ERR, "gettime failed");
            break;
        }

        ts.tv_sec += TIMESTAMP_INTV;
        res = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
        if(res != 0)
        {
            /* TODO: clock_nanosleep failed */
            syslog(LOG_ERR, "clock_nanosleep failed");
            break;
        }
        /* get raw time since epoch */
        rawTime     = time(NULL);
        /* convert raw time to local time */
        localTime   = localtime(&rawTime);
        /* write formatted timestamp into tsStr, %c is full date and time */
        strftime(tsStr, sizeof(tsStr), "timestamp: %c\n", localTime);


        fptr = fopen(OUTPUT_FILE, "a+");
        printf("write timestamp to file\n");
        /* lock mutex for file writing */
        pthread_mutex_lock(timerThread->mutex);
        fputs(tsStr, fptr);
        pthread_mutex_unlock(timerThread->mutex);

        fclose(fptr);
    }
    
    timerThread->thread_complete_success = 1;
    return NULL;
}

/** TODO
 *  @name   client_func
 *  @brief  receives packets from client and passes to process
 * 
 *  @param  thread_param    thread parameters
 * 
 *  @return VOID
*/
void *client_func(void *thread_param)
{
    if(thread_param == NULL)
        return NULL;
        
/* -------------------------- recv setup -------------------------- */
    long recvLen    = 0;
    long totalLen   = 0;
    long currLen    = 0;
    int res         = 0;

    char recvBuf[MAX_BUF_SIZE];
    char *recvPkt = NULL;
    
    struct thread_data *clientData = thread_param;

    printf("\nEntered Client Thread %ld", clientData->thread_ID);

    //printf("\n\tOpening device...");
    //fd = open(OUTPUT_FILE, O_RDWR|O_CREAT|O_APPEND, S_IRWXU|S_IRWXG|S_IRWXO);

/* receive packet from connected client */
    //printf("\nrecv\n");
    while((recvLen = recv(clientData->clientFD, recvBuf, MAX_BUF_SIZE, 0)))
    {
        if(recvLen == -1)
        {
            if(errno == EINTR)
                continue;
            else
            {
                syslog(LOG_ERR, "recv failed, see errno for details");
                printf("\ncleanup and close client thread");
                close(clientData->clientFD);
                clientData->clientFD = -1;
                clientData->thread_complete_success = 1;
            }
        }

        if(totalLen - currLen < recvLen)
        {
            /* add extra byte for null-term string */
            totalLen += (recvLen + NULL_TERM_BYTE);
            /* realloc for additional received bytes (extend) */
            recvPkt = (char *) realloc(recvPkt, totalLen);
            if(recvPkt == NULL)
            {
                syslog(LOG_ERR, "Failed to realloc memory for write buffer");
                printf("\nrealloc for recvPkt failed\n");
                printf("\ncleanup and close client thread");
                close(clientData->clientFD);
                clientData->clientFD = -1;
                clientData->thread_complete_success = 1;
            }
            /* clear newly allocated extended memory */
            //printf("\nclearing realloced memory\n");
            memset(recvPkt + currLen , 0, totalLen - currLen);
        }

        /* load recv buffer data into second buffer for file write */
        memcpy(recvPkt + currLen, recvBuf, recvLen);
        //printf("\nReceived: %s", recvBuf);
        currLen += recvLen;

        total_len += recvLen;

        total_pkt_cnt++;

        //printf("\nprocessing recvPkt\n");
        res = process_recv_pkt(&recvPkt, clientData->clientFD, clientData->mutex, recvLen);
        if(res == -1)
            break;
        printf("\n\trecvPkt processed.");
    }

    /* free if packet processing failed */
    if(recvPkt != NULL)
    {
        //printf("\nfreeing recvPkt\n");
        free(recvPkt);
    }
    
    printf("\nclosing file");
    close(fd);
    printf("\ncleanup and close client thread\n");
    close(clientData->clientFD);

    clientData->clientFD = -1;
    clientData->thread_complete_success = 1;
    return NULL;
}

int main(int argc, char *argv[])
{
    printf("\n\nAESD Socket\n\n");
    
    struct thread_data *threadSetup = NULL;
    struct thread_data *temp = NULL;
    pthread_mutex_t mutex;

    pthread_mutex_init(&mutex, NULL);
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

/* -------------------------- setup linked list -------------------------- */
    SLIST_HEAD(head_s, thread_data) head;
    SLIST_INIT(&head);

/* -------------------------- setup addrinfo for socket -------------------------- */
    //printf("\ngetaddrinfo setup\n");
    struct addrinfo hints;
    /* clear struct memory space */
    memset(&hints, 0, sizeof(hints));
    /* set addrinfo defaults */
    hints.ai_flags     = AI_PASSIVE;
    hints.ai_family    = AF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM;

/* -------------------------- client info for connections -------------------------- */
    //printf("\nclient setup\n");
    struct sockaddr_storage clientAddr;
    /* size stored for accept call */
    socklen_t clientSize;
    int clientFD = 0;
    char clientIP[INET6_ADDRSTRLEN];

/* -------------------------- setup to hold socket address info from getaddrinfo -------------------------- */
    //printf("\nsocket address setup\n");
    struct addrinfo *addrRes;
    int yes = 1;

    //printf("\ngetaddrinfo\n");
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
    //printf("\nsocket\n");
    socketFD = socket(addrRes->ai_family, addrRes->ai_socktype, addrRes->ai_protocol);
    if(socketFD == -1)
    {
        syslog(LOG_ERR, "socket failed to create new socket fd");
        printf("socket failed to create new socket fd");
        freeaddrinfo(addrRes);
        exit(-1);
    }

/* -------------------------- use setsockopt to allow for socket address reuse -------------------------- */
    //printf("\nsetsockopt\n");
    if(setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        syslog(LOG_ERR, "setsockopt failed to config address reuse");
        printf("setsockopt failed to config address reuse");
        close(socketFD);
        exit(-1);
    }

/* -------------------------- bind created socket ID to generated socket address -------------------------- */
    printf("\nbind\n");
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

    //printf("\nlisten\n");
/* -------------------------- listen for connections on created socket -------------------------- */
    res = listen(socketFD, BACKLOG);
    if(res != 0)
    {
        syslog(LOG_ERR, "listen failed, see errno for details");
        printf("listen failed, see errno for details");
        close(socketFD);
        exit(-1);
    }

#ifndef USE_AESD_CHAR_DEVICE
/* create timer thread for timestamping output file */
    threadSetup = (struct thread_data *) malloc(sizeof(struct thread_data));
    if(threadSetup == NULL)
    {
        /* TODO: failed to allocate memory */
        shutdown(socketFD, SHUT_RDWR);
        close(socketFD);
        exit(-1);
    }
    threadSetup->mutex = &mutex;
    threadSetup->thread_complete_success = 0;
    res = pthread_create(&threadSetup->thread_ID, NULL, timer_func, (void *)threadSetup);
    if(res != 0)
    {
        /* TODO: pthread create failed to start thread */
        printf("\nfreeing threadSetup on pthread_create failure (timer)\n");
        free(threadSetup);
        shutdown(socketFD, SHUT_RDWR);
        close(socketFD);
        exit(-1);
    }
/* add thread to linked list after creation */
    SLIST_INSERT_HEAD(&head, threadSetup, entries);
#endif

    //printf("\n\tOpening device...");
    //fd = open(OUTPUT_FILE, O_RDWR|O_CREAT|O_APPEND, S_IRWXU|S_IRWXG|S_IRWXO);

/* -------------------------- Loop for accepting connections and receiving packets -------------------------- */
    while(!cleanExit)
    {
    /* accept incoming connection and save client info */
        //printf("\naccept\n");
        clientSize = sizeof(clientAddr);
        res = accept(socketFD, (struct sockaddr *)&clientAddr, &clientSize);
        if(res == -1)
        {
            //(errno == EINTR)
                 //continue;
            //syslog(LOG_ERR, "accept failed, see errno for details");
            printf("\naccept failed, cleaning up threads");
            goto cleanup_threads;
        }
    /* save connected client's FD for send/recv later */
        clientFD = res;
    /* log accepted connection and client's IP address */
        inet_ntop(clientAddr.ss_family, get_in_addr((struct sockaddr *)&clientAddr), clientIP, sizeof(clientIP));
        syslog(LOG_DEBUG, "Accepted connection from %s", clientIP);
        printf("\nAccepted connection from %s", clientIP);


/************ spawn thread here ***************/

        threadSetup = (struct thread_data *) malloc(sizeof(struct thread_data));
        if(threadSetup == NULL)
        {
            /* TODO: failed to allocate memory */
            shutdown(socketFD, SHUT_RDWR);
            close(socketFD);
            close(fd);
            exit(-1);
        }
        
        threadSetup->clientFD   = clientFD;
        threadSetup->mutex      = &mutex;
        threadSetup->thread_complete_success = 0;
        res = pthread_create(&threadSetup->thread_ID, NULL, client_func, (void *)threadSetup);
        if(res != 0)
        {
            /* TODO: pthread create failed to start thread */
            printf("\nfreeing threadSetup on pthread_create failure\n");
            free(threadSetup);
            shutdown(socketFD, SHUT_RDWR);
            close(socketFD);
            close(fd);
            exit(-1);
        }

        /* add thread to linked list after creation */
        SLIST_INSERT_HEAD(&head, threadSetup, entries);
        //printf("\nadded thread to linked list\n");

cleanup_threads:
        /* remove all thread from linked-list, if it is completed */
        threadSetup = NULL;
        SLIST_FOREACH_SAFE(threadSetup, &head, entries, temp) 
        {
            if (threadSetup->thread_complete_success) 
            {
                pthread_join(threadSetup->thread_ID, NULL);
                SLIST_REMOVE(&head, threadSetup, thread_data, entries);
                printf("\nfreeing threadSetup in cleanup_threads\n");
                free(threadSetup);
                printf("\tfree complete\n");
            }
        }
    }

    printf("\nClean exit interrupt received, closing...");
    syslog(LOG_DEBUG, "closing connection from %s", clientIP);
    close(socketFD);
    close(clientFD);
    syslog(LOG_DEBUG, "closing device");
    close(fd);
    remove(OUTPUT_FILE);

    /* cleanup linked list */
    threadSetup = NULL;
    while(!SLIST_EMPTY(&head))
    {
        threadSetup = SLIST_FIRST(&head);
        SLIST_REMOVE_HEAD(&head, entries);
        printf("\nfreeing threadSetup in cleanExit\n");
        free(threadSetup);
    }
    SLIST_INIT(&head);

    printf("\nCleanup Complete.\n");
    return res;
}
//#include "aesdsocket.h" Does NOT yet exist
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/queue.h>
#include <time.h>

//#define _BSD_SOURCE
#define	SLIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = SLIST_FIRST((head));				\
	    (var) && ((tvar) = SLIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define UNUSED(x) (void)(x)
int error = -1;

void INTSignalHandler(int sig);
void TERMSignalHandler(int sig);

bool bSIGINT = false;
bool bSIGTERM = false;
int nSOCKET;
FILE *fout;
int curLoc = 0;
pthread_mutex_t mutex;

struct thread_info {
    pthread_t thread_id;
    SLIST_ENTRY(thread_info) entries;
    bool bThreadComplete;
    int client;
};

SLIST_HEAD(thread_list, thread_info) threads = SLIST_HEAD_INITIALIZER(threads);

void  INTSignalHandler(int sig){
    UNUSED(sig);
    syslog(LOG_DEBUG, "\nCaught signal, exiting\n");
    printf("\nCaught signal, exiting\n");
    
    // Mask Handlers until exit to prevent reentrancy
    signal(SIGINT, SIG_IGN); // Ignore SIGINT signal (mask)
    signal(SIGTERM, SIG_IGN); // Ignore SIGTERM signal (mask)
    shutdown(nSOCKET, SHUT_RDWR); // closing the listening socket
    pthread_mutex_destroy(&mutex);
    fclose(fout);
    bSIGINT = true;
}

void TERMSignalHandler(int sig){
    UNUSED(sig);
    syslog(LOG_DEBUG, "\nCaught signal, exiting\n");
    printf("\nCaught signal, exiting\n");

    // Mask Handlers until exit to prevent reentrancy
    signal(SIGINT, SIG_IGN); // Ignore SIGINT signal (mask)
    signal(SIGTERM, SIG_IGN); // Ignore SIGTERM signal (mask)
    shutdown(nSOCKET, SHUT_RDWR); // closing the listening socket
    pthread_mutex_destroy(&mutex);
    fclose(fout);
    bSIGTERM = true;
}

void* write_timestamp(void *arg){
    UNUSED(arg);
    time_t current_time;
    struct tm *time_info;
    char timestamp[128];

    sleep(10);
    while( !(bSIGINT || bSIGTERM) ){
        time(&current_time);
        time_info = localtime(&current_time);
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %T %z\n", time_info);
        
        pthread_mutex_lock(&mutex);
        fseek(fout, curLoc, SEEK_SET);
        fprintf(fout, "%s\n", timestamp);
        curLoc = (int)ftell(fout)-1;
        //fflush(fout);
        pthread_mutex_unlock(&mutex);

        sleep(10);
    }
    pthread_exit(NULL);
}

void* thread_function(void* arg){
    char *clientRecvBuffer = (char*)malloc(512);
    char *clientSendBuffer = (char*)malloc(512);
    char* ptr;
    char ch_newline = '\n', ch;
    int nrecv, ngetline, nsend;

    pthread_mutex_lock(&mutex);
    struct thread_info* threadInfo = arg;
    int client = threadInfo->client;
    pthread_mutex_unlock(&mutex);

    printf("Thread %ld has started processing client %d\n", pthread_self(), client);

    while( !(bSIGINT || bSIGTERM) ){
        //printf("------------------------\n");
        do{
            memset(clientRecvBuffer, 0, 512);
            nrecv = (int)recv(client, clientRecvBuffer, 512-1, 0);
            ptr = strchr(clientRecvBuffer, ch_newline);
            if(nrecv <= 0){ // Got error or connection closed by client
                break;
            }
            else if((nrecv == 512-1) && (NULL == ptr)){ // buffer full, but no '\n' found
                //syslog(LOG_DEBUG, "Writing %s to %s\n", clientRecvBuffer, "/var/tmp/aesdsocketdata.txt");
                //printf("Writing %s to %s\n", clientRecvBuffer, "/var/tmp/aesdsocketdata.txt");
                pthread_mutex_lock(&mutex);
                fseek(fout, curLoc, SEEK_SET);
                fprintf(fout, "%s\n", clientRecvBuffer);
                curLoc = (int)ftell(fout)-1;
                //fflush(fout);
                pthread_mutex_unlock(&mutex);
            }
        } while(NULL == ptr);

        if(nrecv <= 0){ // Got error or connection closed by client
            if(nrecv == 0){ // Connection closed
                shutdown(client, SHUT_RDWR); // closing the connected socket
                break;
            }
            else{ // An ERROR occured while recv from client
                shutdown(client, SHUT_RDWR); // closing the connected socket
                free(clientRecvBuffer);
                free(clientSendBuffer);
                syslog(LOG_ERR, "ERROR: Thread %ld aesdsocket failed to recv from client socket\n", pthread_self());
                printf("ERROR: Thread %ld aesdsocket failed to recv from client socket\n", pthread_self());
                syslog(LOG_DEBUG, "Thread %ld Closed connection from %d\n", pthread_self(), client);
                printf("Thread %ld Closed connection from %d\n", pthread_self(), client);
                pthread_exit(&error);
            }
        }
        else{ // We got some good data from a client
            // Finish writing last part of packet (or whole packet) to file
            //printf("Writing %s to %s\n", clientRecvBuffer, "/var/tmp/aesdsocketdata.txt");
            pthread_mutex_lock(&mutex);
            fseek(fout, curLoc, SEEK_SET);
            fprintf(fout, "%s\n", clientRecvBuffer);
            curLoc = (int)ftell(fout)-1;
            //fflush(fout);

            // Start reading the entire file back to the client
            fseek(fout, 0, SEEK_SET);
            do{
                memset(clientSendBuffer, 0, 512);
                ch = '\0';
                for(ngetline = 0; (ngetline<(512-1)) && (ch != '\n'); ngetline++){
                    ch = fgetc(fout); // reading the file
                    //if(ch == EOF){
                    if(feof(fout)){
                        break;
                    }
                    clientSendBuffer[ngetline] = ch;
                }
                //printf("Reading %s from %s\n", clientSendBuffer, "/var/tmp/aesdsocketdata.txt");
                if(ngetline > 1){
                    nsend = (int)send(client, clientSendBuffer, ngetline, 0); // sockfd, buf, len, flags
                }
                if(nsend == -1){
                    syslog(LOG_ERR, "ERROR: Thread %ld aesdsocket failed to send to client socket\n", pthread_self());
                    printf("ERROR: Thread %ld aesdsocket failed to send to client socket\n", pthread_self());
                    shutdown(client, SHUT_RDWR); // closing the connected socket
                    free(clientRecvBuffer);
                    free(clientSendBuffer);               
                    pthread_exit(&error);
                }
            } while(!feof(fout));
            pthread_mutex_unlock(&mutex);
        }
    }
    shutdown(client, SHUT_RDWR); // closing the connected socket
    free(clientRecvBuffer);
    free(clientSendBuffer);
    syslog(LOG_DEBUG, "Thread %ld Completed connection from %d\n", pthread_self(), client);
    printf("Thread %ld Completed connection from %d\n", pthread_self(), client);

    pthread_mutex_lock(&mutex);
    threadInfo->bThreadComplete = true;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main(int argc, char**argv){
    const char *daemonMode = argv[1];
    bool bDaemonMode = false;

    fout = fopen("/var/tmp/aesdsocketdata.txt", "w+");
    if(fout == NULL){
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    if( ((argc-1) != 0) && ((argc-1) != 1) ){
        printf("argc: %d; Only 0 or 1 arguments are acceptable\n", (argc-1));
        fclose(fout);
        return -1;
    }
    else if((argc-1) == 1){
        if(strcmp(daemonMode, "-d") == 0){
            printf("Running aesdsocket as daemon\n");
            bDaemonMode = true;
        }
        else{
            printf("ERROR: Unacceptable aesdsocket argument: %s\n", daemonMode);
            fclose(fout);
            return -1;
        }
    }
    UNUSED(argv);
    signal(SIGINT, INTSignalHandler);
    signal(SIGTERM, TERMSignalHandler);
    openlog(NULL,0,LOG_USER); // init syslog

    int nsocket, nbind, getaddr, nlisten, client;
    struct addrinfo hints, *servinfo;
    pthread_mutex_init(&mutex, NULL);

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    getaddr = getaddrinfo(NULL, "9000", &hints, &servinfo); // node, service(port), hints, res
    if(getaddr != 0){
        syslog(LOG_ERR, "ERROR: aesdsocket failed on getaddrinfo. Returned %d\n", getaddr);
        printf("ERROR: aesdsocket failed on getaddrinfo. Returned %d\n", getaddr);
        closelog();
        freeaddrinfo(servinfo);
        fclose(fout);
        pthread_mutex_destroy(&mutex);
        return -1;
    }
    //printf("servinfo->ai_flags: %d\n", servinfo->ai_flags);
    //printf("servinfo->ai_family: %d\n", servinfo->ai_family);
    //printf("servinfo->ai_socktype: %d\n", servinfo->ai_socktype);
    //printf("servinfo->ai_protocol: %d\n", servinfo->ai_protocol);
    //printf("servinfo->ai_addrlen: %d\n", (int)servinfo->ai_addrlen);
    //printf("servinfo->ai_addr->sa_family: %d\n", servinfo->ai_addr->sa_family);
    //printf("servinfo->ai_addr->sa_data: %s\n", servinfo->ai_addr->sa_data);
    //printf("servinfo->ai_canonname: %s\n", servinfo->ai_canonname);

    nsocket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol); // domain=IPv6, type=TCP, protocol=proper for type
    if(nsocket == -1){
        syslog(LOG_ERR, "ERROR: aesdsocket failed to set socket\n");
        printf("ERROR: aesdsocket failed to set socket\n");
        closelog();
        freeaddrinfo(servinfo);
        fclose(fout);
        pthread_mutex_destroy(&mutex);
        return -1;
    }
    nSOCKET = nsocket;

    nbind = bind(nsocket, servinfo->ai_addr, servinfo->ai_addrlen); // sockfd, addr, addrlen
    freeaddrinfo(servinfo);
    if(nbind != 0){
        syslog(LOG_ERR, "ERROR: aesdsocket failed to bind socket\n");
        printf("ERROR: aesdsocket failed to bind socket\n");
        closelog();
        shutdown(nsocket, SHUT_RDWR); // closing the listening socket
        fclose(fout);
        pthread_mutex_destroy(&mutex);
        return -1;
    }

    // lose the pesky "Address already in use" error message
    int yes = 1;
    if (setsockopt(nsocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
        syslog(LOG_ERR, "ERROR: aesdsocket failed to setsockopt\n");
        printf("ERROR: aesdsocket failed to setsockopt\n");
        closelog();
        shutdown(nsocket, SHUT_RDWR); // closing the listening socket
        fclose(fout);
        pthread_mutex_destroy(&mutex);
        return -1;
    } 

    if(bDaemonMode && (nbind == 0)){
        pid_t cpid;
        //int wstatus;

        cpid = fork(); // split into 2 processes

        if(cpid < 0){
            syslog(LOG_ERR, "ERROR: aesdsocket failed to fork\n");
            printf("ERROR: aesdsocket failed to fork\n");
            closelog();
            shutdown(nsocket, SHUT_RDWR); // closing the listening socket
            fclose(fout);
            pthread_mutex_destroy(&mutex);
            return -1;
        }
        else if(cpid > 0){ // Parent Process Code
            //printf("Hello from Parent\n");
            /*
            if(wait(&wstatus) == -1){
                perror("wait");
                closelog();
                shutdown(nsocket, SHUT_RDWR); // closing the listening socket
                return -1;
            }
            if(WEXITSTATUS(wstatus) != 0){
                printf("exited, status=%d\n", WEXITSTATUS(wstatus));
                perror("wstatus");
                closelog();
                shutdown(nsocket, SHUT_RDWR); // closing the listening socket
                return -1;
            }
            */
            //closelog();
            //shutdown(nsocket, SHUT_RDWR); // closing the listening socket
            exit(0);
        }
        else{ // (cpid == 0) -> Child Process Code
            printf("Hello from aesdsocket daemon\n");
            // Create new session and process group to prevent 
            // terminal signals from mixing with the daemon
            //if(setsid() == -1){
            //    syslog(LOG_ERR, "ERROR: Failed to create a new session and process group\n");
            //    printf("ERROR: Failed to create a new session and process group\n");
            //    exit(1);
            //}
            //Set the working directory to the root directory
            //if(chdir("/") == -1){
            //    syslog(LOG_ERR, "ERROR: Failed to change working directory\n");
            //    printf("ERROR: Failed to change working directory\n");
            //    exit(1);
            //}
        }
    }

    nlisten = listen(nsocket, SOMAXCONN); // sockfd, backlog(# of connections allowed)
    if((nlisten != 0) && !(bSIGINT || bSIGTERM)){
        syslog(LOG_ERR, "ERROR: aesdsocket failed to listen socket\n");
        printf("ERROR: aesdsocket failed to listen socket\n");
        closelog();
        remove("/var/tmp/aesdsocketdata.txt");
        shutdown(nsocket, SHUT_RDWR); // closing the listening socket
        fclose(fout);
        pthread_mutex_destroy(&mutex);
        return -1;
    }

    struct thread_info* thread_info_i;
    pthread_t time_thread;
    pthread_create(&time_thread, NULL, write_timestamp, NULL);

    printf("sizeof(struct thread_info) = %ld\n", sizeof(struct thread_info));

    while( !(bSIGINT || bSIGTERM) ){
        printf("aesdsocket server ready to accept a client...\n");
        client = accept(nsocket, servinfo->ai_addr, &servinfo->ai_addrlen); // sockfd, addr, addrlen
        if((client < 0) && !(bSIGINT || bSIGTERM)){
            syslog(LOG_ERR, "ERROR: aesdsocket failed to accept socket\n");
            printf("ERROR: aesdsocket failed to accept socket. ERROR: %d\n", client);
            closelog();
            fclose(fout);
            remove("/var/tmp/aesdsocketdata.txt");
            shutdown(nsocket, SHUT_RDWR); // closing the listening socket
            pthread_join(time_thread, NULL);
            pthread_mutex_destroy(&mutex);
            shutdown(client, SHUT_RDWR); // closing the connected socket
            shutdown(nsocket, SHUT_RDWR); // closing the listening socket
            return -1;
        }
        if(!(bSIGINT || bSIGTERM)){
            syslog(LOG_DEBUG, "Accepted connection from %d\n", client);
            printf("Accepted connection from %d\n", client);

            struct thread_info* thread_info = malloc(sizeof(struct thread_info));
            thread_info->bThreadComplete = false;
            thread_info->client = client;
            pthread_create(&thread_info->thread_id, NULL, thread_function, (void *)thread_info);

            pthread_mutex_lock(&mutex);
            SLIST_INSERT_HEAD(&threads, thread_info, entries);
            
            SLIST_FOREACH_SAFE(thread_info_i, &threads, entries, thread_info){
                if(thread_info_i->bThreadComplete){
                    pthread_join(thread_info_i->thread_id, NULL);
                    SLIST_REMOVE(&threads, thread_info_i, thread_info, entries);
                    free(thread_info_i);
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }
    pthread_join(time_thread, NULL);
    fclose(fout);
    remove("/var/tmp/aesdsocketdata.txt");
    shutdown(client, SHUT_RDWR); // closing the connected socket
    shutdown(nsocket, SHUT_RDWR); // closing the listening socket
    closelog();
    pthread_mutex_destroy(&mutex);
    printf("aesdsocket complete\n");

    return 0;
}
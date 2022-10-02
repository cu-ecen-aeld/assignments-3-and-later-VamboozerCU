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

#define UNUSED(x) (void)(x)

void INTSignalHandler(int sig);
void TERMSignalHandler(int sig);

bool bSIGINT = false;
bool bSIGTERM = false;
int nSOCKET;

void  INTSignalHandler(int sig){
    UNUSED(sig);
    syslog(LOG_DEBUG, "\nCaught signal, exiting\n");
    printf("\nCaught signal, exiting\n");
    
    // Mask Handlers until exit to prevent reentrancy
    signal(SIGINT, SIG_IGN); // Ignore SIGINT signal (mask)
    signal(SIGTERM, SIG_IGN); // Ignore SIGTERM signal (mask)
    shutdown(nSOCKET, SHUT_RDWR); // closing the listening socket
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
    bSIGTERM = true;
}

int main(int argc, char**argv){
    const char *daemonMode = argv[1];
    bool bDaemonMode = false;

    if( ((argc-1) != 0) && ((argc-1) != 1) ){
        printf("argc: %d; Only 0 or 1 arguments are acceptable\n", (argc-1));
        return -1;
    }
    else if((argc-1) == 1){
        if(strcmp(daemonMode, "-d") == 0){
            printf("Running aesdsocket as daemon\n");
            bDaemonMode = true;
        }
        else{
            printf("ERROR: Unacceptable aesdsocket argument: %s\n", daemonMode);
            return -1;
        }
    }
    UNUSED(argv);
    signal(SIGINT, INTSignalHandler);
    signal(SIGTERM, TERMSignalHandler);
    openlog(NULL,0,LOG_USER); // init syslog

    FILE *fout;
    int nsocket, nbind, getaddr, nlisten, client, nrecv, nsend;
    struct addrinfo hints, *servinfo;
    int curLoc;
    char ch_newline = '\n', ch;
    char* ptr;
    int ngetline;
    
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
        return -1;
    }

    // lose the pesky "Address already in use" error message
    int yes = 1;
    if (setsockopt(nsocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
        syslog(LOG_ERR, "ERROR: aesdsocket failed to setsockopt\n");
        printf("ERROR: aesdsocket failed to setsockopt\n");
        closelog();
        shutdown(nsocket, SHUT_RDWR); // closing the listening socket
        return -1;
    } 

    if(bDaemonMode && (nbind == 0)){
        pid_t cpid;
        int wstatus;

        cpid = fork(); // split into 2 processes

        if(cpid < 0){
            syslog(LOG_ERR, "ERROR: aesdsocket failed to fork\n");
            printf("ERROR: aesdsocket failed to fork\n");
            closelog();
            shutdown(nsocket, SHUT_RDWR); // closing the listening socket
            return -1;
        }
        else if(cpid > 0){ // Parent Process Code
            //printf("Hello from Parent\n");
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
        }
        else{ // (cpid == 0) -> Child Process Code
            printf("Hello from aesdsocket daemon\n");
        }
    }

    char *clientRecvBuffer = (char*)malloc(512);
    char *clientSendBuffer = (char*)malloc(512);
    fout = fopen("/var/tmp/aesdsocketdata.txt", "w+");
    curLoc = 0;

    while( !(bSIGINT || bSIGTERM) ){
        nlisten = listen(nsocket, 1); // sockfd, backlog(# of connections allowed)
        if((nlisten != 0) && !(bSIGINT || bSIGTERM)){
            syslog(LOG_ERR, "ERROR: aesdsocket failed to listen socket\n");
            printf("ERROR: aesdsocket failed to listen socket\n");
            closelog();
            remove("/var/tmp/aesdsocketdata.txt");
            shutdown(nsocket, SHUT_RDWR); // closing the listening socket
            free(clientRecvBuffer);
            free(clientSendBuffer); 
            return -1;
        }

        printf("aesdsocket server awaiting client...\n");
        client = accept(nsocket, servinfo->ai_addr, &servinfo->ai_addrlen); // sockfd, addr, addrlen
        if((client < 0) && !(bSIGINT || bSIGTERM)){
            syslog(LOG_ERR, "ERROR: aesdsocket failed to accept socket\n");
            printf("ERROR: aesdsocket failed to accept socket. ERROR: %d\n", client);
            closelog();
            remove("/var/tmp/aesdsocketdata.txt");
            shutdown(nsocket, SHUT_RDWR); // closing the listening socket
            free(clientRecvBuffer);
            free(clientSendBuffer); 
            return -1;
        }
        if(!(bSIGINT || bSIGTERM)){
            syslog(LOG_DEBUG, "Accepted connection from %d\n", client);
            printf("Accepted connection from %d\n", client);
        }

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
                    fseek(fout, curLoc, SEEK_SET);
                    fprintf(fout, "%s\n", clientRecvBuffer);
                    curLoc = (int)ftell(fout)-1;
                }
            } while(NULL == ptr);

            if(nrecv <= 0){ // Got error or connection closed by client
                if(nrecv == 0){ // Connection closed
                    shutdown(client, SHUT_RDWR); // closing the connected socket
                    break;
                }
                else{ // An ERROR occured while recv from client
                    fclose(fout);
                    remove("/var/tmp/aesdsocketdata.txt");
                    shutdown(client, SHUT_RDWR); // closing the connected socket
                    shutdown(nsocket, SHUT_RDWR); // closing the listening socket
                    free(clientRecvBuffer);
                    free(clientSendBuffer);
                    syslog(LOG_ERR, "ERROR: aesdsocket failed to recv from client socket\n");
                    printf("ERROR: aesdsocket failed to recv from client socket\n");
                    syslog(LOG_DEBUG, "Closed connection from %d\n", client);
                    printf("Closed connection from %d\n", client);
                    closelog();
                    return -1;
                }
            }
            else{ // We got some good data from a client
                // Finish writing last part of packet (or whole packet) to file
                //printf("Writing %s to %s\n", clientRecvBuffer, "/var/tmp/aesdsocketdata.txt");
                fseek(fout, curLoc, SEEK_SET);
                fprintf(fout, "%s\n", clientRecvBuffer);
                curLoc = (int)ftell(fout)-1;

                // Start reading the entire file back to the client
                fseek(fout, 0, SEEK_SET);
                do{
                    memset(clientSendBuffer, 0, 512);
                    ch = '\0';
                    for(ngetline = 0; (ngetline<(512-1)) && (ch != '\n'); ngetline++){
                        ch = fgetc(fout); // reading the file
                        if(ch == EOF){
                            break;
                        }
                        clientSendBuffer[ngetline] = ch;
                    }
                    //printf("Reading %s from %s\n", clientSendBuffer, "/var/tmp/aesdsocketdata.txt");
                    if(ngetline > 1){
                        nsend = (int)send(client, clientSendBuffer, ngetline, 0); // sockfd, buf, len, flags
                    }
                    if(nsend == -1){
                        syslog(LOG_ERR, "ERROR: aesdsocket failed to send to client socket\n");
                        printf("ERROR: aesdsocket failed to send to client socket\n");
                        fclose(fout);
                        remove("/var/tmp/aesdsocketdata.txt");
                        shutdown(nsocket, SHUT_RDWR); // closing the listening socket
                        shutdown(client, SHUT_RDWR); // closing the connected socket
                        closelog();
                        free(clientRecvBuffer);
                        free(clientSendBuffer);               
                        return -1;
                    }
                } while(!feof(fout));
            }
            //printf("************************\n");
            if(bSIGINT || bSIGTERM){
                fclose(fout);
                remove("/var/tmp/aesdsocketdata.txt");
                shutdown(client, SHUT_RDWR); // closing the connected socket
                shutdown(nsocket, SHUT_RDWR); // closing the listening socket
                free(clientRecvBuffer);
                free(clientSendBuffer);
                syslog(LOG_DEBUG, "Closed connection from %d\n", client);
                printf("Closed connection from %d\n", client);
                closelog();
                printf("aesdsocket complete\n");
                return 0;
            }
        }
        shutdown(client, SHUT_RDWR); // closing the connected socket
    }
    fclose(fout);
    remove("/var/tmp/aesdsocketdata.txt");
    shutdown(client, SHUT_RDWR); // closing the connected socket
    shutdown(nsocket, SHUT_RDWR); // closing the listening socket
    free(clientRecvBuffer);
    free(clientSendBuffer);
    closelog();
    printf("aesdsocket complete\n");
    return 0;
}
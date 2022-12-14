#include "systemcalls.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
//#include <stdio.h>
#include <fcntl.h>
#include <string.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd){

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int ret = system(cmd);
    if(ret){
        return false;
    }
    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...){
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        printf("command[%d] = %s\n", i, command[i]);
        //if(strstr(command[i], "$")){
            //perror("ERROR: Can't include '$' as arguments into execv()");
            //return false;
        //}
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];
    if(command[0][0] != '/'){
        perror("ERROR: Cmd NOT absolute path\n");
        return false;
    }
/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    pid_t cpid;
    int wstatus;

    cpid = fork();

    if(cpid == -1){
        perror("fork");
        return false;
    }
    else if(cpid == 0){ // Child Process Code
        printf("Hello from Child\n");
        // int execv(const char *pathname, char *const argv[]);
        execv(command[0], command);
        perror("execv");
        return false;
    }
    else{ // Parent Process Code
        printf("Hello from Parent\n");
        if(wait(&wstatus) == -1){
            perror("wait");
            return false;
        }
        if(WEXITSTATUS(wstatus) != 0){
            printf("exited, status=%d\n", WEXITSTATUS(wstatus));
            perror("wstatus");
            return false;
        }
    }
    
    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...){
    va_list args;
    va_start(args, count);
    char * command[count+1];
    printf("=====================================\n");
    printf("outputfile = %s\n", outputfile);
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        printf("command[%d] = %s\n", i, command[i]);
        //if(strstr(command[i], "$")){
        //    perror("ERROR: Can't include '$' as arguments into execv()");
        //    return false;
        //}
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];
    if(command[0][0] != '/'){
        perror("ERROR: Cmd NOT absolute path\n");
        return false;
    }
/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    pid_t cpid;
    int wstatus;
    //FILE *fout;
    int fout;
    //char *newenviron[] = { NULL };

    //if(outputfile[0] != '/'){
    //    perror("outputfile");
    //    return false;
    //}

    cpid = fork();

    if(cpid == -1){
        perror("ERROR: fork");
        return false;
    }
    else if(cpid == 0){ // Child Process Code
        printf("Hello from Child\n");
        fout = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
        //fout = fopen(outputfile, "w+");
        //fprintf(fout, "%s\n", writestr);
        if(fout < 0){ 
            perror("ERROR: fopen"); 
            return false;
        }
        if (dup2(fout, 1) < 0) { perror("ERROR: dup2"); return false; }
        //fclose(fout);
        close(fout);
    // int execv(const char *pathname, char *const argv[]);
        if(execv(command[0], command)){
            perror("ERROR: execv");
            return false;
        }
    }
    else{ // Parent Process Code
        printf("Hello from Parent\n");
        if(wait(&wstatus) == -1){
            perror("ERROR: wait");
            return false;
        }
        if(WEXITSTATUS(wstatus) != 0){
            printf("exited, status=%d\n", WEXITSTATUS(wstatus));
            perror("ERROR: wstatus");
            return false;
        }
    }

    va_end(args);

    return true;
}

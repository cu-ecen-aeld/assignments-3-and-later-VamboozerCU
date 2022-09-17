#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
//#include "writer.h"

const char * exampleStr = "";

int main(int argc, char**argv)
{
    const char *writefile = argv[1];
    const char *writestr = argv[2];
    FILE *fout;

    openlog(NULL,0,LOG_USER);

    if((argc-1) != 2){
        syslog(LOG_ERR, "argc: %d; Only 2 arguments are acceptable.\n", (argc-1));
        printf("argc: %d; Only 2 arguments are acceptable.\n", (argc-1));
        return 1;
    }
    /*
    You do not need to make your "writer" utility create directories 
    which do not exist.  You can assume the directory is created by 
    the caller.
    */
    printf("writefile: %s\n", writefile);
    printf("writestr: %s\n", writestr);

    fout = fopen(writefile, "w");
    syslog(LOG_DEBUG, "Writing %s to %s\n", writestr, writefile);
    fprintf(fout, "%s\n", writestr);

    fclose(fout);
    return 0;
}
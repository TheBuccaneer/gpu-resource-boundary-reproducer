#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
int main(int argc,char **argv) {
 if(argc!=3 || strcmp(argv[1],"--approved-run"))return 64;
 puts("{\"fixture\":\"CPU_ONLY\",\"started\":true}");fflush(stdout);
 if(!strcmp(argv[2],"CPU-FAIL"))return 7;
 if(!strcmp(argv[2],"CPU-SIGNAL")){raise(SIGTERM);return 9;}
 if(!strcmp(argv[2],"CPU-TIMEOUT")){signal(SIGTERM,SIG_IGN);for(;;)pause();}
 if(!strcmp(argv[2],"CPU-INCONCLUSIVE"))return 78;
 if(!strcmp(argv[2],"CPU-SKIP"))return 77;
 return 0;
}

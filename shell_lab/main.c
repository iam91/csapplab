#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/wait.h>
#include<sys/types.h>
#include"job.h"

#define MAX_ARGS      128
#define MAX_LINE      1024
#define MAX_FILE_NAME 1024

#define SHELL_PROMPT "> "
#define BG_MARKER    "&"

#define BUILTIN_CMD_QUIT "quit"
#define BUILTIN_CMD_JOBS "jobs"
#define BUILTIN_CMD_BG   "bg"
#define BUILTIN_CMD_FG   "fg"

#define IS_IN_FILE 0x1
#define IS_OUT_FILE 0x2

#define ignore_whitespace(buf) do{ while(*buf && *buf == ' ') buf++; }while(0)

void eval(const char *);
int redirect(char *);
int parse(char *, char **);
int is_background(int *, char **);
int builtin(const char *);

/** start of built-in commands **/
static void quit();
static void jobs();
/** end of built-in commands **/

int main(){
    char cmdline[MAX_LINE];

    while(1){
        printf("%s", SHELL_PROMPT);
        fgets(cmdline, MAX_LINE, stdin);
        if(feof(stdin)){
            exit(0);
        }

        eval(cmdline);
    }
}

void eval(const char *cmdline){
    int bg;
    pid_t pid;
    int argc;
    int ctch;
    char buf[MAX_LINE];
    char *argv[MAX_ARGS];
    
    strcpy(buf, cmdline);

    buf[strlen(buf) - 1] = ' ';

    redirect(buf);
    argc = parse(buf, argv);
    bg = is_background(&argc, argv);
    if(!argc){ return; }

    builtin(argv[0]);

    ctch = pid = fork();
    if(ctch == -1){
        //TODO error handle
    }
    if(pid == 0){
        ctch = execve(argv[0], argv, NULL);
        if(ctch == -1){
            //TODO error handle
            perror("execve");
        }
    }else{
        if(!bg){
            int jid = addjob(pid, JOB_STATE_FG);
            ctch = waitpid(pid, NULL, 0);
            if(ctch == -1){
                //TODO error handle
            }
        }else{
            int jid = addjob(pid, JOB_STATE_BG);
            printf("[%d] %d\n", jid, pid);
        }
    }
}

int redirect(char *buf){

    int idx;
    int io;
    char *b = buf;
    char ifile[MAX_FILE_NAME];
    char ofile[MAX_FILE_NAME];

    io = 0;
    idx = 0;
    while(*buf){
        //delimiter encountered
        if(!*buf || *buf == '<' || *buf == '>'){
            if(io & IS_IN_FILE) ifile[idx] = '\0';
            if(io & IS_OUT_FILE) ofile[idx] = '\0';
            io = 0;
        }

        ignore_whitespace(buf);

        if(*buf == '<'){
            memset(ifile, 0, MAX_FILE_NAME);
            io = IS_IN_FILE;
            idx = 0;
            *buf = ' ';
        }
        else if(*buf == '>'){
            memset(ofile, 0, MAX_FILE_NAME);
            io = IS_OUT_FILE;
            idx = 0;
            *buf = ' ';
        }
        else if(*buf){
            if(io & IS_IN_FILE){
                ifile[idx++] = *buf;
                *buf = ' ';
            }
            if(io & IS_OUT_FILE){
                ofile[idx++] = *buf;
                *buf = ' ';
            } 
        }
        buf++;
    }
    buf = b;
    return 0;
}

int parse(char *buf, char **argv){
    int argc = 0;
    char *delimiter = NULL;

    ignore_whitespace(buf);
    
    while((delimiter = strchr(buf, ' '))){
        *delimiter = '\0';
        argv[argc++] = buf;
        buf = delimiter + 1;
        ignore_whitespace(buf);
    }
    argv[argc] = NULL;
    return argc;
}

int is_background(int *argc, char **argv){
    if(strcmp(argv[*argc - 1], BG_MARKER)){
        return 0;
    }else{
        argv[--*argc] = NULL;
        return 1;
    }
}

int builtin(const char *cmd){
    if(!strcmp(cmd, BUILTIN_CMD_QUIT)){
        quit();
    }
    else if(!strcmp(cmd, BUILTIN_CMD_JOBS)){
        jobs();
    }
    return 0;
}

/** start of built-in commands **/
void quit(){
    exit(0);
}

void jobs(){
    printf("Jobs\n");
}
/** end of built-in commands **/
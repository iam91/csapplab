#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/stat.h>
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

#define REDIR_IN 0x01
#define REDIR_OUT 0x02

#define ignore_whitespace(buf) do{ while(*buf && *buf == ' ') buf++; }while(0)

void eval(const char *);
int parse_redirect(char *, char *, char *);
int parse_args(char *, char **);
int is_background(int *, char **);
int builtin(const char *);
void redirect(const int, const char *, const char *);

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
    int redir;
    char buf[MAX_LINE];
    char *argv[MAX_ARGS];

    char ifile[MAX_FILE_NAME];
    char ofile[MAX_FILE_NAME];
    int ifd;
    int ofd;
    
    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';

    redir = parse_redirect(buf, ifile, ofile);

    argc = parse_args(buf, argv);
    bg = is_background(&argc, argv);
    if(!argc){ return; }

    builtin(argv[0]);

    ctch = pid = fork();
    if(ctch == -1){
        //TODO error handle
    }
    if(pid == 0){

        redirect(redir, ifile, ofile);

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
            //TODO recycle child process 
            int jid = addjob(pid, JOB_STATE_BG);
            printf("[%d] %d\n", jid, pid);
        }
    }
}

void redirect(const int redir, const char *ifile, const char *ofile){
    int ifd, ofd;
    int c;

    printf("redir: %d\n", redir);

    if(redir & REDIR_IN){
        ifd = open(ifile, O_RDONLY);
        if(ifd == -1){
            if(errno == ENOENT){
                printf("Not exists\n");
                exit(0);
            }
            perror("open in file");
        }
        c = dup2(ifd, STDIN_FILENO);
        close(ifd);
        if(c == -1){
            perror("dup2 in file");
        }
    }
    if(redir & REDIR_OUT){
        ofd = open(ofile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if(ofd == -1){
            perror("open out file");
        }
        c = dup2(ofd, STDOUT_FILENO);
        close(ofd);
        if(c == -1){
            perror("dup2 out file");
        }
    }
}

int parse_redirect(char *buf, char *ifile, char *ofile){
    //TODO parse_redirection parsing check
    int idx;
    int io;
    int redir;
    char *b = buf;

    io = 0;
    idx = 0;
    redir = 0;
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
            redir |= REDIR_IN;
            io = IS_IN_FILE;
            idx = 0;
            *buf = ' ';
        }
        else if(*buf == '>'){
            memset(ofile, 0, MAX_FILE_NAME);
            redir |= REDIR_OUT;
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
    return redir;
}

int parse_args(char *buf, char **argv){
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
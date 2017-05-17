#include<unistd.h>
#include<string.h>
#include<signal.h>
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

#define SHELL_PROMPT "tsh> "
#define BG_MARKER    "&"

#define BUILTIN_CMD_QUIT "quit"
#define BUILTIN_CMD_JOBS "jobs"
#define BUILTIN_CMD_BG   "bg"
#define BUILTIN_CMD_FG   "fg"

#define IS_NOT_BUILTIN  0
#define IS_BUILTIN_QUIT 1
#define IS_BUILTIN_JOBS 2
#define IS_BUILTIN_BG   3
#define IS_BUILTIN_FG   4

#define IS_IN_FILE 0x1
#define IS_OUT_FILE 0x2

#define REDIR_IN 0x01
#define REDIR_OUT 0x02

#define ignore_whitespace(buf) do{ while(*buf && *buf == ' ') buf++; }while(0)

static int ctch = 0;

void eval(const char *);
int parse_redirect(char *, char *, char *);
int parse_args(char *, char **);
int is_background(int *, char **);
int is_builtin(const char *);
void exe_builtin(const int, const char **);
job_t *getjob(const char *);

int redirect_input(const int, const char *);
int redirect_output(const int, const char *);
void fix_io(int, int);

/** start of signal handlers **/
void sigchld_handler(int);
void sigtstp_handler(int);
void sigint_handler(int);
/** end of signal handlers **/

/** start of built-in commands **/
void quit(const char **);
void jobs(const char **);
void bg(const char **);
void fg(const char **);
/** end of built-in commands **/

int main(){
    char cmdline[MAX_LINE];

    //register signal handlers
    signal(SIGCHLD, sigchld_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGINT, sigint_handler);
    
    while(1){
        printf("%s", SHELL_PROMPT);
        fgets(cmdline, MAX_LINE, stdin);
        if(feof(stdin)){ exit(0); }

        eval(cmdline);
    }
}

void eval(const char *cmdline){
    int bg, bltin;
    pid_t pid;
    int argc;
    int redir;
    char buf[MAX_LINE];
    char *argv[MAX_ARGS];

    char ifile[MAX_FILE_NAME];
    char ofile[MAX_FILE_NAME];
    int stdin_backup;
    int stdout_backup;
    
    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';

    redir = parse_redirect(buf, ifile, ofile);
    if(redir == -1){
        printf("Parsing fails\n");
        return;
    }
    argc = parse_args(buf, argv);

    bg = is_background(&argc, argv);
    if(!argc){ return; }
    bltin = is_builtin(argv[0]);

    if(bltin){
        stdin_backup = redirect_input(redir, ifile);
        stdout_backup = redirect_output(redir, ofile);
        exe_builtin(bltin, (const char **)argv);
        fix_io(stdin_backup, stdout_backup);
        return;
    }

    //not bulit-in command
    ctch = pid = fork();
    if(ctch == -1){
        //TODO error handle
    }
    if(pid == 0){
        redirect_input(redir, ifile);
        redirect_output(redir, ofile);
        ctch = execve(argv[0], argv, NULL);
        if(ctch == -1){
            //TODO error handle
            perror("execve");
        }
    }else{
        if(!bg){
            addjob(pid, JOB_STATE_FG);
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

void fix_io(int stdin_backup, int stdout_backup){
    int c;
    if(stdin_backup != STDIN_FILENO){ //must check why?
        c = dup2(stdin_backup, STDIN_FILENO);
        close(stdin_backup);
        if(c == -1){
            perror("reset stdin dup2");
        }
    }
    if(stdout_backup != STDOUT_FILENO){ //must check why?
        c = dup2(stdout_backup, STDOUT_FILENO);
        close(stdout_backup);
        if(c == -1){
            perror("reset stdout dup2");
        }
    }
}

int redirect_input(const int redir, const char *ifile){
    int c, ifd, stdin_backup = STDIN_FILENO;
    if(redir & REDIR_IN){
        stdin_backup = dup(STDIN_FILENO);
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
    return stdin_backup;
}

int redirect_output(const int redir, const char *ofile){
    int c, ofd, stdout_backup = STDOUT_FILENO;
    if(redir & REDIR_OUT){
        stdout_backup = dup(STDOUT_FILENO);
        //chmod 744
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
    return stdout_backup;
}

int parse_redirect(char *buf, char *ifile, char *ofile){
    //TODO some problems with &
    int io;
    int idx;
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

    if(redir & REDIR_OUT && !strlen(ofile)){ redir = -1; }
    if(redir & REDIR_IN && !strlen(ifile)){ redir = -1; }

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

int is_builtin(const char *cmd){
    if(!strcmp(cmd, BUILTIN_CMD_QUIT)){
        return IS_BUILTIN_QUIT;
    }else if(!strcmp(cmd, BUILTIN_CMD_JOBS)){
        return IS_BUILTIN_JOBS;
    }else if(!strcmp(cmd, BUILTIN_CMD_FG)){
        return IS_BUILTIN_FG;
    }else if(!strcmp(cmd, BUILTIN_CMD_BG)){
        return IS_BUILTIN_BG;
    }else{
        return IS_NOT_BUILTIN;
    }
}

void exe_builtin(const int builtin, const char **argv){
    switch(builtin){
        case IS_BUILTIN_QUIT:
            quit(argv); break;
        case IS_BUILTIN_JOBS:
            jobs(argv); break;
        case IS_BUILTIN_BG:
            bg(argv); break;
        case IS_BUILTIN_FG:
            fg(argv); break;
        default:
            break;
    }
}

job_t *getjob(const char *xid){
    if(!xid || !strlen(xid)) return NULL;
    if(xid[0] == '%') return getjobjid(atoi(&xid[1]));
    else return getjobpid(atoi(xid));
}

/** start of built-in commands **/
void quit(const char **argv){
    exit(0);
}

void jobs(const char **argv){
    list_bg_jobs();
}

void bg(const char **argv){
    job_t *job = getjob(argv[1]);
    ctch = kill(job->pid, SIGCONT);
    if(ctch == -1) perror("bg kill");
    job->state = JOB_STATE_BG;
}

void fg(const char **argv){
    job_t *job = getjob(argv[1]);
    ctch = kill(job->pid, SIGCONT);
    if(ctch == -1) perror("fg kill");
    job->state = JOB_STATE_FG;
    
    ctch = waitpid(job->pid, NULL, 0);
    if(ctch == -1){
        //TODO error handle
    }
}
/** end of built-in commands **/

/** start of signal handlers **/
void sigchld_handler(int sig){
    pid_t pid;
    while((pid = waitpid(-1, NULL, WNOHANG)) > 0){
        deletejob(pid);
        printf("PID: %d reaped.\n", pid);
    }
    if(pid == -1){
        if(errno != ECHILD){
            perror("waitpid");
        }
    }
}

void sigtstp_handler(int sig){
    pid_t fg_pid = fgpid();
    ctch = kill(-fg_pid, SIGTSTP);
    if(ctch == -1){ 
        if(errno == ECHILD){
            perror("kill"); 
        }
    }
}

void sigint_handler(int sig){
    pid_t fg_pid = fgpid();
    ctch = kill(-fg_pid, SIGINT);
    if(ctch == -1){ 
        if(errno == ECHILD){
            perror("kill"); 
        }
    }
}
/** end of signal handlers **/
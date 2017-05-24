#include<sys/types.h>

#define MAXLINE 1024
#define MAXJOBS 16
#define MAXJID  (1<<16)

#define UNDEF 0x0
#define FG    0x1
#define BG    0x2
#define ST    0x4

typedef struct job_t{
    pid_t pid;
    int jid;
    int state;
    char cmdline[MAXLINE];
} job_t;

void clearjob(job_t *job);
void initjobs();
int maxjid(); 
int addjob(pid_t pid, int state, const char *cmdline);
int deletejob(pid_t pid); 
pid_t fgpid();
struct job_t *getjobpid(pid_t pid);
struct job_t *getjobjid(int jid); 
int pid2jid(pid_t pid); 
void listjobs(int states);
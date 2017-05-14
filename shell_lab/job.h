#include<sys/types.h>

#define MAX_JOBS 16
#define MAX_JID  (1<<16)

#define JOB_STATE_UNDEF 0
#define JOB_STATE_FG    1
#define JOB_STATE_BG    2
#define JOB_STATE_ST    3

typedef struct job_t{
    pid_t pid;
    int jid;
    int state;
} job_t;

void clearjob(job_t *job);
void initjobs();
int maxjid(); 
int addjob(pid_t pid, int state);
int deletejob(pid_t pid); 
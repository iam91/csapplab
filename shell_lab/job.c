#include<stdio.h>
#include"job.h"

#define check_pid(pid) do{ if(pid < 1) return 0; }while(0)

static int next_jid = 1;
static job_t jobs[MAX_JOBS];

int maxjid(){
    int i, max = 0;
    for(i = 0; i < MAX_JOBS; i++){
        if(jobs[i].jid > max){
            max = jobs[i].jid;
        }
    }
    return max;
}

void clearjob(job_t *job){
    job->pid = 0;
    job->jid = 0;
    job->state = JOB_STATE_UNDEF;
}

void initjobs(){
    int i;
    for(i = 0; i < MAX_JOBS; i++){
        clearjob(&jobs[i]);
    }
}

int addjob(pid_t pid, int state){
    int i;

    check_pid(pid);

    for(i = 0; i < MAX_JOBS; i++){
        if(jobs[i].state == JOB_STATE_UNDEF){
            jobs[i].pid = pid;
            jobs[i].jid = next_jid++;
            jobs[i].state = state;
            return jobs[i].jid;
        }
    }
    return 0;
}

int deletejob(pid_t pid){
    int i;

    check_pid(pid);

    for(i = 0; i < MAX_JOBS; i++){
        if(jobs[i].pid == pid){
            clearjob(&jobs[i]);
            next_jid = maxjid() + 1;
            return 1;
        }
    }
    return 0;
}

void list_bg_jobs(){
    int i;
    for(i = 0; i < MAX_JOBS; i++){
        if(jobs[i].state == JOB_STATE_BG){
            printf("[%d] (%d)\n", jobs[i].jid, jobs[i].pid);
        }
    }
}

pid_t fgpid(){
    int i;
    for(i = 0; i < MAX_JOBS; i++){
        if(jobs[i].state == JOB_STATE_FG){
            return jobs[i].pid;
        }
    }
    return 0;
}

job_t *getjobpid(pid_t pid){
    int i;
    if(pid < 1) return NULL;
    for(i = 0; i < MAX_JOBS; i++){
        if(jobs[i].pid == pid){
            return &jobs[i];
        }
    }
    return NULL;
}

job_t *getjobjid(int jid){
    int i;
    if(jid < 1) return NULL;
    for(i = 0; i < MAX_JOBS; i++){
        if(jobs[i].jid == jid){
            return &jobs[i];
        }
    }
    return NULL;
}


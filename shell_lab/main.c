#include<string.h>
#include<stdlib.h>
#include<stdio.h>

#define MAX_ARGS_LENGTH 128
#define SHELL_PROMPT "> "

void eval(const char *);
int parse(const char *, char **);

int main(){
    char cmdline[MAX_ARGS_LENGTH];

    while(1){
        printf("%s", SHELL_PROMPT);
        fgets(cmdline, MAX_ARGS_LENGTH, stdin);
        if(feof(stdin)){
            exit(0);
        }

        eval(cmdline);
    }
}

void eval(const char *cmdline){
    int i;
    int argc;
    char **argv = (char **)malloc(sizeof(char **));
    
    argc =parse(cmdline, argv);
    for(i = 0; i < argc; i++){
        printf("%s\n", argv[i]);
    } 
}

int parse(const char *cmdline, char **argv){
    int len = 0;
    char buf[MAX_ARGS_LENGTH];
    char *currArgv;
    int argc = 0;
    int pre = 0;
    int curr = 0;
    int arglen = 0;
    
    strcpy(buf, cmdline);
    len = strlen(cmdline);
    buf[len - 1] = ' ';

    //ignore beginning whitespace
    for(curr = 0; buf[curr] == ' ' && curr < len; curr++);
    
    //parse the command line
    pre = curr;
    while(curr < len){
        if(buf[curr] == ' '){
            arglen = curr - pre;
            currArgv = (char *)malloc(sizeof(char) * (arglen + 1));

            strncpy(currArgv, buf + pre, arglen);
            currArgv[arglen] = '\0';

            argv[argc] = currArgv;

            while(curr < len && buf[curr] == ' '){
                curr++;
            }
            pre = curr;
            argc++;
        }else{
            curr++;
        }
    }
    return argc;
}
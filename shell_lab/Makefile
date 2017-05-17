FILES=main.c job.c job.h
CC=gcc
CFLAGS=-Wall

zshell: ${FILES}
	${CC} ${CFLAGS} -o zshell ${FILES}

debug: ${FILES}
	${CC} -ggdb ${CFLAGS} -o zshell ${FILES}

clear:
	rm ./zshell
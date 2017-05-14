CC = gcc
CFLAGS = -Wall
FILES = main.c job.c job.h
TARGET = zshell

zshell: ${FILES}
	gcc ${CFLAGS} -o ${TARGET} ${FILES}
debug: ${FILES}
	gcc -ggdb ${CFLAGS} -o ${TARGET} ${FILES}
clear:
	rm ${TARGET}

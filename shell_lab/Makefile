FILES=main.c job.c job.h
CC=gcc
CFLAGS=-Wall

zshell: ${FILES}
	${CC} ${CFLAGS} -o tsh ${FILES}

debug: ${FILES}
	${CC} -ggdb ${CFLAGS} -o tsh ${FILES}

test:
	cp ./tsh ./shlab-handout/tsh

clear:
	rm ./tsh
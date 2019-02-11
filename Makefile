CC=gcc
CFLAGS=-std=gnu99 -g

yash: shell.c signals.o parser.o runner.o jobs.o
	$(CC) -o yash shell.c signals.o parser.o runner.o jobs.o $(CFLAGS)

signals.o: signals.c
	$(CC) -c -o signals.o signals.c $(CFLAGS)

parser.o: parser.c
	$(CC) -c -o parser.o parser.c $(CFLAGS)

runner.o: runner.c
	$(CC) -c -o runner.o runner.c $(CFLAGS)

jobs.o: jobs.c
	$(CC) -c -o jobs.o jobs.c $(CFLAGS)


GCC=gcc

GCCFLAGS=-Wall -fsanitize=address -pthread -std=gnu99 -D_GNU_SOURCE -g
LDFLAGS=-lpthread -lrt
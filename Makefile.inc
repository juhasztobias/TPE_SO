GCC=gcc
GPP=g++
LD=ld
AR=ar
ASM=nasm

GCCFLAGS=-fno-exceptions -g -std=c99 -Wall -ffreestanding -nostdlib -fno-common -fno-builtin-malloc -fno-builtin-free -fno-builtin-realloc
ARFLAGS=rvs
ASMFLAGS=-felf64
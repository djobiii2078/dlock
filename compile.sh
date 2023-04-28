#!/bin/bash 

gcc -olock.o lock.c -lpthread
gcc -olock-atom.o lock-atom.c -lpthread -latomic

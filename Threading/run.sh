#!/bin/bash
rm -f test
cc main.c -o test -lpthread
./test

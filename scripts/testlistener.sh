#!/usr/bin/env bash
[ ! -p fifo ] && mkfifo fifo
cat test.dat > fifo
cat fifo | nc -l -p 8000

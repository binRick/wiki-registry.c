#!/bin/bash
reset
passh -L .nodemon reap nodemon --delay .1 -w Makefile -e c,h,sh,Makefile --signal SIGKILL -w ./ -x sh -- -c "clear;${@:-make dev}||true"

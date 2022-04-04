#!/bin/bash
reset
nodemon --delay .1 -e c,h,sh --signal SIGKILL -w ./ -x sh -- -c "clear;./test.sh ${@:-}||true"

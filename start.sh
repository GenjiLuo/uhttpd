#!/bin/bash

# stop running program first of all
for i in `ps ax | grep \.\/uhttpd | awk '{print $1}'`;
do
    kill -9 $i;
done

# there's problem to start 'uhttpd' but 'uhttpd2/3' works well
# don't know why
# FIXME
[ -x uhttpd ] && ./uhttpd 2>&1 > /dev/null &
[ -x uhttpd2 ] && ./uhttpd2 2>&1 > /dev/null &
[ -x uhttpd3 ] && ./uhttpd3 2>&1 > /dev/null &
[ -x uhttpd4 ] && ./uhttpd4 2>&1 > /dev/null &
[ -x uhttpd5 ] && ./uhttpd5 2>&1 > /dev/null &

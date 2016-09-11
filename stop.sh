#!/bin/bash

for i in `ps aux | grep \.\/uhttpd | awk '{print $2}'`;
do
    kill -9 $i; 
done


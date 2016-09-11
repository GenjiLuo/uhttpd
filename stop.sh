#!/bin/bash

for i in `ps | grep uhttpd | awk '{print $1}'`;
do
    kill -9 $i; 
done


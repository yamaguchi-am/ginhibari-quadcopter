#!/bin/bash

awk 'BEGIN{n=0} /^\ \ /{print $1,"=",n; n=n+1}' < $1 | sed s/,//g

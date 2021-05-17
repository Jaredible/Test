#!/bin/bash

# If $1 is null, return test.txt
# but do not set $1
def1=${1:-test.txt}

echo $1
echo $def1


# If var1 is null, return dog.txt
# and set #var1 to dog.txt
# If it is not null, give it the nonnull value 
var1=
def2=${var1:=dog.txt}
echo $var1
echo $def2

# Outputting an error if they do not give us a command line argument
#file=${1:?"Usage: $0 filename"}


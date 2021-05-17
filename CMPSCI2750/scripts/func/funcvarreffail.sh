#!/bin/bash

function addOne()
{
	## Declare it an integer so we increment it nicely
	declare -i temp
	temp="$1";
	echo "Before incrementing in func, it is $1"
	temp+=1;

	## Sets positional parameter $1 to temp
	set -- $temp;

	echo "After incrementing in func, it is $1"
}

n=12;

echo "n before function call is $n"
addOne $n

echo "n after function call is $n"


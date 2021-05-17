#!/bin/bash

function dostuff()
{
	local numargs="$#"
	local str1="$1";   ## First argument to function
	echo "Function dostuff was called with $numargs arguments and the first was $str1"
}

echo "The first option outside of function is $1"

dostuff
dostuff more

echo "The first option outside of function and after call is $1"

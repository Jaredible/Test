#!/bin/bash

# This will work by substituting the actual variable in
# so $1 expands to the symbol n
function addOne()
{
	let $1+=1;
}

n=12;

echo "n before function call is $n"
addOne n

echo "n after function call is $n"


#!/bin/bash

#Let us access the value of n inside the function

function addOne() 
{
	echo ${!1}   ## displays $n
	echo $1
	let $1+=1;   ## let n+=1
	echo ${!1}   ## displays $n
}

n=12;
addOne n;
echo $n;


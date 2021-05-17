#!/bin/bash

str="yes"
read str
while [[ "$str" != "no" ]]
do
	echo $str
	read str
done

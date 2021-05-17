#!/bin/bash

br=()					##(empty array)

echo -n "Please input an array:" 
read -a br


for el in "${br[@]}"
do
	echo $el
done

for (( i=0; i < ${#br[@]}; i++ ))
do
	echo ${br[$i]}
done

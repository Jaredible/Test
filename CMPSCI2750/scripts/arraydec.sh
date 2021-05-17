#!/bin/bash

br=()					##(empty array)

#declare -a br

fruits=("red apple" "golden banana")
fruits+=("navel orange")		##(array concatenation)
echo ${fruits[1]}			##(array element value, golden banana) 
echo ${#fruits[*]}
echo ${#fruits[@]}			##(length of array) 
fruits[2]="green pear"			##(array element assignment) 
fruits[6]="seedless watermelon"		##(gap in index allowed) 
echo ${#fruits[*]}
br+=("${fruits[@]}")			##(br now same as fruits)
echo ${fruits[*]}
echo ${fruits[6]}

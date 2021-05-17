#!/bin/bash
CompSci=("Intro to C++" "Data Structures" "Linux Tools")

echo ${CompSci[*]}
CompSci[5]=${CompSci[2]}
echo ${CompSci[*]}

#!/bin/bash

function latex ()
{
	/usr/bin/pdflatex ${1%.tex}.tex && \
	/usr/bin/acroread ${1%.tex}.pdf
}

## allows it to be called as latex pathname.tex or latex pathname

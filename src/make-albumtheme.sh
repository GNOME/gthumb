#!/bin/sh

flex albumtheme.l
mv lex.yy.c lex.albumtheme.c 
indent lex.albumtheme.c
bison albumtheme.y -o albumtheme.c
indent albumtheme.c

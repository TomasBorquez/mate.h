# 01 Basic Script

This is the simplest example of a script, you can just run it with `gcc mate.c -o mate && ./mate`, there is not much to mention here, you can
see the inner workings of mate very easily, in fact you are supposed to know how some stuff works under the hood and to look at function definitions
that contain useful comments as well as point to why you are getting certain errors.

Mate rebuilds itself if there were any changes on `mate.c`, so after compiling once you can just run it with `./mate`.

## Advice
A build system for this simple example is a bit overkill, I recommended to only move to a build system if you have many vendor packages or 
many C files, this will make compile times usually faster and make your project more organized.

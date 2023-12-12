# EXT2 File System 

![](https://img.shields.io/badge/Difficulty-Hard-lightgrey.svg)  ![](https://img.shields.io/badge/Ubuntu-20.04-yellow.svg) <br>

Lecture book: Systems Programming in Unix/Linux

----
Design and implement a binary tree to simulate the Unix/Linux file system tree, which supports pwd, ls, cd, mkdir, rmdir, creat, rm operatiobns as in a real file system. 

## Dependencis:
gcc
```
 sudo apt-get install gcc-multilib
```
Create virtual disks
```
sudo dd if=/dev/zero of=mydisk bs=1024 count=1440
sudo mke2fs -b 1024 mydisk 1440
```
Generate a.out
```
gcc main.c util.c
```
Run program 
```
./a.out
```

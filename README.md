# EXT2 File System 

![](https://img.shields.io/badge/Difficulty-Hard-lightgrey.svg)  ![](https://img.shields.io/badge/Ubuntu-20.04-yellow.svg) <br>
--------
Lecture book: Systems Programming in Unix/Linux

---
**Course** : Systems Programming C/C++

----
Design and implement a Linux-compatible EXT2 file system.

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

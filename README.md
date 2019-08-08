# REAL-TIME-SYSTEMS: 22 / 01 / 2014

This project is related with the design and implementation of the Shooting computer game
which was developed under Linux operating system and C programming language, using pthread
library for multithreading and allegro library for the graphics design. 

The description of the project, what it looks like and how it works is thoroughly discussed in <b>Report.pdf</b>

The main goal of this project is to know how to manage multiple threads on a Real-time system. 

Demo of the application can be found at https://youtu.be/OlQaUQeCeww

## Requirements
Allegro library

## Run the Program 
gcc Shooting.c -o shooting `allegro-config --libs` -lpthread -lrt
<br>./shooting

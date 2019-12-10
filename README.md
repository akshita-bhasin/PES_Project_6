README File:

PES PROJECT 6 README

This is a readme for Project #6 for the Principles of Embedded Software (Fall 2019) class.

Team Members: Akshita Bhasin & Madhukar Arora

Source Files:

1.  circularbuffer.h
2.  circularbuffer.c
3.  led_control.h
4.  led_control.c
5.  logger.h
6.  logger.c
7.  timestamp.h
8.  timestamp.c
9.  FreeRTOSConfig.h
10.  main.c

Guides on Compilation / Execution Notes:

Hardware Used: Freedom Board FRDM KL25Z
IDE Used: MCU Xpresso (https://mcuxpresso.nxp.com/en/welcome)

Build Using Macros:

1.  To run the program in Debug Mode set the log_level_a in logger.c to 1.
    
2.  To run the program in Normal Mode set the log_level_a in logger.c to
    
3.  Program 1 and 2 are executed using #ifdef macros.
    
    ```c
    #if PGM_1 // runs program 1
    
    #if PGM_2 // runs program 2
    ```
    

Observations:

1.  We faced issues using semaphores to control access to the shared LED with the application hitting a hard fault. Used flags to deal with this.
    
2.  Priority setting for tasks only lead to the highest priority task being executed, and hence had to change a few underlying modules.
    
3.  TaskDelayUntil gave various hardfault errors woth semaphores, we weren't able to fix it, but will work on it in the future.

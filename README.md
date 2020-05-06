Shelldone
=========

Shelldone is just a shell written in C from scratch so I can play with it.
I guess it will never be as complete as zsh/csh/bash/whatever, but actually it
is not my goal.
I'm just working on it to learn and improve my skills in C programming.

Notes to self
-------------

Currently, the job handling is kind of broken (`Ctrl+Z` doesn't work anymore for
instance). This is because from now on, we put every *job* in its own process
group instead of the shell's one. Thus we need to attach the foreground job the
tty so the `SIGTSTP` signal is not handled anymore by the parent process.
What we need to do now, is to *enqueue* every *job* regardless if it is sent to
the foreground or to the background.
There is also an issue with the way SIGCHLD is handled currently. We **must
not** ignore it otherwise we cannot await our children later on.
What we need to do instead is to block it and then unblock it at the beginning
of a new line parsing.

Compilation
-----------

```
$ make
```

Execution
---------

```
$ ./shelldone
```

Features
--------

- Jobs control (command jobs, fg, bg + SIGTSTP (^Z) )
- Arguments protection: "an example", 'another example'
- Multi-line commands (with a final backslash (\) )
- Multiple commands: ls | grep .c && echo OK
- Background commands: sleep 120 &
- Extensible with modules (see README in plugins directory)

Example
-------

```
ziirish@carbon:~/workspace/shelldone/src$ valgrind --leak-check=full
--show-reachable=yes ./shelldone
==22418== Memcheck, a memory error detector
==22418== Copyright (C) 2002-2010, and GNU GPL'd, by Julian Seward et al.
==22418== Using Valgrind-3.6.0.SVN-Debian and LibVEX; rerun with -h for
copyright info
==22418== Command: ./shelldone
==22418==
shell> module load ../plugins/parsing/jobs/jo
jobs.c  jobs.so
shell> module load ../plugins/parsing/jobs/jobs.so
Module 'jobs' successfuly loaded.
shell> ../../test/bl
blah    blah.c
shell> ../../test/blah
1
2
3
^Z
[1] 22421 (../../test/blah) suspended
shell> ../../test/blah
1
2
3
^Z
[2] 22422 (../../test/blah) suspended
shell> ../../test/blah
1
2
3
^Z
[3] 22423 (../../test/blah) suspended
shell> fg %1
[1]  - continued 22421 (../../test/blah)
4
5
6
^Z
[1] 22421 (../../test/blah) suspended
shell> fg %3 %2 %1
[3]  - continued 22423 (../../test/blah)
4
5
6
7
8
9
10
[2]  - continued 22422 (../../test/blah)  
4
5
6
7
8
9
10
[1]  + continued 22421 (../../test/blah)  
7
8
9
10
shell> quit
==22418==
==22418== HEAP SUMMARY:
==22418==     in use at exit: 0 bytes in 0 blocks
==22418==   total heap usage: 6,933 allocs, 6,933 frees, 725,868 bytes allocated
==22418==
==22418== All heap blocks were freed -- no leaks are possible
==22418==
==22418== For counts of detected and suppressed errors, rerun with: -v
==22418== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 4 from 4)
```

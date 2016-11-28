There are three Makefile-type files in this directory:

 - Makefile is a top-level Makefile that can be used as-is to build
   your system (operating system) kernel test file as well as your
   user-level test programs.

 - Makefile.sys is a *TEMPLATE* for a Makefile that can be used to
   build your system (operating system) kernel test file.
 
 - Makefile.user is a *TEMPLATE* for a Makefile that can be used to
   build your user-level test programs.

Please copy these three files to your own directory in order to use them.

Also, please read the comments at the top of each of these three files
for more information on how to use them.  In particular, you *MUST*
modify your copy of each of Makefile.sys and Makefile.user, as 
described in the comments at the top of each file.


In addition to these three Makefile-type files, this directory also
contains the source code to three simple example user-level test programs:

 - pingpong.c: This program sends a sequence of "ping-pong" messages
   back and forth between two RedNet user processes.  The command line
   must specify the number of ping-pong rounds.

 - round.c: This program does message passing in a simple round-robin
   pattern among a set of RedNet user processes.  The command line must
   specify the number of round-robin rounds and the number of processes.

 - random.c: This program does message passing in a given number of
   randomized rounds among a given number of RedNet user processs.  In
   each round, the process "driving" that round does a SendMessage to
   each other process, and each other process does a ReceiveMessage to
   receive that message.  The choice of which process drives the next
   round is made randomly.  The command line must specify the number
   of rounds and the number of processes.

Please see comments at the top of each source file, and the source code
itself, for more information on each of these three programs.

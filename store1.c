/*
 *  Simple example test program for the COMP 420 RedNet peer-to-peer
 *  storage system (Lab 2).  This program does some sample peer-to-peer
 *  Insert, Lookup, and Reclaim operations, after everyone does a
 *  Join.  See the comments in-line below for more explanation of
 *  what the program does.
 *
 ***********************************************************************
 *  YOU MUST WRITE YOUR OWN IMPLEMENTATION OF Join, Insert, Lookup,
 *  AND Reclaim.  Each of these should *ONLY* build a message and send
 *  it to the local operating system kernel (using SendMessage with a
 *  destination address of 0) to request the corresponding operation
 *  to be performed by your peer-to-peer storage system implementation
 *  that you must write in your kernel.  The actual *BEHAVIOR* of
 *  joining, inserting, looking up, or reclaiming should be done in
 *  your kernel.  See Section 3 of the Lab 2 description.
 ***********************************************************************
 *  It is recommended to put your implementation of Join, Insert,
 *  Lookup, and Reclaim in a seprate source file.  Then, edit the
 *  Makefile.user to add the corresponding .o filename at the end
 *  of the "LIBS=" line.  For example, if your implementation of
 *  these four procedures is in p2p-request.c, then the "LIBS=" line
 *  should read:
 *
 *  LIBS = $(PUBDIR)/lib/libusr420.so p2p-request.o
 *
 *  Again, each of these four procedures in this file should *ONLY*
 *  build a message and send it to the local operating system kernel
 *  to request the corresponding operation.
 ***********************************************************************
 *
 *  Run as: rednet -P 1 -N [other_flags] system_prog -- store1 ##
 *
 *  The -P 1 flag causes period HandleMessage calls once each
 *  second.  This allows you to control any periodic or timer-related
 *  functions you need your implementation of the peer-to-peer
 *  system to perform.  For consistency between everyone's
 *  implementation behavior, use -P 1 (not some other value) and
 *  use a simple counter (such as a static int) if you want to do
 *  something every 2 seconds or every 5 seconds or whatever.
 *
 *  The -N flag is necessary to start all 32 computers automatically
 *  and to generate the network topology of hop connections between
 *  them (see Section 4.2 of the RedNet introduction).  Each computer
 *  and thus its kernel starts immediately, but the user process on
 *  each of these computers starts running at a time randomly spread
 *  over the first 20 seconds after starting "rednet".
 *
 *  The ## argument at the end is automatically replaced, for each copy
 *  of the user program started, with an "index" indicating which
 *  copy of the user program this is.  This value will range from
 *  "0" to "31", respectively, for each user process.  The value is
 *  visible as a "%d" format character string as argv[1], which we
 *  parse below using atoi().
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <rednet.h>
#include <rednet-p2p.h>

nodeID Nid;
int Idx;

#define FID1    0x1111
char data1[] = "data 1 string";

#define FID2    0x9999
char data2[] = "DATA 2 LONGER STRING";

char buff[P2P_FILE_MAXSIZE];    /* largest enough for any file  */

int
main(int argc, char **argv) {
    int status;

    if (argc < 2) {
        fprintf(stderr,
                "Process index argument missing: run with ## on command line.\n");
        exit(1);
    }

    Idx = atoi(argv[1]);    /* number from 0 to 31 */
    Nid = GetNodeID();

    fprintf(stderr, "Pid %d nodeID %04x index %d\n", GetPid(), Nid, Idx);

    /*
     *  As noted above, each of the 32 computers generated by -N starts
     *  up immediately, with the kernel (system) process running.  But
     *  the user process on each starts at a time randomly spread
     *  over the first 20 seconds of execution.  We do the Join()
     *  right away, since the kernel on all of the computers should be
     *  up and running now and thus able to participate in message
     *  forwarding including an expanding ring search.
     */
    if (Idx != 0) {
        fprintf(stderr, "Process Idx %d is sleeping before Join\n", Idx);
        MilliSleep(25 * 1000);
    }
    status = Join(Nid);
    if (status != 0) {
        fprintf(stderr, "ERROR: Join returned %d!\n", status);
        exit(1);
    }
    if (Idx == 0) {
        fprintf(stderr, "Process Idx %d is sleeping after Join\n", Idx);
        MilliSleep(25 * 1000);
    }

    /*
     *  Now, for this simple test program, try to ensure that all of
     *  the 32 different computers have started up and Join'd before
     *  we proceed.  25 seconds leaves time for all of the user
     *  processes to start (they do so randomlhy over the first 20
     *  seconds) plus at least 5 seconds to complete the Join (even
     *  for a user process that starts right at the end of the
     *  20-second startup window).
     */
    MilliSleep(25 * 1000);

    /*
     *  Finally, we do a sequence of P2P storage operations:
     *
     *  - Process index 5 does two Insert operations.
     *
     *  - Process index 12 then does two corresponding Lookup operations.
     *
     *  - Process index 1 does two corresponding Reclaim operations.
     *
     *  This is just an example test program, and this same basic code
     *  can easily be modified to create arbitrary sequences of different
     *  peer-to-peer storage operations done by the various processes
     *  in the system for testing.
     */

    if (Idx == 5) {
        /*
         *  Insert two different files, at fileIDs FID1 and FID2.
         *  The Insert operation should return 0 on success.
         */
        status = Insert(FID1, data1, sizeof(data1));
        if (status != 0) {
            fprintf(stderr, "ERROR: Insert 1 returned %d!\n", status);
            exit(1);
        }
        status = Insert(FID2, data2, sizeof(data2));
        if (status != 0) {
            fprintf(stderr, "ERROR: Insert 2 returned %d!\n", status);
            exit(1);
        }
        fprintf(stderr, "There will be a delay before the Lookup tests...\n");
    }

    MilliSleep(25 * 1000);  /* allow both Inserts to finish first */

    if (Idx == 12) {
        /*
         *  Lookup the two different files we inserted above.
         *  The Lookup operation should return the length of the
         *  data on success.
         */
        status = Lookup(FID1, buff, sizeof(buff));
        if (status != sizeof(data1)) {
            fprintf(stderr, "ERROR: Lookup 1 returned %d!\n", status);
            exit(1);
        }
        if (strcmp(buff, data1) != 0) {
            fprintf(stderr, "ERROR: Lookup 1 returned wrong data!\n");
            exit(1);
        }
        status = Lookup(FID2, buff, sizeof(buff));
        if (status != sizeof(data2)) {
            fprintf(stderr, "ERROR: Lookup 2 returned %d!\n", status);
            exit(1);
        }
        if (strcmp(buff, data2) != 0) {
            fprintf(stderr, "ERROR: Lookup 2 returned wrong data!\n");
            exit(1);
        }
        fprintf(stderr, "There will be a delay before the Reclaim tests...\n");
    }

    MilliSleep(25 * 1000);  /* allow both Lookups to finish too */

    // if (Idx == 1) {
    //     /*
    //      *  Reclaim the two different files we inserted above.
    //      *  The Reclaim operation should return 0 on success.
    //      */
    //     status = Reclaim(FID1);
    //     if (status != 0) {
    //         fprintf(stderr, "ERROR: Reclaim 1 returned %d!\n", status);
    //         exit(1);
    //     }
    //     status = Reclaim(FID2);
    //     if (status != 0) {
    //         fprintf(stderr, "ERROR: Reclaim 2 returned %d!\n", status);
    //         exit(1);
    //     }
    //     fprintf(stderr, "There will be a delay before the exit...\n");
    // }

    // MilliSleep(25 * 1000);  /* allow both Reclaims to also finish */

    /*
     *  Now all the test operations should be done across all
     *  user processes, so let everyone exit now (the computer and
     *  its kernel will also shut down when we exit).
     */
    exit(0);
}

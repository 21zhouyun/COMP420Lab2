/*
 *  Simple example test program using the COMP 420 RedNet distributed
 *  system programming environment.
 *
 *  This program performs a simple "ping-pong" message exchange back
 *  and forth between two processes.  The count of ping-pongs to
 *  perform must be specified as an integer on the command line.
 *
 *  If the program finds that it is the initial user-level program
 *  created at system startup, it takes on the role of the "ping"
 *  process and creates a new process to take on the role as the
 *  "pong" process.  The values passed back and forth in the ping-pong
 *  messages are simply consecutive integer values.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <rednet.h>

void DoPingPong(int, char **);
void DoPings(int, int);
void DoPongs(int);

int
main(int argc, char **argv)
{
    int count;

    TracePrintf(1, "GetPid %d\n", GetPid());

    if (argc < 2) {
	fprintf(stderr, "ERROR: 'count' argument missing.\n");
	exit(1);
    }

    count = atoi(argv[1]);
    fprintf(stderr, "count %d\n", count);

    DoPingPong(count, argv);

    exit(0);
}

void
DoPingPong(int count, char **args)
{
    int pid, ppid;

    ppid = GetPPid();
    TracePrintf(1, "pingpong GetPPid %d\n", ppid);

    if (ppid == 0) {
	fprintf(stderr, "pingpong doing CreateProcess\n");
	pid = CreateProcess(args[0], (const char **)args);
	fprintf(stderr, "pingpong CreateProcess pid %d\n", pid);
    }

    if (ppid == 0)
	DoPings(count, pid);
    else
	DoPongs(count);
}

void
DoPings(int count, int pong_pid)
{
    int i;
    int pong;
    int status;
    int src;

    fprintf(stderr, "DoPings %d %d\n", count, pong_pid);

    for (i = 0; i < count; i++) {
	TracePrintf(10, "%d: DoPing sending to %d\n", i, pong_pid);
	status = SendMessage(pong_pid, &i, sizeof(i));
	TracePrintf(10, "%d: DoPing send status %d\n", i, status);
	if (status != 0) {
	    fprintf(stderr, "ERROR: got status %d from SendMessage\n", status);
	    exit(1);
	}

	TracePrintf(10, "%d: DoPing receiving\n", i);
	status = ReceiveMessage(&src, &pong, sizeof(pong));
	fprintf(stderr, "%d: DoPing receive status %d src %d pong %d\n",
	    i, status, src, pong);
    }
}

void
DoPongs(int count)
{
    int i;
    int ping;
    int status;
    int src;

    fprintf(stderr, "DoPongs %d\n", count);

    for (i = 0; i < count; i++) {
	TracePrintf(10, "%d: DoPong receiving\n", i);
	status = ReceiveMessage(&src, &ping, sizeof(ping));
	fprintf(stderr, "%d: DoPong receive status %d src %d ping %d\n",
	    i, status, src, ping);

	TracePrintf(10, "%d: DoPong sending to %d\n", i, src);
	status = SendMessage(src, &i, sizeof(i));
	TracePrintf(10, "%d: DoPong send status %d\n", i, status);
	if (status != 0) {
	    fprintf(stderr, "ERROR: got status %d from SendMessage\n", status);
	    exit(1);
	}
    }
}

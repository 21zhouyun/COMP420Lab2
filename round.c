/*
 *  Simple example test program using the COMP 420 RedNet distributed
 *  system programming environment.
 *
 *  This program does message passing in a simple round-robin pattern
 *  among a set of processes.
 *
 *  There should be two arguments on the command line when you run
 *  the program:
 *
 *  argv[1] = The number of complete rounds of messages to send, where
 *            each round of messages consists of one message sent by
 *            and another received by each of the processes, going
 *            from the first to the second process, from the second
 *            to the third, and so on, finally being received back at
 *            the first process.
 *
 *  argv[2] = The number of processes to use in the program.  The
 *            initial user process becomes the first of these, and
 *            that process then creates the others.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <rednet.h>

#define	MAXPROCS	16

int pids[MAXPROCS];
int self_idx;	/* index of my own pid in pids */
int next_idx;	/* index of my SendMessage destination pid in pids */

int ppid;	/* my parent process id; 0 means I am the initail process */

/*
 *  Called from the initial process.  Create the other procdesses.
 */
void
CreateOthers(int procs, const char **args)
{
    int i;
    int status;

    pids[0] = GetPid();

    TracePrintf(10, "CreateOthers procs %d args[0] %p\n", procs, args[0]);
    for (i = 0; args[i]; i++) {
	TracePrintf(30, "args %d = '%s'\n", i, args[i]);
    }

    for (i = 1; i < procs; i++) {
	TracePrintf(10, "doing CreateProcess %d\n", i);
	pids[i] = CreateProcess(args[0], (const char **)args);
	TracePrintf(10, "CreateProcess %d pid %d\n", i, pids[i]);
    }

    TracePrintf(10, "CreateOthers sending pids\n");

    for (i = 1; i < procs; i++) {
	status = SendMessage(pids[i], pids, sizeof(int) * procs);
	if (status < 0) {
	    fprintf(stderr, "ERROR: can't send pids to process %d\n", i);
	    fprintf(stderr, "SendMessage: status %d\n", status);
	    exit(1);
	}
    }

    TracePrintf(10, "CreateOthers done sending pids\n");

    self_idx = 0;
    next_idx = 1;
}

/*
 *  Called from other than the initial process.  Get the list of pids of
 *  all the processes from the initial process.
 */
void
GetPids(int procs)
{
    int pid;
    int i;
    int len;

    TracePrintf(10, "GetPids procs %d\n", procs);

    len = ReceiveMessage(&pid, pids, sizeof(pids));
    if (len < 0) {
	fprintf(stderr, "ERROR: can't get pids\n");
	fprintf(stderr, "ReceiveMessage: status %d\n", len);
	exit(1);
    }
    if (len != sizeof(int) * procs) {
	fprintf(stderr, "ERROR: got wrong length %d for pids\n", len);
	exit(1);
    }
    if (pid != ppid) {
	fprintf(stderr, "ERROR: got pids from %d, not %d\n", pid, ppid);
	exit(1);
    }

    pid = GetPid();
    for (i = 1; i < procs; i++) {
	if (pids[i] == pid) break;
    }
    if (i >= procs) {
	fprintf(stderr, "ERROR: process %d can't find my own pid %d\n",
	    i, pid);
	exit(1);
    }
    self_idx = i;
    next_idx = (i + 1) % procs;
}

/*
 *  Do the next round of message passing.
 */
void
DoRound(int n)
{
    int status;
    int len;
    int src;
    int msg;

    TracePrintf(10, "pid %d round %d\n", GetPid(), n);

    if (ppid == 0) {

	/*
	 *  Delay a bit before we initiate each new round.
	 */
	MilliSleep(500);

	msg = n;
	status = SendMessage(pids[next_idx], &msg, sizeof(msg));
	if (status < 0) {
	    fprintf(stderr, "DoRound initial pid %d SendMessage returned %d\n",
		GetPid(), status);
	    exit(1);
	}
	len = ReceiveMessage(&src, &msg, sizeof(msg));
	if (len != sizeof(msg)) {
	    fprintf(stderr,
		"DoRound initial pid %d ReceuiveMessage returned %d\n",
		GetPid(), len);
	    exit(1);
	}
	if (msg != n) {
	    fprintf(stderr, "DoRound initial pid %d got msg %d not round %d\n",
		GetPid(), msg, n);
	    exit(1);
	}
	fprintf(stderr, "initial %d received %d\n", GetPid(), msg);

    } else {

	len = ReceiveMessage(&src, &msg, sizeof(msg));
	if (len != sizeof(msg)) {
	    fprintf(stderr,
		"DoRound initial pid %d ReceuiveMessage returned %d\n",
		GetPid(), len);
	    exit(1);
	}
	if (msg != n) {
	    fprintf(stderr, "DoRound other pid %d got msg %d not round %d\n",
		GetPid(), msg, n);
	    exit(1);
	}
	fprintf(stderr, "other %d received %d\n", GetPid(), msg);
	status = SendMessage(pids[next_idx], &msg, sizeof(msg));
	if (status < 0) {
	    fprintf(stderr, "DoRound other pid %d SendMessage returned %d\n",
		GetPid(), status);
	    exit(1);
	}

    }
}

int
main(int argc, char **argv)
{
    int count;
    int procs;
    int i;

    TracePrintf(1, "GetPid %d\n", GetPid());
    TracePrintf(20, "main pid %d ppid %d: argc %d\n",
	GetPid(), GetPPid(), argc);

    ppid = GetPPid();

    /*
     *  Check and parse the command line arguments.
     */

    if (argc < 2) {
	fprintf(stderr, "ERROR: number of rounds argument missing.\n");
	exit(1);
    }

    count = atoi(argv[1]);
    TracePrintf(1, "number of rounds %d\n", count);
    if (count < 0) {
	fprintf(stderr, "ERROR: invalid number of rounds specified.\n");
	exit(1);
    }

    if (argc < 3) {
	fprintf(stderr, "ERROR: number of processes argument missing.\n");
	exit(1);
    }

    if (argc > 3) {
	fprintf(stderr, "ERROR: too many arguments supplied.\n");
	exit(1);
    }

    procs = atoi(argv[2]);
    TracePrintf(1, "number of processes %d\n", procs);
    if (procs < 2 || procs > MAXPROCS) {
	fprintf(stderr, "ERROR: invalid number of processes specified.\n");
	exit(1);
    }

    /*
     *  If we are the initial process, create the other processes.
     *  Otherwise, receive the list of pids from the initial process.
     */
    if (ppid == 0) {
	CreateOthers(procs, (const char **)argv);
    } else {
	GetPids(procs);
    }

    /*
     *  Now, everybody knows the same thing: the list of pids of all
     *  'procs' processes, and thus that process's own position in
     *  that sequence.  The only difference now is that the initial
     *  process has a GetPPid of 0, whereas the other processes have
     *  nonzero for this.
     *
     *  Finally, do the specified number of rounds.  In each round, the
     *  initial process does a SendMessage/ReceiveMessage sequence,
     *  whereas each other  process does a ReceiveMessage/SendMessage
     *  sequence.
     */
    for (i = 0; i < count; i++) {
	DoRound(i);
    }

    exit(0);
}

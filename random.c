/*
 *  Simple example test program using the COMP 420 RedNet distributed
 *  system programming environment.
 *
 *  This program does message passing in a given number of "rounds"
 *  among a given number of processs.  In each round, the process
 *  "driving" that round does a SendMessage to each other process,
 *  and each other process does a ReceiveMessage to receive that
 *  message.  The choice of which process drives the next round is
 *  made randomly.
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
int self_idx;		/* index of my own pid in pids */

struct msg {
    int n;		/* current round number for this message */
    int next_idx;	/* index of process to drive the next round */
};

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
}

/*
 *  Do the next round of message passing.  In each round, the process
 *  driving that round does a SendMessage to each other  process, and
 *  each other process does a ReceiveMessage to receive that message.
 *  The choice of which process drives the next round is made randomly.
 */
void
DoRound(int procs, int n)
{
    int status;
    int len;
    int src;
    int i;
    static struct msg msg;

    TracePrintf(10, "idx %d round %d\n", self_idx, n);

    /*
     *  Check if I am the process who should drive the next round by
     *  doing the message sending.
     */
    if (msg.next_idx == self_idx) {
	MilliSleep(500);
	/*
	 *  Generate the index of which process should forward the next
	 *  round of messages to everyone.  We generate next_idx such that
	 *  it will never not equal ourselves.
	 */
	msg.next_idx = (unsigned int)random() % (procs - 1);
	if (msg.next_idx >= self_idx) msg.next_idx++;

	/*
	 *  Send a message to everyone but ourselves.  We do the
	 *  sends starting with the process after ourselves, wrapping
	 *  around the set of pids.
	 */
	msg.n = n;
	fprintf(stderr, "idx %d sending %d next_idx %d\n",
	    self_idx, msg.n, msg.next_idx);
	i = self_idx + 1;
	while (1) {
	    i %= procs;
	    if (i == self_idx) break;
	    TracePrintf(20, "pid %d Send %d: n %d next %d\n",
		GetPid(), pids[i], msg.n, msg.next_idx);
	    status = SendMessage(pids[i], &msg, sizeof(msg));
	    if (status < 0) {
		fprintf(stderr, "DoRound pid %d SendMessage returned %d\n",
		    GetPid(), status);
		exit(1);
	    }
	    i++;
	}
    } else {
	len = ReceiveMessage(&src, &msg, sizeof(msg));
	if (len != sizeof(msg)) {
	    fprintf(stderr,
		"DoRound pid %d ReceuiveMessage returned %d\n",
		GetPid(), len);
	    exit(1);
	}
#ifdef	CHECK_IN_ORDER
	/*
	 *  Check that I receive each message in order of increasing
	 *  round number.  With the MilliSleep above before each new
	 *  batch of messages is sent, they *should* always arrive in
	 *  order, but this is not actually guaranteed by the program.
	 *  Without the MilliSleep above, it is much easier for them
	 *  to arrive out of order.  So by default, I have desabled
	 *  the check here, but feel free to enable it if you want to.
	 */
	if (msg.n != n) {
	    fprintf(stderr, "DoRound pid %d got msg %d not round %d\n",
		GetPid(), msg.n, n);
	    exit(1);
	}
#endif
	fprintf(stderr, "idx %d received %d next_idx %d\n",
	    self_idx, msg.n, msg.next_idx);
    }
}

int
main(int argc, char **argv)
{
    int count;		/* the number of rounds */
    int procs;		/* the number of processes */
    int i;

    TracePrintf(1, "GetPid %d\n", GetPid());
    TracePrintf(20, "main pid %d ppid %d: argc %d\n",
	GetPid(), GetPPid(), argc);

    ppid = GetPPid();

    /*
     *  Initialize the random number generator the same way in all
     *  processes.  The initial seed is the pid of the initial process.
     */
    if (ppid == 0)
	srandom(GetPid());
    else
	srandom(ppid);

    /*
     *  Check and parse the command line arguments.
     */

    if (argc < 2) {
	fprintf(stderr, "ERROR: number of rounds argument missing.\n");
	exit(1);
    }

    count = atoi(argv[1]);
    fprintf(stderr, "number of rounds %d\n", count);
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
    fprintf(stderr, "number of processes %d\n", procs);
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
     *  process driving that round does a SendMessage to each other
     *  process, and each other process does a ReceiveMessage to
     *  receive that message.  The choice of which process drives the
     *  next round is made randomly.
     */
    for (i = 0; i < count; i++) {
	DoRound(procs, i);
    }

    fprintf(stderr, "%d exiting\n", GetPid());

    exit(0);
}

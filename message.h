#ifndef MESSAGE_H
#define MESSAGE_H

#include <rednet-p2p.h>

const int JOIN = 0;
const int FLOOD = 1;

typedef struct message {
	int type;
} Message;

typedef struct join_message {
	int type;
	nodeID id;
} JoinMessage;

typedef struct flood_message {
	int type;
	int sequence_number;
	int hop_count;
} FloodMessage;

#endif


#ifndef MESSAGE_H
#define MESSAGE_H

#include <rednet-p2p.h>
#include <array>

const int JOIN = 0;
const int JOIN_RES = 1;
const int FLOOD = 2;
const int FLOOD_RES = 3;

typedef struct message {
    int type;
} Message;

typedef struct join_message {
    int type;
    nodeID id;
} JoinMessage;

typedef struct entry {
    nodeID id;
    int pid;
} Entry;

typedef struct join_message_res {
    int type;
    nodeID id;
    std::array<Entry, P2P_LEAF_SIZE> leaf_set;
} JoinResponseMessage;

typedef struct flood_message {
    int type;
    int sequence_number;
    int hop_count;
} FloodMessage;


#endif


#ifndef MESSAGE_H
#define MESSAGE_H

#include <rednet-p2p.h>
#include <iostream>

#include <array>

const int JOIN = 0;
const int JOIN_RES = 1;
const int FLOOD = 2;
const int FLOOD_RES = 3;
const int EXCHANGE = 4;

struct Message {
    int type;
    Message(int message_type): type(message_type) {}
};

struct JoinMessage {
    int type;
    nodeID id;
    JoinMessage(nodeID node_id): type(JOIN), id(node_id) {}
};

struct Entry {
    nodeID id;
    int pid;
    Entry(): id(0), pid(0) {}
    Entry(nodeID node_id, int process_id): id(node_id), pid(process_id) {}
    friend std::ostream& operator<< (std::ostream& out, const Entry& e);
};

struct JoinResponseMessage {
    int type;
    nodeID id;
    Entry leaf_set[P2P_LEAF_SIZE];
    JoinResponseMessage(nodeID node_id, Entry other_leaf_set[P2P_LEAF_SIZE]);
};

struct ExchangeMessage {
    int type;
    nodeID id;
    Entry leaf_set[P2P_LEAF_SIZE];
    ExchangeMessage(nodeID node_id, Entry other_leaf_set[P2P_LEAF_SIZE]);
};

struct FloodMessage {
    int type;
    int sequence_number;
    int hop_count;
    FloodMessage(int s, int h): type(FLOOD), sequence_number(s), hop_count(h) {}
};


#endif


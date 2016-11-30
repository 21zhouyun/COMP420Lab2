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
    std::array<Entry, P2P_LEAF_SIZE> leaf_set;
    JoinResponseMessage(nodeID node_id, std::array<Entry, P2P_LEAF_SIZE> other_leaf_set):
        type(JOIN_RES), id(node_id), leaf_set(other_leaf_set) {}
};

struct ExchangeMessage {
    int type;
    nodeID id;
    std::array<Entry, P2P_LEAF_SIZE> leaf_set;
    ExchangeMessage(nodeID node_id, std::array<Entry, P2P_LEAF_SIZE> other_leaf_set):
        type(EXCHANGE), id(node_id), leaf_set(other_leaf_set) {}
};

struct FloodMessage {
    int type;
    int sequence_number;
    int hop_count;
    FloodMessage(int s, int h): type(FLOOD), sequence_number(s), hop_count(h) {}
};


#endif


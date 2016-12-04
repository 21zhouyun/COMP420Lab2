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
const int EXCHANGE_RES = 5;
const int INSERT = 6;
const int INSERT_CONFIRM = 7;
const int REPLICATE = 8;
const int REPLICATE_CONFIRM = 9;
const int LOOK_UP = 10;
const int LOOK_UP_CONFIRM = 11;
const int LOOK_UP_FAIL = 12;
const int RECLAIM = 13;
const int RECLAIM_CONFIRM = 14;
const int RECLAIM_FAIL = 15;
const int RECLAIM_REPLICATE = 16;
const int RECLAIM_REPLICATE_CONFIRM = 17;

const int data_message_header_size = sizeof(int) + sizeof(fileID);

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

struct ExchangeResponseMessage {
    int type;
    nodeID id;
    Entry leaf_set[P2P_LEAF_SIZE];
    ExchangeResponseMessage(nodeID node_id, Entry other_leaf_set[P2P_LEAF_SIZE]);
};

struct FloodMessage {
    int type;
    int sequence_number;
    int hop_count;
    FloodMessage(int s, int h): type(FLOOD), sequence_number(s), hop_count(h) {}
};

struct ReplicateConfirmMessage {
    int type;
    fileID fid;
    ReplicateConfirmMessage(fileID file_id): type(REPLICATE_CONFIRM), fid(file_id) {}
};

struct LookupMessage {
    int type;
    fileID fid;
    int len;
    LookupMessage(fileID file_id, int length): type(LOOK_UP), fid(file_id), len(length) {}
};

/**
 * This message is used for difference steps of reclaiming a file based on the type given.
 */
struct FileMessage {
    int type;
    fileID fid;
    FileMessage(int message_type, fileID file_id): type(message_type), fid(file_id) {}
};

/**
 * Insert message format:
 * int type
 * fileID fid
 * char[len]
 *
 * @param  fid      fileID
 * @param  contents content of the file
 * @param  len      length of the content to copy to the buffer
 * @return          allocated Insert message
 */
char* MakeDataMessage(fileID fid, void* contents, int len, int type);
int ParseDataMessageHeader(const void* msg, int len, fileID* fid);
int ParseDataMessageContent(const void* msg, int len, char* buf, int buf_len);

#endif


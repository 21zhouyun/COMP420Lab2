#include "message.h"
#include <cstring>
#include <algorithm>

std::ostream& operator<< (std::ostream& out, const Entry& e) {
    return out << "(" << e.id << "," << e.pid << ")";
}

JoinResponseMessage::JoinResponseMessage(nodeID node_id, Entry other_leaf_set[P2P_LEAF_SIZE]):
    type(JOIN_RES), id(node_id) {
    std::copy(other_leaf_set, other_leaf_set + P2P_LEAF_SIZE, leaf_set);
}

ExchangeMessage::ExchangeMessage(nodeID node_id, Entry other_leaf_set[P2P_LEAF_SIZE]):
    type(EXCHANGE), id(node_id) {
    std::copy(other_leaf_set, other_leaf_set + P2P_LEAF_SIZE, leaf_set);
}

char* MakeDataMessage(fileID fid, void* contents, int len, int type) {
    int offset = 0;
    char* message = new char[data_message_header_size + len * sizeof(char)];
    std::memcpy(message + offset, &type, sizeof(int));
    offset += sizeof(int);
    std::memcpy(message + offset, &fid, sizeof(fileID));
    offset += sizeof(fileID);
    std::memcpy(message + offset, contents, len * sizeof(char));
    offset += len * sizeof(char);
    return message;
}

int ParseDataMessageHeader(const void* msg, int len, fileID* fid) {
    // skip type
    int offset = sizeof(int);
    std::memcpy(fid, msg + offset, sizeof(fileID));
    offset += sizeof(fileID);
    return offset;
}

int ParseDataMessageContent(const void* msg, int len, char* buf, int buf_len) {
    // skip type and fid
    int offset = data_message_header_size;
    std::memcpy(buf, msg + offset, buf_len * sizeof(char));
    offset += buf_len * sizeof(char);
    return offset;
}
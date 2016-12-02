#include "message.h"
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
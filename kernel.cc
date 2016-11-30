#include <rednet.h>
#include <unordered_map>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <array>

#include "message.h"

const int NORMAL = 0;
const int RINGSEARCH = 1;
const int JOINING = 2;

/**
 * Mode of kernel.
 */
int mode = 0;

std::unordered_map<int, int> sequence_number_map;

/**
 * Variables for expanding ring search.
 */
int sequence_number = 0;
int hop_count = 0;

/**
 * Overlay network.
 */
bool joined_overlay_network = false;
nodeID node_id;
const unsigned short RING_SIZE = 65535;

std::array<Entry, P2P_LEAF_SIZE> leaf_set = {{{0, 0}, {0, 0}, {0, 0}, {0, 0}}};

void HandleJoinMessage(int src, int dest, const void *msg, int len);
void HandleJoinResponseMessage(int src, int dest, const void *msg, int len);
void RingSearch(int src, int sequence_number, int hop_count);
void HandleFloodMessage(int src, int dest, const void *msg, int len);
void HandleFloodResponseMessage(int src, int dest, const void *msg, int len);

unsigned short distance(nodeID x, nodeID y);

void HandleMessage(int src, int dest, const void *msg, int len) {
    int pid = GetPid();

    if (src == 0 && dest == 0 && len == 0) {
        // periodic alarm
        if (mode == RINGSEARCH) {
            // ring search timeout, increase ring size and redo ring search
            RingSearch(GetPid(), ++sequence_number, ++hop_count);
        }
    } else if (src == pid && dest != 0) {
        // TODO: send message
        TracePrintf(10, "Send message from %d to %d\n", src, dest);
    } else if (src == dest || dest == 0) {
        // Received message
        Message* message = (Message*) msg;
        switch (message->type) {
        case JOIN: {
            HandleJoinMessage(src, dest, msg, len);
            break;
        }
        case JOIN_RES: {
            HandleJoinResponseMessage(src, dest, msg, len);
            break;
        }
        case FLOOD: {
            HandleFloodMessage(src, dest, msg, len);
            break;
        }
        case FLOOD_RES: {
            HandleFloodResponseMessage(src, dest, msg, len);
            break;
        }
        default:
            std::cerr << "Unknown message type: " << message->type << std::endl;
            break;
        }
    }
}

void HandleJoinMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "Received join message from %d", src);
    JoinMessage* message = (JoinMessage*) msg;
    if (GetPid() == src) {
        // this is the initial join message
        node_id = message->id;
        RingSearch(GetPid(), ++sequence_number, ++hop_count);
    } else {
        // this is the join message from some other node that is
        // not in the overlay network. Route it.
        int next_hop = GetPid();
        unsigned short min_diff = distance(message->id, node_id);
        for (Entry e : leaf_set) {
            if (e.pid > 0 && distance(e.id, message->id) < min_diff) {
                next_hop = e.pid;
            }
        }
        if (next_hop == GetPid()) {
            // current node is the closest node
            JoinResponseMessage* reply = new JoinResponseMessage;
            reply->type = JOIN_RES;
            reply->id = node_id;
            reply->leaf_set = leaf_set; // this should copy the array
            if (TransmitMessage(GetPid(), src, reply, sizeof(reply)) < 0) {
                std::cerr << "Fail to send join response message from "
                          << GetPid() << " to " << src << std::endl;
            }
            delete reply;
        } else {
            // forward join message to next node
            if (TransmitMessage(src, next_hop, msg, len) < 0) {
                std::cerr << "Fail to forward join message from "
                          << src << " to " << next_hop << std::endl;
            }
        }
    }
}

void HandleJoinResponseMessage(int src, int dest, const void *msg, int len) {
    if (mode == JOINING) {
        TracePrintf(10, "Received join response message from %d\n", src);
        mode = NORMAL;
        JoinResponseMessage* message = (JoinResponseMessage*) msg;
        joined_overlay_network = true;
        // TODO: create own leaf set
    }
}

void RingSearch(int src, int sequence_number, int hop_count) {
    TracePrintf(10, "RingSearch with sequence number %d and hop count %d\n",
                sequence_number, hop_count);
    mode = RINGSEARCH;
    FloodMessage* message = new FloodMessage;
    message->type = FLOOD;
    message->sequence_number = sequence_number;
    message->hop_count = hop_count;
    TransmitMessage(src, -1, message, sizeof(message));
    delete message;
}

void HandleFloodMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "Received flood message from %d\n", src);
    int pid = GetPid();
    FloodMessage* fmessage = (FloodMessage*) msg;
    if (sequence_number_map.find(src) != sequence_number_map.end()
            && fmessage->sequence_number <= sequence_number_map[src]) {
        return;
    }
    sequence_number_map[src] = fmessage->sequence_number;
    // process the message
    if (joined_overlay_network) {
        // we are part of the overlay, reply to the src
        Message* reply = new Message;
        reply->type = FLOOD_RES;
        if (TransmitMessage(pid, src, reply, sizeof(reply)) < 0) {
            std::cerr << "Failed to send reply to flood message from "
                      << pid << " to " << src << std::endl;
        }
        delete reply;
    } else {
        if (--(fmessage->hop_count) == 0) {
            return;
        }
        if (TransmitMessage(src, -1, fmessage, len) < 0) {
            std::cerr << "Failed to forward flood message from " << src << std::endl;
        }
    }
}

void HandleFloodResponseMessage(int src, int dest, const void *msg, int len) {
    if (mode == RINGSEARCH) {
        TracePrintf(10, "Received flood response message from %d\n", src);
        mode = JOINING;

        JoinMessage* message = new JoinMessage;
        message->type = JOIN;
        message->id = node_id;
        if (TransmitMessage(GetPid(), src, message, sizeof(message)) < 0) {
            std::cerr << "Fail to send join message from "
                      << GetPid() << " to " << src << std::endl;
        }
        delete message;
    }
}


unsigned short distance(nodeID x, nodeID y) {
    return std::min(std::abs(x - y), RING_SIZE - std::abs(x - y));
}
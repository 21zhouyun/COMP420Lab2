#include <rednet.h>
#include <unordered_map>
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <array>
#include <utility>
#include <iterator>
#include <set>

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
const int hop_count_limit = 5;

int sequence_number = 0;
int hop_count = 0;
int alarm_round = 0;

/**
 * Overlay network.
 */
bool joined_overlay_network = false;
nodeID node_id;
const int RING_SIZE = 65536;

Entry leaf_set[P2P_LEAF_SIZE] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
// nodes in leaf set where I haven't get response from an exchange message
std::set<nodeID> dead_node;
/**
 * Storage
 */
// fileID to <file, file_len> pair
std::unordered_map<fileID, std::pair<char*, int>> file_map;
// fileID to <pid, wait_count> pair
std::unordered_map<fileID, std::pair<int, int>> confirmation_waiting_map;

void HandleJoinMessage(int src, int dest, const void *msg, int len);
void HandleJoinResponseMessage(int src, int dest, const void *msg, int len);
void RingSearch(int src, int sequence_number, int hop_count);
void HandleFloodMessage(int src, int dest, const void *msg, int len);
void HandleFloodResponseMessage(int src, int dest, const void *msg, int len);
void HandleExchangeMessage(int src, int dest, const void *msg, int len);
void HandleExchangeResponseMessage(int src, int dest, const void *msg, int len);
void HandleInsertMessage(int src, int dest, const void *msg, int len);
void HandleReplicateMessage(int src, int dest, const void *msg, int len);
void HandleLookupMessage(int src, int dest, const void *msg, int len);
void HandleReclaimMessage(int src, int dest, const void *msg, int len);
void HandleReclaimReplicateMessage(int src, int dest, const void *msg, int len);

/**
 * Route a given message to a destination in the overlay network
 * @param src  original source of the message
 * @param dest destination node id
 * @param msg  raw message
 * @param len  length of the message
 * @param type type of the message
 */
void Route(int src, nodeID dest, const void *msg, int len, int type);

/**
 * The distance from x(smaller) to y(larger).
 * @param  x smaller node id
 * @param  y larger node id
 * @return   distance from x to y
 */
unsigned short Distance(nodeID x, nodeID y);

/**
 * The numeric distance between x and y. Unlike Distance(x, y),
 * the absolute distance is symmetric. This metric is used for
 * routing
 * @param  x node id of first node
 * @param  y node id of second node
 * @return   absolute distance between x and y
 */
unsigned short AbsoluteDistance(nodeID x, nodeID y);

/**
 * Update leaf set of this node
 * @param id  the new node id to consider
 * @param src the pid of the node with the new node id
 */
void UpdateLeafSet(nodeID id, int src);

/**
 * Update the upperhalf of leaf set of this node
 * @param id  the new node id to consider
 * @param src the pid of the node with the new node id
 */
void UpdateUpperLeafSet(nodeID id, int src);

/**
 * Remove the node with the given node id from leaf set
 * @param id node id of the node to remove
 */
void RemoveNodeFromLeafSet(nodeID id);

/**
 * Print leaf set with TracePrintf()
 */
void PrintLeafSet();

void HandleMessage(int src, int dest, const void *msg, int len) {
    int pid = GetPid();

    if (src == 0 && dest == 0 && len == 0) {
        // periodic alarm, only handle alarm every 2 period
        if (alarm_round % 2 == 0) {
            switch (mode) {
            case RINGSEARCH: {
                // ring search timeout, increase ring size and redo ring search
                // https://piazza.com/class/is5hhwlricz17p?cid=51
                if (hop_count < hop_count_limit) {
                    RingSearch(GetPid(), ++sequence_number, ++hop_count);
                } else {
                    // we cannot find any existing node, assume we are the first node
                    mode = NORMAL;
                    joined_overlay_network = true;
                    // confirm join
                    int status = 0;
                    DeliverMessage(src, GetPid(), &status, sizeof(int));
                    std::cerr << GetPid() << " joined network as first node" << std::endl;
                }
                break;
            }
            case NORMAL: {
                // remove dead node from leaf set
                for (nodeID id : dead_node) {
                    RemoveNodeFromLeafSet(id);
                }
                PrintLeafSet();
                ExchangeMessage* message = new ExchangeMessage(node_id, leaf_set);
                for (const auto &e : leaf_set) {
                    dead_node.insert(e.id);
                    if (e.pid > 0 && TransmitMessage(GetPid(), e.pid, message, sizeof(ExchangeMessage)) < 0) {
                        std::cerr << "Fail to send exchange message from "
                                  << GetPid() << " to " << e.pid << std::endl;
                    }
                }
                delete message;
                break;
            }
            }
        }
        alarm_round = (alarm_round + 1) % 2;
    } else if (src == pid && dest != 0) {
        // TODO: send message
        TracePrintf(10, "Send message from %d to %d\n", src, dest);
    } else if (pid == dest || dest == 0 || dest == -1) {
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
        case EXCHANGE: {
            HandleExchangeMessage(src, dest, msg, len);
            break;
        }
        case EXCHANGE_RES: {
            HandleExchangeResponseMessage(src, dest, msg, len);
            break;
        }
        case INSERT: {
            HandleInsertMessage(src, dest, msg, len);
            break;
        }
        case INSERT_CONFIRM: {
            TracePrintf(10, "Received insert confirmation message from %d\n", src);
            // forward confirmation
            int status = 0;
            DeliverMessage(src, GetPid(), &status, sizeof(int));
            break;
        }
        case REPLICATE: {
            HandleReplicateMessage(src, dest, msg, len);
            break;
        }
        case REPLICATE_CONFIRM: {
            TracePrintf(10, "Received replicate confirmation message from %d\n", src);
            ReplicateConfirmMessage* message = (ReplicateConfirmMessage*) msg;
            confirmation_waiting_map[message->fid].second--;
            TracePrintf(10, "Still need %d confirmations\n", confirmation_waiting_map[message->fid].second);
            if (confirmation_waiting_map[message->fid].second == 0) {
                // send insert confirmation message
                if (GetPid() == confirmation_waiting_map[message->fid].first) {
                    // current node is the destination
                    int status = 0;
                    DeliverMessage(GetPid(), GetPid(), &status, sizeof(int));
                } else {
                    TracePrintf(10, "Send insert confirm from %d to %d\n", GetPid(), confirmation_waiting_map[message->fid].first);
                    Message* reply = new Message(INSERT_CONFIRM);
                    if (TransmitMessage(GetPid(), confirmation_waiting_map[message->fid].first, reply, sizeof(Message)) < 0) {
                        std::cerr << "Fail to send insert confirmation from "
                                  << GetPid() << " to " << confirmation_waiting_map[message->fid].first << std::endl;
                    }
                    delete reply;
                }
            }
            break;
        }
        case LOOK_UP: {
            HandleLookupMessage(src, dest, msg, len);
            break;
        }
        case LOOK_UP_CONFIRM: {
            TracePrintf(10, "Received look up response from %d of length %d\n", src, len);
            int status = 0;
            // we get the file
            int file_len = len - data_message_header_size;
            status = file_len;
            char* buf = new char[file_len];
            ParseDataMessageContent(msg, len, buf, file_len);
            DeliverMessage(src, dest, &status, sizeof(int));
            DeliverMessage(src, dest, buf, file_len);
            delete[] buf;
            break;
        }
        case LOOK_UP_FAIL: {
            int status = -1;
            DeliverMessage(src, dest, &status, sizeof(int));
            break;
        }
        case RECLAIM: {
            HandleReclaimMessage(src, dest, msg, len);
            break;
        }
        case RECLAIM_CONFIRM: {
            TracePrintf(10, "Received reclaim confirmation message from %d\n", src);
            // forward confirmation
            int status = 0;
            DeliverMessage(src, GetPid(), &status, sizeof(int));
            break;
        }
        case RECLAIM_FAIL: {
            TracePrintf(10, "Received reclaim fail message from %d\n", src);
            int status = -1;
            DeliverMessage(src, GetPid(), &status, sizeof(int));
            break;
        }
        case RECLAIM_REPLICATE: {
            HandleReclaimReplicateMessage(src, dest, msg, len);
            break;
        }
        case RECLAIM_REPLICATE_CONFIRM: {
            TracePrintf(10, "Received reclaim replicate confirmation message from %d\n", src);
            FileMessage* message = (FileMessage*) msg;
            confirmation_waiting_map[message->fid].second--;
            TracePrintf(10, "Still need %d confirmations\n", confirmation_waiting_map[message->fid].second);
            if (confirmation_waiting_map[message->fid].second == 0) {
                if (GetPid() == confirmation_waiting_map[message->fid].first) {
                    //current node is the destination
                    int status = 0;
                    DeliverMessage(GetPid(), GetPid(), &status, sizeof(int));
                } else {
                    // send reclaim confirmation message
                    Message* reply = new Message(RECLAIM_CONFIRM);
                    if (TransmitMessage(GetPid(), confirmation_waiting_map[message->fid].first, reply, sizeof(Message)) < 0) {
                        std::cerr << "Fail to send reclaim confirmation from "
                                  << GetPid() << " to " << confirmation_waiting_map[message->fid].first << std::endl;
                    }
                    delete reply;
                }
            }
            break;
        }
        default:
            std::cerr << "Unknown message type: " << message->type << std::endl;
            break;
        }
    }
}

void HandleJoinMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "Received join message from %d\n", src);
    JoinMessage* message = (JoinMessage*) msg;
    if (GetPid() == src) {
        // this is the initial join message
        node_id = message->id;
        RingSearch(GetPid(), ++sequence_number, ++hop_count);
    } else {
        // this is the join message from some other node that is
        // not in the overlay network. Route it.
        Route(src, message->id, msg, len, JOIN);
    }
}

void HandleJoinResponseMessage(int src, int dest, const void *msg, int len) {
    if (mode == JOINING) {
        TracePrintf(10, "Received join response message from %d\n", src);
        mode = NORMAL;
        JoinResponseMessage* message = (JoinResponseMessage*) msg;
        joined_overlay_network = true;
        std::copy(message->leaf_set, message->leaf_set + P2P_LEAF_SIZE, leaf_set);
        UpdateLeafSet(message->id, src);

        // confirm join
        int status = 0;
        DeliverMessage(src, GetPid(), &status, sizeof(int));
        std::cerr << GetPid() << " joined by attaching to " << src << std::endl;
    }
}

void RingSearch(int src, int sequence_number, int hop_count) {
    TracePrintf(10, "RingSearch with sequence number %d and hop count %d\n",
                sequence_number, hop_count);
    mode = RINGSEARCH;
    FloodMessage* message = new FloodMessage(sequence_number, hop_count);
    if (TransmitMessage(src, -1, message, sizeof(FloodMessage)) < 0) {
        std::cerr << "Fail to send flood message from " << src << std::endl;
    }
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
        TracePrintf(10, "Response to flood message from %d\n", src);
        // we are part of the overlay, reply to the src
        Message* reply = new Message(FLOOD_RES);
        if (TransmitMessage(pid, src, reply, sizeof(Message)) < 0) {
            std::cerr << "Failed to send reply to flood message from "
                      << pid << " to " << src << std::endl;
        }
        delete reply;
    } else {
        if (--(fmessage->hop_count) == 0) {
            return;
        }
        TracePrintf(10, "Forward flood message from %d\n", src);
        if (TransmitMessage(src, -1, fmessage, len) < 0) {
            std::cerr << "Failed to forward flood message from " << src << std::endl;
        }
    }
}

void HandleFloodResponseMessage(int src, int dest, const void *msg, int len) {
    if (mode == RINGSEARCH) {
        TracePrintf(10, "Received flood response message from %d\n", src);
        mode = JOINING;

        JoinMessage* message = new JoinMessage(node_id);
        if (TransmitMessage(GetPid(), src, message, sizeof(JoinMessage)) < 0) {
            std::cerr << "Fail to send join message from "
                      << GetPid() << " to " << src << std::endl;
        }
        delete message;
    } else {
        TracePrintf(10, "Discard flood response message from %d. Current mode %d\n", src, mode);
    }
}

void HandleExchangeMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "Received exchange message from %d\n", src);
    ExchangeMessage* message = (ExchangeMessage*) msg;
    // send back reply before update
    ExchangeResponseMessage* reply = new ExchangeResponseMessage(node_id, leaf_set);
    if (TransmitMessage(GetPid(), src, reply, sizeof(ExchangeResponseMessage)) < 0) {
        std::cerr << "Fail to send exchange reply from " << GetPid()
                  << " to " << src << std::endl;
    }
    delete reply;

    //Update leaf set based on the information
    UpdateLeafSet(message->id, src);

    // we received an exchange message from a dead node
    // bring it back to life
    dead_node.erase(message->id);

    for (Entry e : message->leaf_set) {
        // only update node we know that is not dead
        // to avoid "ghost" effect where a removed node
        // is added back from dated exchange message
        // by other nodes
        if (dead_node.find(e.id) == dead_node.end()) {
            UpdateLeafSet(e.id, e.pid);
        }
    }
}

void HandleExchangeResponseMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "Received exchange response message from %d\n", src);
    ExchangeResponseMessage* message = (ExchangeResponseMessage*) msg;
    dead_node.erase(message->id);
    //Update leaf set based on the information
    UpdateLeafSet(message->id, src);
    for (Entry e : message->leaf_set) {
        UpdateLeafSet(e.id, e.pid);
    }
}

void HandleInsertMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "%04x received insert message from %d\n", node_id, src);
    fileID fid = 0;
    ParseDataMessageHeader(msg, len, &fid);
    Route(src, fid, msg, len, INSERT);
}

void HandleReplicateMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "Received replicate message from %d\n", src);
    fileID fid = 0;
    ParseDataMessageHeader(msg, len, &fid);
    int file_len = len - data_message_header_size;
    char* data = new char[file_len];
    ParseDataMessageContent(msg, len, data, file_len);
    if (file_map.find(fid) != file_map.end()) {
        // free existing file
        char* old_file = file_map[fid].first;
        delete[] old_file;
        file_map.erase(fid); // this is probably not needed
    }
    file_map[fid] = std::make_pair(data, file_len);
    TracePrintf(10, "Store(replicate) file %d of size %d at pid: %d nodeID: %d content: %s\n",
                fid, file_len, GetPid(), node_id, data);
    ReplicateConfirmMessage* message = new ReplicateConfirmMessage(fid);
    if (TransmitMessage(GetPid(), src, message, sizeof(Message)) < 0) {
        std::cerr << "Fail to send replicate confirmation from "
                  << GetPid() << " to " << src << std::endl;
    }
    delete[] message;
}

void HandleLookupMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "%04x received lookup message from %d\n", node_id, src);
    LookupMessage* message = (LookupMessage*) msg;
    Route(src, message->fid, msg, len, LOOK_UP);
}

void HandleReclaimMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "Received reclaim message from %d\n", src);
    FileMessage* message = (FileMessage*) msg;
    Route(src, message->fid, msg, len, RECLAIM);
}

void HandleReclaimReplicateMessage(int src, int dest, const void *msg, int len) {
    TracePrintf(10, "%04x received reclaim replicate message from %d\n", node_id, src);
    FileMessage* message = (FileMessage*) msg;
    fileID fid = message->fid;
    if (file_map.find(fid) != file_map.end()) {
        // free existing file
        TracePrintf(10 , "Find file %hu to reclaim at %d\n", fid, GetPid());
        char* old_file = file_map[dest].first;
        delete[] old_file;
        file_map.erase(dest); // this is probably not needed
    }
    // send back confirmation
    FileMessage* reply = new FileMessage(RECLAIM_REPLICATE_CONFIRM, fid);
    if (TransmitMessage(GetPid(), src, reply, sizeof(FileMessage)) < 0) {
        std::cerr << "Fail to send reclaim replicate confirmation"
                  << " from " << GetPid() << " to " << src;
    }
    delete reply;
}

void Route(int src, nodeID dest, const void *msg, int len, int type) {
    int next_hop = GetPid();
    // first treat dest as smaller than current node
    unsigned short min_distance = AbsoluteDistance(dest, node_id);
    for (const auto &e : leaf_set) {
        if (e.pid > 0 && AbsoluteDistance(e.id, dest) < min_distance) {
            next_hop = e.pid;
            min_distance = AbsoluteDistance(e.id, dest);
        }
    }

    if (next_hop == GetPid()) {
        // current node is the closest node
        // handle this message
        switch (type) {
        case JOIN: {
            // reply to new node's join request
            JoinResponseMessage* reply = new JoinResponseMessage(node_id, leaf_set);
            if (TransmitMessage(GetPid(), src, reply, sizeof(JoinResponseMessage)) < 0) {
                std::cerr << "Fail to send join response message from "
                          << GetPid() << " to " << src << std::endl;
            }
            delete reply;

            // then update my leaf set since I see a new node
            // this has to be done after sending the reply because otherwise
            // the new node might see itself in the leaf set
            UpdateLeafSet(dest, src);
            break;
        }
        case INSERT: {
            // store the file
            fileID fid = (fileID) dest;
            int file_len = len - data_message_header_size;
            char* data = new char[file_len];
            ParseDataMessageContent(msg, len, data, file_len);
            if (file_map.find(fid) != file_map.end()) {
                // free existing file
                char* old_file = file_map[fid].first;
                delete[] old_file;
                file_map.erase(fid); // this is probably not needed
            }
            TracePrintf(10, "Store file %d of size %d at pid: %d nodeID: %04x content: %s\n",
                        fid, file_len, GetPid(), node_id, data);
            file_map[fid] = std::make_pair(data, file_len);

            // send copy to 2 other node
            char* message = MakeDataMessage(fid, data, file_len, REPLICATE);
            int left_neighbor = leaf_set[P2P_LEAF_SIZE / 2 - 1].pid;
            int right_neighbor = leaf_set[P2P_LEAF_SIZE / 2].pid;
            int num_replicate = 0;
            if (left_neighbor != 0) {
                num_replicate++;
                if (TransmitMessage(GetPid(), left_neighbor, message, len) < 0) {
                    std::cerr << "Fail to forward insert message from "
                              << src << " to " << left_neighbor << std::endl;
                }
            }
            if (right_neighbor != 0) {
                num_replicate++;
                if (TransmitMessage(GetPid(), right_neighbor, message, len) < 0) {
                    std::cerr << "Fail to forward insert message from "
                              << src << " to " << right_neighbor << std::endl;
                }
            }
            TracePrintf(10, "Send %d replicates to neighbor\n", num_replicate);
            confirmation_waiting_map[fid] = std::make_pair(src, num_replicate);
            delete[] message;
            break;
        }
        case LOOK_UP: {
            LookupMessage* message = (LookupMessage*) msg;
            fileID fid = message->fid;
            int buf_len = message->len;
            if (file_map.find(fid) != file_map.end()) {
                // send back the found file
                int file_len = std::min(buf_len, file_map[fid].second);
                TracePrintf(10, "Find file %d of size %d at pid: %d nodeID: %04x content %s\n",
                            fid, file_len, GetPid(), node_id, file_map[fid].first);
                char* reply = MakeDataMessage(fid, file_map[fid].first, file_len, LOOK_UP_CONFIRM);
                if (TransmitMessage(GetPid(), src, reply,
                                    data_message_header_size + file_len) < 0) {
                    std::cerr << "Fail to reply to look up message from " << src << std::endl;
                }
                delete[] reply;
            } else {
                // we don't have the file, send back response message without content
                TracePrintf(10, "Cannot find file %d\n", fid);
                Message* reply = new Message(LOOK_UP_FAIL);
                if (TransmitMessage(GetPid(), src, reply, sizeof(Message)) < 0) {
                    std::cerr << "Fail to reply to look up message from " << src << std::endl;
                }
                delete[] reply;
            }
            break;
        }
        case RECLAIM: {
            FileMessage* message = (FileMessage*) msg;
            fileID fid = message->fid;
            if (file_map.find(fid) != file_map.end()) {
                // free existing file
                TracePrintf(10 , "Find file %hu to reclaim at pid: %d nodID: %04x\n", fid, GetPid(), node_id);
                char* old_file = file_map[fid].first;
                delete[] old_file;
                file_map.erase(fid); // this is probably not needed

                // send reclaim replicate to neighbor
                FileMessage* reclaim_replicate_message = new FileMessage(RECLAIM_REPLICATE, fid);
                int left_neighbor = leaf_set[P2P_LEAF_SIZE / 2 - 1].pid;
                int right_neighbor = leaf_set[P2P_LEAF_SIZE / 2].pid;
                int num_replicate = 0;
                if (left_neighbor != 0) {
                    num_replicate++;
                    if (TransmitMessage(GetPid(), left_neighbor, reclaim_replicate_message, sizeof(FileMessage)) < 0) {
                        std::cerr << "Fail to forward reclaim replicate message from "
                                  << src << " to " << left_neighbor << std::endl;
                    }
                }
                if (right_neighbor != 0) {
                    num_replicate++;
                    if (TransmitMessage(GetPid(), right_neighbor, reclaim_replicate_message, sizeof(FileMessage)) < 0) {
                        std::cerr << "Fail to forward reclaim replicate message from "
                                  << src << " to " << right_neighbor << std::endl;
                    }
                }
                // TODO: Using the same map might result in some problem when one
                // node is inserting a file and another node is reclaiming the same
                // file.
                confirmation_waiting_map[fid] = std::make_pair(src, num_replicate);
                delete reclaim_replicate_message;
            } else {
                // we couldn't find the file to reclaim
                Message* reply = new Message(RECLAIM_FAIL);
                if (TransmitMessage(GetPid(), src, reply, sizeof(Message)) < 0) {
                    std::cerr << "Fail to send reclaim fail message from "
                              << GetPid() << " to " << src << std::endl;
                }
            }
            break;
        }
        default: {
            std::cerr << "Unknown message type to route: " << type << std::endl;
        }
        }
    } else {
        // forward message to next node
        if (TransmitMessage(src, next_hop, msg, len) < 0) {
            std::cerr << "Fail to forward join message from "
                      << src << " to " << next_hop << std::endl;
        }
    }
}

unsigned short Distance(nodeID x, nodeID y) {
    if (x <= y) {
        return y - x;
    } else {
        return RING_SIZE - x + y;
    }
}

unsigned short AbsoluteDistance(nodeID x, nodeID y) {
    return std::min(std::abs(x - y), RING_SIZE - std::abs(x - y));
}

void UpdateLeafSet(nodeID id, int src) {
    TracePrintf(10, "Update leaf set (%d,%d)\n", id, src);
    int distance;
    int max_distance;
    int index;

    if (node_id == id) {
        return;
    }

    // if id is already in leaf set, do nothing
    // this is slow for large leaf set
    for (const auto &e : leaf_set) {
        if (e.id == id) {
            return;
        }
    }

    // Greedy algorithm
    // 1. try to treat the given nodeID as smaller than the current node_id
    // 1.a if there is an empty spot in the lower half, just fill it
    // 2. if the smaller half of the leaf set is already filled,
    // see if we are closer to any of those nodes
    // 2.a if we are closer, pick the node in the smaller half that is most distant
    // from current node and replace it. Then try to fit the replaced node into larger half
    // 2.b if not, treat the given nodeID as larger than the current node_id and
    // try to fit it into the larger half
    distance = Distance(id, node_id);
    max_distance = distance;
    for (int i = 0; i < P2P_LEAF_SIZE / 2; i++) {
        Entry* e = &leaf_set[i];
        if (e->pid == 0) {
            // 1.a
            e->id = id;
            e->pid = src;
            return;
        } else if (Distance(e->id, node_id) > max_distance) {
            max_distance = Distance(e->id, node_id);
            index = i;
        }
    }
    if (max_distance > distance) {
        // 2.a
        UpdateUpperLeafSet(leaf_set[index].id, leaf_set[index].pid);
        leaf_set[index].id = id;
        leaf_set[index].pid = src;
    } else {
        UpdateUpperLeafSet(id, src);
    }
}

void UpdateUpperLeafSet(nodeID id, int src) {
    int distance;
    int max_distance;
    int index;

    distance = Distance(node_id, id);
    max_distance = distance;
    for (int i = P2P_LEAF_SIZE / 2; i < P2P_LEAF_SIZE; i++) {
        Entry* e = &leaf_set[i];
        if (e->pid == 0) {
            e->id = id;
            e->pid = src;
            return;
        } else if (Distance(node_id, e->id) > max_distance) {
            max_distance = Distance(node_id, e->id);
            index = i;
        }
    }
    if (max_distance > distance) {
        leaf_set[index].id = id;
        leaf_set[index].pid = src;
    }
}

void RemoveNodeFromLeafSet(nodeID id) {
    TracePrintf(10, "Remove dead node %04x from leaf set\n", id);
    for (auto &e : leaf_set) {
        if (e.id == id) {
            e.id = 0;
            e.pid = 0;
        }
    }
}

void PrintLeafSet() {
    TracePrintf(10, "node_id: %04x, leaf_set: (%d), (%d), (%d), (%d)\n",
                node_id, leaf_set[0].id, leaf_set[1].id, leaf_set[2].id, leaf_set[3].id);
}
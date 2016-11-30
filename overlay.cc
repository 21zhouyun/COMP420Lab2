#include <rednet.h>
#include <rednet-p2p.h>
#include <iostream>

#include "message.h"

/**
 * Join the p2p storage system.
 * @param  id [the nodeID to use]
 * @return    [status of the join]
 */
int Join(nodeID id) {
    TracePrintf(10, "Forward join request from nodeID %hu\n", id);
    JoinMessage* message = new JoinMessage;
    message->type = JOIN;
    message->id = id;
    SendMessage(0, message, sizeof(message));
    delete message;
    return 0;
}

/**
 * Store a file in the p2p storage system.
 * @param  fid      fileID
 * @param  contents content of the file
 * @param  len      length of the content
 * @return          status of the insert
 */
int Insert(fileID fid, void* contents, int len) {
    // TODO
    return 0;
}

/**
 * Retrieve a copy of a file
 * @param  fid      fileID
 * @param  contents buffer to hold the file content
 * @param  len      length of the buffer
 * @return          status of the lookup
 */
int Lookup(fileID fid, void* contents, int len) {
    // TODO
    return 0;
}

/**
 * Remove a file
 * @param  fid fileID
 * @return     status of reclaim
 */
int Reclaim(fileID fid) {
    // TODO
    return 0;
}
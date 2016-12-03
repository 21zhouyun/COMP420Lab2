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
    JoinMessage* message = new JoinMessage(id);
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
    TracePrintf(10, "Forward insert request\n");
    int status = 0;
    int src = 0;
    char* message = MakeDataMessage(fid, contents, len, INSERT);
    SendMessage(0, message, data_message_header_size + len * sizeof(char));
    delete[] message;

    if (ReceiveMessage(&src, &status, sizeof(int)) < 0) {
        std::cerr << "Fail to receive confirmation message for insert" << std::endl;
    }
    TracePrintf(10, "Done inserting file %hu\n", fid);
    return status;
}

/**
 * Retrieve a copy of a file
 * @param  fid      fileID
 * @param  contents buffer to hold the file content
 * @param  len      length of the buffer
 * @return          status of the lookup
 */
int Lookup(fileID fid, void* contents, int len) {
    TracePrintf(10, "Forward lookup request\n");
    int src = 0;
    int status = 0;

    if (len == 0) {
        return 0;
    }
    LookupMessage* message = new LookupMessage(fid, len);
    SendMessage(0, message, sizeof(LookupMessage));
    delete message;

    // receive status first
    if (ReceiveMessage(&src, &status, sizeof(int)) < 0) {
        std::cerr << "Fail to receive confirmation message for lookup" << std::endl;
    }

    if (status == 0) {
        return status;
    }

    if (ReceiveMessage(&src, contents, len) < 0) {
        std::cerr << "Fail to receive reply message for lookup" << std::endl;
    }
    return status;
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
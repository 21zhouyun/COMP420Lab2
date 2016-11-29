#include <rednet.h>
#include <unordered_map>
#include <iostream>

#include "message.h"

std::unordered_map<int, int> sequence_number_map;

void HandleMessage(int src, int dest, const void *msg, int len) {
	int pid = GetPid();
	if (src == pid && dest != 0) {
		// TODO: send message
		TracePrintf(10, "Send message from %d to %d\n", src, dest);
	} else if (src == dest || dest == 0) {
		// Received message
		Message* message = (Message*) msg;
		switch (message->type) {
		case JOIN: {
			TracePrintf(10, "Received join message from %d", src);
			// TODO
			break;
		}
		case FLOOD: {
			FloodMessage* fmessage = (FloodMessage*) msg;
			if (sequence_number_map.find(src) != sequence_number_map.end()
			        && fmessage->sequence_number <= sequence_number_map[src]) {
				return;
			}
			// TODO process the message
			fmessage->hop_count--;
			if (TransmitMessage(src, -1, fmessage, len) < 0) {
				std::cerr << "Failed to forward flood message from " << src << std::endl;
			}
			break;
		}
		default:
			std::cerr << "Unknown message type: " << message->type << std::endl;
			break;
		}
	}
}
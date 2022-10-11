//
//  message.h
//  siphon-demo
//
//  Created by Baochun Li on 2020-04-28.
//  Copyright © 2020 Baochun Li. All rights reserved.
//

#ifndef message_h
#define message_h
#include <stdbool.h>

typedef struct blockMessage {
    int seq_id;                   // sequence number
    int payload_size;             // size of the data payload, in bytes
    unsigned char * payload_ptr;  // pointer to the data payload
} BlockMessage;

void messageArrived(BlockMessage * block_message);
void decode(BlockMessage * block_message);
void sendMessage(BlockMessage * block_message);
void printMessageContent(BlockMessage * block_message);

#endif /* message_h */

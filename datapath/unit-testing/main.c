//
//  main.c
//  siphon-demo
//
//  Created by Baochun Li on 2020-04-28.
//  Copyright © 2020 Baochun Li. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "src/message.h"
#include "src/FIFOqueue/FIFO.h"
#include "src/main_Macro.h"


// This is a pointer for a shared buffer that behaves as a FIFO buffer used for simulating the sending and receiving of messages
FIFOPointer fifo_queue_pointer;


// Uses to print the message out in a formal way
void printMessageContent(BlockMessage * block_message){
    for(int i=0;i < block_message->payload_size; i++){
        // if the message is the normal character, print it directly,
        // otherwise, print the ASCII value
        int ascii_value =  (int)block_message->payload_ptr[i];
        if((32<=ascii_value) && (ascii_value<=127)) printf("%c", (int)block_message->payload_ptr[i]);
        else printf("%d", ascii_value);
    }
    printf(" | seq id: %d ", block_message->seq_id);
    printf("\n");
}

// Fake implementation only for testing Siphon's API
void receivedByDestination(struct blockMessage * block_message) {
    fifoGet(fifo_queue_pointer, block_message);
}

// Fake implementation only for testing Siphon's API
void sendMessage(BlockMessage * block_message) {
    // Adding the sequence identifier of the message
    block_message->seq_id++;
    fifoAdd(fifo_queue_pointer, block_message);

    // Printing the sending message on the screen
    printf("Sending message: ");
    printMessageContent(block_message);
}

int main(void) {

    // Creating a FIFO (queue) that can store length_of_simulation_buffer items
    fifo_queue_pointer = fifoCreate(length_of_FIFO_buffer);

    // Getting the full path of the data file
    char* original_data_file_path = malloc(sizeof(char) * SOURCE_FILE_LENGTH);
    getcwd(original_data_file_path, sizeof(char) * SOURCE_FILE_LENGTH);
    strcat(original_data_file_path, "/original-data.txt");

    // Opening the file containing the data to be set
    FILE *file = fopen(original_data_file_path, "rb");
    if (file == NULL) { // Judging whether the file is opened successfully.
        printf("Unable to open file: %s\n", "original-data.txt");
        return 1;
    }

    // Creating a new message for the sender side
    struct blockMessage * block_message = malloc(sizeof(BlockMessage));
    block_message->seq_id = 0;
    block_message->payload_ptr = malloc(sizeof(unsigned char) * maximum_message_size);

    // Creating a message for the receiver side
    struct blockMessage * received_block_message = malloc(sizeof(BlockMessage));
    received_block_message->payload_ptr = malloc(sizeof(unsigned char) * maximum_message_size);

    // Sending messages repeatedly
    for (int message_count = 0; message_count < number_of_messages; message_count++) {

        // Getting new data if there is no waiting to send messages in FIFO buffer
        if(isFIFOEmpty(fifo_queue_pointer)){
            // Reading message_size bytes from the file
            fread(block_message->payload_ptr, sizeof(unsigned char), message_size, file);
            block_message->payload_size = message_size;

            printf("Data to be encoded: %.*s\n", block_message->payload_size, block_message->payload_ptr);

            // Encoding the message now
            messageArrived(block_message);
        }

        // In the receiver side:

        // Receiving the message
        receivedByDestination(received_block_message);
        printf("Receiving message: ");
        printMessageContent(received_block_message);

        // Decoding the message
        decode(received_block_message);

        printf("\n");

    }

    // Releasing the allocated memory
    free(block_message->payload_ptr);
    free(block_message);
    free(received_block_message);
    free(received_block_message->payload_ptr);
    free(original_data_file_path);

    fifoRelease(fifo_queue_pointer);

    return 0;
}

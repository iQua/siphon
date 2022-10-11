#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../src/message.h"


// This is a demo of sending multiple messages and their parities.
// This is a very simple RS coding demo.
//
// The decoding schema for this demo is to compare the received parity message with the calculated parity
// on the receiver side. Then, the received message with error will be printed. It will print out 'No error'
// if no error occurs.
//
// Configurable variable:
//  - block_size: denotes the K in the (N, K).
//
// With the number of 'block_size' messages as input, this demo sends block_size+1 messages out where the added 1 is the
// parity message.
//
// For example:
//  For two messages "A good", and " person",
//  it can send "A good", " person", and the set of parity 01 for "A good" and  " person".
//  That is, we have two messages as input and send three messages out.




// Configurable variable
const int block_size = 3; // This is the K in RS - (N, K) code


int encoder_unsolved_messages_counter = block_size; // The number of messages that have not been calculated parity
int messages_counter_for_decoding = 0; // The number of received messages for each decoding process

// Pointers to the memories for parities sets
unsigned char* encoder_parity_set_ptr = NULL;
unsigned char* decoder_parity_set_ptr = NULL;



//
// Gets the parity bit for the char ch.
//
// Get the parity bit for the given input character.
//
// ch: The input single character that needed to be calculated
//
//   -It returns 1 if there is odd number of 1s
//   -It returns 0 if there is even number of 1s
//
//  Reference: https://www.geeksforgeeks.org/compute-parity-number-using-xor-table-look/
//
int getParity(unsigned char ch)
{
    int parity = 0;
    while (ch)
    {
        parity = !parity;
        ch      = ch & (ch - 1);
    }
    return parity;
}

//
// Gets the parity bit for a string.
//
//  Get the parity bit for the given first 'length' characters of the string.
//  It will output the parity of set bits after xor of these characters.
//
// chars: A pointer to a string.
// length: the first 'length' of the string will be calculated
//
//   -It returns 1 if there is odd number of 1s in the set bits after xor of the 'length' number of characters in the string.
//   -It returns 0 if there is even number of 1s in the set bits after xor of the 'length' number of characters in the string.
//
//  The principle is, for two numbers:
//   - If two numbers where both numbers have odd number of set bits in it then its XOR will have even number of set bits in it.
//   - If two numbers where both numbers have even number of set bits in it then its XOR will have even number of set bits in it.
//   - If two numbers where one number has even number of set bits and another has odd number of set bits then its XOR will have odd number of set bits in it.
//
//  Principle reference: https://stackoverflow.com/questions/50438913/parity-of-set-bits-after-xor-of-two-numbers
//
int getStringParity(unsigned char* chars, int length)
{
    int parity = 0;
    for(int i=0;i<length;i++){
        parity += getParity(chars[i]);
    }
    return (parity+1) % 2;
}

//
// Operates on the arrived message.
//
// Processing the arrived message. This function behaves as a encoding function.
//
// block_message: A pointer to a block message that contains sequence id, payload size and a pointer to the payload.
//                That is, it contains the message to be encoded.
//
//   -No return
//
//  Note: Siphon will call the 'messageArrived' function when a new message needs to be encoded
//        and sent by the source node.
//
void messageArrived(struct blockMessage * block_message) {
    // Obtaining the size, in number of bytes, of the payload (data portion) of the incoming message
    int payload_size = (int) block_message->payload_size;
    // Pointer to the actual payload in the message
    unsigned char * data = block_message->payload_ptr;

    // Allocating the memory for the encoder_parities_set
    if(encoder_parity_set_ptr == NULL) encoder_parity_set_ptr = malloc(block_size);

    // Toy example of encoding the data: converting the data to upper case
    // Note: Here, encoding occurs 'in place' directly within the message payload itself,
    // but it doesn't have to be.
    for (int i = 0; i < payload_size; i++) {
        data[i] = toupper(data[i]);
    }

    // If it is not the end of messaged required for calculating parities
    // - sending the current message directly
    // - concatenating parity for current message to the parity set
    if(encoder_unsolved_messages_counter>1){
        sendMessage(block_message); // send the current encoded data

        // Calculating parity for current message
        unsigned char obtained_parity = (unsigned char) getStringParity(block_message->payload_ptr, block_message->payload_size);
        // Getting the ending position of the payload in the parity set
        int ending_position = block_size-encoder_unsolved_messages_counter;
        // Concatenating the calculated parity to the end of the parity set
        memcpy(encoder_parity_set_ptr+ending_position, &obtained_parity, sizeof(unsigned char) * 1);

        encoder_unsolved_messages_counter--;

    }
    else{ //encoder_unsolved_messages_counter==1, at the end of required messages
        sendMessage(block_message); // send the current encoded data

        // Calculating the parity for current message
        unsigned char obtained_parity = (unsigned char) getStringParity(block_message->payload_ptr, block_message->payload_size);
        // Getting the ending position of the payload in the parity set
        int ending_position = block_size-encoder_unsolved_messages_counter;
        // Concatenating the calculated parity to the end of the parity set
        memcpy(encoder_parity_set_ptr+ending_position, &obtained_parity, 1);


        encoder_unsolved_messages_counter--;
        assert(encoder_unsolved_messages_counter==0); // Making sure that all required messages are processed

        // Setting the payload size to the size of the parity set
        block_message->payload_size = block_size;
        // Copying the parity set to the parity message to send out
        memcpy(block_message->payload_ptr, encoder_parity_set_ptr, block_message->payload_size);

        // Sending the parity message
        sendMessage(block_message);

        // Free the memory for the parity_set
        free(encoder_parity_set_ptr);
        encoder_parity_set_ptr = NULL;

        encoder_unsolved_messages_counter = block_size;

    }
}

//
// Decoding the message.
//
// This function is the decoding function provided by the decoder. It is used to decode the message.
//
// block_message: A pointer to a block message that contains sequence id, payload size and a pointer to the payload.
//                That is, it contains the message to be decoded.
//
//   -No return
//
//  Note: Siphon will call the 'decode()' function when a new message needs to be decoded.
//
void decode(struct blockMessage * block_message) {
    // Obtaining the size, in number of bytes, of the payload (data portion) of the incoming message
    int payload_size = block_message->payload_size;
    // Pointer to the actual payload in the message
    unsigned char * data = block_message->payload_ptr;

    // Allocating the memory for the decoder_parities_set
    if(decoder_parity_set_ptr == NULL) decoder_parity_set_ptr = malloc(block_size);

    // The message arrives
    if(messages_counter_for_decoding < block_size){

        // Calculating parity for current message
        unsigned char obtained_parity = (unsigned char) getStringParity(block_message->payload_ptr, block_message->payload_size);
        // Getting the ending position of the payload in the parity set
        int ending_position = sizeof(unsigned char) * (messages_counter_for_decoding);
        // Concatenating the calculated parity to the end of parities_set
        memcpy(decoder_parity_set_ptr+ending_position, &obtained_parity, sizeof(unsigned char) * 1);

        // Decoding the data. That is to convert the data to the lowercase.
        for (int i = 0; i < payload_size; i++) data[i] = tolower(data[i]);

        // Printing the decoded data out.
        printf("The Decoded received message: %.*s\n", block_message->payload_size, block_message->payload_ptr);

        messages_counter_for_decoding++;

    }
    else{ // The parity message arrives, then the decoding process is operated

        printf("With the calculated parities set: ");
        for(int i=0;i < messages_counter_for_decoding; i++) printf("%d", (int)decoder_parity_set_ptr[i]);

        // Detecting the error by using received parity and the calculated parity
        int detection_error_counter = 0;
        printf(", the decoding result is: ");
        for(int i=0; i< block_message->payload_size; i++){
            if(block_message->payload_ptr[i] != decoder_parity_set_ptr[i]){
                printf(" The error in %d-th received message; ", i+1);
                detection_error_counter++;
            }
        }
        if(detection_error_counter == 0) printf(" No error ");
        printf("\n");

        // Resetting the message counter for decoding
        messages_counter_for_decoding=0;

        // Free the memory for the parity_set
        free(decoder_parity_set_ptr);
        decoder_parity_set_ptr = NULL;
    }
}

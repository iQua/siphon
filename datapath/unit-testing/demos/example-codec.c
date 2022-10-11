#include <ctype.h>
#include <string.h>

#include "../src/message.h"
#include "../src/utils/Dictionary.h"


// This is the demo of sending the same message any number of times.
//  This demo can be regarded as the Repetition code.
//
// The decoding schema for this demo is the majority voting.
//
// Configurable variable:
//  - number_copies: The number of times to send the message repeatedly.
//
// For example:
//  For the message contains "A small dog",
//  it can repeatedly send "A small dog" several times defined by the variable 'number_copies';


const int number_copies = 3;

int start_sequence_id_tracker = 0; // A tracker is used to mark the starting sequence ID of a decoding process.
int expected_sequence_id_tracker = 0; // A tracker is used to mark the expected ending sequence ID of a decoding process.

// Defining the dictionary to achieve the majority voting
DictionaryRef       received_messages_map = NULL;
DictionaryCallbacks received_messages_map_callback;
int dictionary_length = number_copies;


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
    // Obtain the size, in number of bytes, of the payload (data portion) of the incoming message
    int payload_size = (int) block_message->payload_size;
    // Pointer to the actual payload in the message
    unsigned char * data = block_message->payload_ptr;

    // Toy example of encoding the data: converting the data to upper case
    // Note: Here, encoding occurs 'in place' directly within the message payload itself,
    // but it doesn't have to be.
    for (int i = 0; i < payload_size; i++) {
        data[i] = toupper(data[i]);
    }

    // Now send three copies of the message using repetition code, using the 
    // sendMessage() API provided by Siphon
    for (int copies = 0; copies < number_copies; copies++) {
      sendMessage(block_message);
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


    // Toy example of decoding the data: converting the data to lower case
    // Note: Here, encoding occurs 'in place' directly within the message payload itself,
    for (int i = 0; i < payload_size; i++) {
        data[i] = tolower(data[i]);
    }


    // Marking the starting sequence id and expected sequence id
    if(start_sequence_id_tracker==0){ // this is a new decoding process
        start_sequence_id_tracker = block_message->seq_id;
        expected_sequence_id_tracker = start_sequence_id_tracker+number_copies-1;
    }

    // if the sequence id of the arrived message is out of the sequence id bound for decoding
    //  - it means that some messages are missed for current decoding
    // the current decoding process will be dropped
    if(block_message->seq_id>expected_sequence_id_tracker){
        DictionaryClear(received_messages_map); // clear the dictionary for the new decoding process

        start_sequence_id_tracker = block_message->seq_id;
        expected_sequence_id_tracker = start_sequence_id_tracker+number_copies-1;
    }

    // Creating and Initializing the map that is used to count the received messages
    // key: the content of the message, value: the occurrence number
    if(received_messages_map==NULL){
        received_messages_map_callback = DictionaryStandardStringCallbacks();
        received_messages_map = DictionaryCreate( dictionary_length, &received_messages_map_callback );
    }

    // If the current received message has been in the map, increment count by 1
    // else, insert current received message to the map and set the value to "1"
    if(DictionaryKeyExists(received_messages_map, block_message->payload_ptr)){ // if there is a message in the map,
        int* same_messages_received_number = (int *) DictionaryGetValue(received_messages_map, block_message->payload_ptr);
        *same_messages_received_number = *same_messages_received_number+1; // Increasing the value
        DictionaryInsert( received_messages_map, block_message->payload_ptr, same_messages_received_number);
    }
    else{ // if the received message is a new one
        int initialized_value = 1;
        DictionaryInsert( received_messages_map, block_message->payload_ptr,  &initialized_value);
    }

    // Decoding the message with majority voting if (number_copies) messages are received.
    if(block_message->seq_id == expected_sequence_id_tracker){
        // Getting the message with maximum occurrence number from the map
        GetKeyWithMaximumValue(received_messages_map, block_message->payload_ptr, block_message->payload_size);

        // Obtaining the decoded message
        printf("Decoded message (the maximum number of occurrence): %.*s\n", block_message->payload_size, block_message->payload_ptr);

        // Clearing the dictionary for the next decoding process.
        DictionaryClear(received_messages_map);

        // Resetting the sequence ID tracker to 0
        start_sequence_id_tracker=0;
    }
}
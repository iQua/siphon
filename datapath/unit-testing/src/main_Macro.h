//
// Created by Sijia Chen on 2020/5/13.
//

// In this head file, you can set the configurable variables.
// Note, the configurable variables in this head file corresponds
//  to the variables in the configuration file of siphon-datapath.


#ifndef SIPHON_DEMO_MAIN_MACRO_H
#define SIPHON_DEMO_MAIN_MACRO_H

// Macro definition of file length.
#define SOURCE_FILE_LENGTH 1024


const int number_of_messages = 10; // Number of messages to send
const int length_of_FIFO_buffer = 100; // Number of messages that the FIFO buffer can hold
const int maximum_message_size = 100; // The maximum size of the message
const int message_size = 10; // The actual message size (I.e. the payload size in the message)



#endif //SIPHON_DEMO_MAIN_MACRO_H

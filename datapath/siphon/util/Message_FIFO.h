//
// Created by Sijia Chen on 2020/5/4.
//

#ifndef SIPHON_MESSAGE_FIFO_H
#define SIPHON_MESSAGE_FIFO_H

#include <cstddef>
#include <iostream>
#include <cstring>

#include "base/base_block_message.h"

namespace siphon{
    namespace util {

        // This structure holds the pointers to the messages that needed to be sent out
        struct FIFODescriptor {

            unsigned char **buffer_pool; // the starting address on the memory for pointers

            volatile uint16_t read_offset; // the starting point of memory offset for reading data
            volatile uint16_t write_offset; // the position of memory offset for writing data

            size_t capacity_of_items_number; // upper bound of the number of items in the buffer
            size_t stored_items_number; // the stored number of items in the buffer
        };


        // Definition for the fifo_ptr type, which is a pointer to a fifo_data struct
        typedef struct FIFODescriptor *FIFOPointer;


        // Creates a FIFO using dynamic memory
        //
        // This function is used to create a buffer, it allocates memory for a buffer that can hold
        // the requested number of items. The size is the number_of_items * sizeof(unsigned char*).
        //
        // number_of_items: The number of pointers to elements the buffer should be able to store,
        //
        // If a buffer is succesfully created, returns a pointer to the
        // structure that contains the buffer information (fifo_pointer). NULL is returned if
        // something fails.
        //
        FIFOPointer fifoCreate(uint16_t number_of_items);

        // Releases the memories occupied by the given FIFO
        //
        // This function is used to release any memory that is occupied by the given FIFO.
        //
        // fifo_pointer: The fifo to be released,
        //
        //  No return.
        //
        void fifoRelease(FIFOPointer fifo_pointer);


        // Adds one item to the FIFO buffer
        //
        // This function writes an item to the fifo buffer back. This function affects
        // the write pointer and the stored items counter.
        //
        // fifo_pointer: Pointer to a fifo_descriptor structure.
        // block_message: A pointer to BlockMessage structure.
        //
        // Returns true if there is space in the FIFO to add the item. If the
        // buffer is full and no data can be copied it returns false.
        //
        bool fifoAdd(FIFOPointer fifo_pointer, const block_seed::blockMessage *block_message);

        //
        // Obtains one item from the FIFO buffer.
        //
        // This function reads an item from the fifo buffer front. This function affects
        // the read pointer and the stored items counter.
        //
        // The number of bytes to be copied to the provided buffer was defined when the
        // fifo buffer was created with the function fifo_create() (size parameter).
        //
        // fifo Pointer: A pointer to a fifo_descriptor structure.
        // block_message: A pointer to BlockMessage structure.
        //
        // Returns true if there is data available on the fifo buffer to be
        // copied, if the buffer is empty and no data can be read this returns false.
        //
        bool fifoGet(FIFOPointer fifo_pointer, block_seed::blockMessage *block_message);

        //
        // Checks if the FIFO is full.
        //
        // Check if it can accept one item at least.
        //
        // fifo_pointer: Pointer to a fifo_descriptor structure.
        //
        // This function returns true if the buffer is full, false otherwise.
        //
        bool isFIFOFull(FIFOPointer fifo_pointer);

        //
        // Checks if the FIFO is empty.
        //
        // Check if the buffer has no data stored in it.
        //
        // fifo_pointer: Pointer to a fifo_descriptor structure.
        //
        // This function returns true if the buffer is empty, false otherwise.
        //
        bool isFIFOEmpty(FIFOPointer fifo_pointer);

        //
        // Copies data from the FIFO buffer.
        //
        // Copy the message from the FIFO to the given message.
        //
        // fifo_pointer: Pointer to a fifo_descriptor structure.
        // block_message: Pointer to the BlockMessage structure.
        //
        // No return for this function.
        //
        static void fifoCopyFrom(FIFOPointer fifo_pointer, block_seed::blockMessage *block_message);

        //
        // Copies data to the FIFO buffer.
        //
        // Copy the message from the given message to the FIFO .
        //
        // fifo_pointer: Pointer to a fifo_descriptor structure.
        // block_message: Pointer to the BlockMessage structure.
        //
        // No return for this function.
        //
        static void fifoCopyTo(FIFOPointer fifo_pointer, const block_seed::blockMessage *block_message);

    }
}

#endif //SIPHON_MESSAGE_FIFO_H

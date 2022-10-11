//
// Created by Sijia Chen on 2020/4/30.
//


#include "FIFO.h"


FIFOPointer fifoCreate(uint16_t number_of_items)
{
    FIFOPointer newfifo_pointer;
    if (number_of_items > 0) {
        newfifo_pointer = (struct FIFODescriptor *) malloc(sizeof(struct FIFODescriptor));
        if (newfifo_pointer != NULL) {
            // Trying to allocate space for the pointers
            newfifo_pointer->buffer_pool = (unsigned char **) malloc(number_of_items * sizeof(unsigned char *));
            if (newfifo_pointer->buffer_pool != NULL) {
                // Initializing structure members
                newfifo_pointer->read_offset = 0;
                newfifo_pointer->write_offset = 0;
                newfifo_pointer->capacity_of_items_number=number_of_items;
                newfifo_pointer->stored_items_number=0;
                // Returning the pointer to fifo_descriptor structure
                return newfifo_pointer;
            } else {
                // Cannot allocate space for items, free struct resources
                free(newfifo_pointer);
            }
        }
    }
    // Return NULL if something fails
    return NULL;
}

void fifoRelease( FIFOPointer fifo_pointer )
{
    if( fifo_pointer != NULL )
    {
        free(fifo_pointer->buffer_pool);
        free(fifo_pointer);
    }
}

bool fifoAdd(FIFOPointer fifo_pointer, const BlockMessage * block_message)
{
    if (!isFIFOFull(fifo_pointer)) {
        fifoCopyTo(fifo_pointer, block_message);
        fifo_pointer->stored_items_number += 1;
        return true;
    } else {
        return false;
    }
}

bool fifoGet(FIFOPointer fifo_pointer, BlockMessage * block_message)
{
    if (!isFIFOEmpty(fifo_pointer)) {
        fifoCopyFrom(fifo_pointer, block_message);
        fifo_pointer->stored_items_number -= 1;
        return true;
    } else {
        return false;
    }
}

bool isFIFOFull(FIFOPointer fifo_pointer)
{
    if (fifo_pointer->stored_items_number >= fifo_pointer->capacity_of_items_number)
        return true;
    else
        return false;
}

bool isFIFOEmpty(FIFOPointer fifo_pointer)
{
    if (fifo_pointer->stored_items_number == 0)
        return true;
    else
        return false;
}

static void fifoCopyFrom(FIFOPointer fifo_pointer, BlockMessage * block_message)
{
    memcpy(&block_message->seq_id, fifo_pointer->buffer_pool[fifo_pointer->read_offset], sizeof(int));
    memcpy(&block_message->payload_size, fifo_pointer->buffer_pool[fifo_pointer->read_offset]+sizeof(int), sizeof(int));
    memcpy(block_message->payload_ptr, fifo_pointer->buffer_pool[fifo_pointer->read_offset]+sizeof(int)+sizeof(int), block_message->payload_size);

    // Releasing the allocated buffer when the data in buffer pops out
    if(fifo_pointer->buffer_pool[fifo_pointer->read_offset]!=NULL)
        free(fifo_pointer->buffer_pool[fifo_pointer->read_offset]);

    fifo_pointer->read_offset += 1;
    if (fifo_pointer->read_offset >= fifo_pointer->capacity_of_items_number) {
        fifo_pointer->read_offset = 0;
    }
}

static void fifoCopyTo(FIFOPointer fifo_pointer, const BlockMessage * block_message)
{
    int seq_id = block_message->seq_id;
    int payload_size = block_message->payload_size;
    fifo_pointer->buffer_pool[fifo_pointer->write_offset] = malloc(sizeof(int) + sizeof(int) + payload_size);

    // Copying data in the given block_message to the buffer
    memcpy(fifo_pointer->buffer_pool[fifo_pointer->write_offset], &seq_id, sizeof(int));
    memcpy(fifo_pointer->buffer_pool[fifo_pointer->write_offset]+sizeof(int), &payload_size, sizeof(int));
    memcpy(fifo_pointer->buffer_pool[fifo_pointer->write_offset]+sizeof(int)+sizeof(int), block_message->payload_ptr, payload_size);

    fifo_pointer->write_offset += 1;
    if (fifo_pointer->write_offset >= fifo_pointer->capacity_of_items_number) {
        fifo_pointer->write_offset = 0;
    }
}

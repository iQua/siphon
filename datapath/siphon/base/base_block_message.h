//
// Created by Sijia Chen on 2020/3/28.
//

#ifndef SIPHON_BASE_BLOCK_MESSAGE_H
#define SIPHON_BASE_BLOCK_MESSAGE_H


namespace siphon
{
    namespace block_seed
    {
        // This is the struct for the block message.
        struct blockMessage
        {
        public:
            int seq_id; // the sequence ID
            int payload_size; // size of the payload in message
            unsigned char * udp_parameters; // pointer to the data for udp parameters
            unsigned char* payload_ptr; // pointer to the buffer of message

            blockMessage(): seq_id(0), payload_size (0), udp_parameters(nullptr), payload_ptr(nullptr){}

            void clear()
            {
                payload_size=0;
                seq_id=0;
                if(udp_parameters)
                    free(udp_parameters);
                if(payload_ptr)
                    free(payload_ptr);
                payload_ptr = nullptr;
                udp_parameters = nullptr;
            }
            void add_seq_id()
            {
                seq_id += 1;
            }
        } ;

    } // namespace block_seed

} // namepsace siphon



#endif //SIPHON_BASE_BLOCK_MESSAGE_H

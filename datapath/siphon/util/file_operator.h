//
// Created by Sijia Chen on 2020/3/26.
//

#ifndef SIPHON_FILE_OPERATOR_H
#define SIPHON_FILE_OPERATOR_H

#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <sstream> //std::stringstream

namespace siphon {
    namespace util {

        class FileOperator {
        public:
            FileOperator(std::string file_type, std::string op_type);

            void open_file(std::string file_path);

            const uint8_t ld_from_txt_file(unsigned char *loaded_data, std::string item_type);

            const uint8_t ld_line_from_txt_file(unsigned char *loaded_data, std::string item_type);

            const uint8_t ld_block_from_txt_file(unsigned char *loaded_data, std::string item_type, const uint8_t block_bytes);

            void wt_to_txt_file(unsigned char *data);

            void wt_line_to_txt_file(unsigned char *data);

            void wt_block_to_txt_file(unsigned char *data, const uint8_t block_bytes);

            virtual ~FileOperator() {};


        private:
            std::string operator_type; // define this is a read or write operator
            std::fstream file_operator; // A stream operator to process the file
            std::string original_data; // the original data
        };
    }
}




#endif //SIPHON_FILE_OPERATOR_H

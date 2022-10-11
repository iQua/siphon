//
// Created by Sijia Chen on 2020/3/26.
//

#include "file_operator.h"


namespace siphon {
    namespace util {
        FileOperator::FileOperator(std::string file_type, std::string op_type) {

            if (file_type == "txt") {
                original_data = "";
            }
            operator_type = op_type; // reader or writer
        }

        void FileOperator::open_file(std::string write_file_path) {

            std::ios_base::iostate exceptionMask = file_operator.exceptions() | std::ios::failbit;
            file_operator.exceptions(exceptionMask);
            try {
                if (operator_type == "reader") file_operator.open(write_file_path, std::fstream::in);
                if (operator_type == "writer") file_operator.open(write_file_path, std::fstream::app);
            }
            catch (std::ios_base::failure &e) {
                std::cout << "\n ** Failed for opening the original data file. ** \n" << std::endl;
                std::cerr << e.what() << '\n';
            }
        }

        const uint8_t FileOperator::ld_from_txt_file(unsigned char *loaded_data, std::string item_type) {
            if (original_data == "") {
                if (!file_operator.is_open()) {
                    std::cout << "\n ** Failed for opening the original data file. ** \n" << std::endl;
                    std::cout << "\n ** Please open the file first. ** \n" << std::endl;
                    return 0;
                }
                std::stringstream strStream;
                strStream << file_operator.rdbuf(); //read the file
                original_data = strStream.str(); //str holds the content of the file
            }

            int str_length = original_data.length();

            if (item_type == "str") {
                // declaring character array
                //unsigned char* ori_data_buffer = new unsigned char[str_length+1];
                // copying the contents of the
                // string to char array
                std::copy(original_data.begin(), original_data.end(), loaded_data);
                loaded_data[str_length] = '\0';
            }
            if (item_type == "digital") {
                std::vector<std::string> results;
                boost::algorithm::split(results, original_data, boost::is_any_of(","));

                for (int i = 0; i < results.size(); i++) {
                    loaded_data[i] = std::stoi(results[i]);
                }
            }
            return str_length;
        }

        const uint8_t FileOperator::ld_line_from_txt_file(unsigned char *loaded_data, std::string item_type) {
            if (!file_operator.is_open()) {
                std::cout << "\n ** Failed for opening the original data file. ** \n" << std::endl;
                std::cout << "\n ** Please open the file first. ** \n" << std::endl;
                return 0;
            }
            if (!file_operator.good()) { // go back to beginging

                file_operator.clear();
                file_operator.seekg(0, std::ios::beg);
            }
            // While it's not the end of file, grab data.
            std::getline(file_operator, original_data, '\t');


            int str_length = original_data.length();

            if (item_type == "str") {
                // declaring character array
                //unsigned char* ori_data_buffer = new unsigned char[str_length+1];
                // copying the contents of the
                // string to char array
                std::copy(original_data.begin(), original_data.end(), loaded_data);
                loaded_data[str_length] = '\0';
            }
            if (item_type == "digital") {
                std::vector<std::string> results;
                boost::algorithm::split(results, original_data, boost::is_any_of(","));

                for (int i = 0; i < results.size(); i++) {
                    loaded_data[i] = std::stoi(results[i]);
                }
            }

            return str_length;
        }

        const uint8_t FileOperator::ld_block_from_txt_file(unsigned char *loaded_data, std::string item_type,
                                                           const uint8_t block_bytes) {

            if (!file_operator.is_open()) {
                std::cout << "\n ** Failed for opening the original data file. ** \n" << std::endl;
                std::cout << "\n ** Please open the file first. ** \n" << std::endl;
                return 0;
            }
            if (!file_operator.good()) { // go back to beginging
                file_operator.clear();
                file_operator.seekg(0, std::ios::beg);
            }

            // While it's not the end of file, grab block of data.
            if (item_type == "str") {
                // declaring character array
                file_operator.read(reinterpret_cast<char *>(loaded_data), block_bytes);
                loaded_data[block_bytes] = '\0';
            }
            if (item_type == "digital") {
                std::vector<std::string> results;
                while (results.size() < block_bytes) {

                    //long pos = file_operator.tellg();
                    //file_operator.seekg(pos);
                    std::vector<char> digtial;
                    int i = 0;
                    while (true) {
                        char *oData_tmp = new char[1];
                        file_operator.read(oData_tmp, 1);
                        if (oData_tmp[0] == ',') break;
                        digtial[i] = oData_tmp[0]; // set '\0'
                        i++;
                    }
                    std::string dig(digtial.begin(), digtial.end());
                    results.push_back(dig);
                }

                for (int i = 0; i < results.size(); i++) {
                    loaded_data[i] = std::stoi(results[i]);
                }
            }
            return block_bytes;
        }

        void FileOperator::wt_to_txt_file(unsigned char *data) {
            original_data = reinterpret_cast<char *> (data);
            if (!file_operator.is_open()) {
                std::cout << "\n ** Failed for opening the original data file. ** \n" << std::endl;
                std::cout << "\n ** Please open the file first. ** \n" << std::endl;
                return;
            }
            file_operator << original_data;

        }

        void FileOperator::wt_block_to_txt_file(unsigned char *data, const uint8_t block_bytes) {
            original_data = reinterpret_cast<char *> (data);

            if (!file_operator.is_open()) {
                std::cout << "\n ** Failed for opening the original data file. ** \n" << std::endl;
                std::cout << "\n ** Please open the file first. ** \n" << std::endl;
                return;
            }
            file_operator.write(reinterpret_cast<const char *>(data), block_bytes);
        }
    }
}
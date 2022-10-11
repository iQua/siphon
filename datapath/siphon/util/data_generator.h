//
// Created by Sijia Chen on 2020/3/24.
//

#ifndef SIPHON_DATA_GENERATOR_H
#define SIPHON_DATA_GENERATOR_H


#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <map>
#include <cstdint>
#include <cstddef>
#include <list>
#include <memory>
#include <random>

namespace siphon {

    class DataGenerator {
    public:
        DataGenerator(std::size_t generate_data_size=0);

        void gen_normal_digitals(unsigned char* generated_data, std::size_t v_range[]);
        std::string generate_one_word(std::size_t wd_length);
        void gen_normal_string(unsigned char* generated_data);

        virtual ~DataGenerator() {};


    private:

        std::size_t gen_dt_bytes;
    };

}


#endif //SIPHON_DATA_GENERATOR_H

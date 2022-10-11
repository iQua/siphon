//
// Created by Sijia Chen on 2020/3/24.
//

#include "data_generator.h"


namespace siphon {

    DataGenerator::DataGenerator(std::size_t generate_data_bytes) {

        gen_dt_bytes = generate_data_bytes;
    }

    void DataGenerator::gen_normal_digitals(unsigned char* generated_data, std::size_t v_range[]){

        size_t lower_v = v_range[0];
        size_t upper_v= v_range[1];
        std::random_device rd; // obtain a random number from hardware
        std::mt19937 eng(rd()); // seed the generator
        std::uniform_int_distribution<> distr(lower_v, upper_v); // define the range

        for(int i=0; i<gen_dt_bytes; ++i)
            generated_data[i] = distr(eng);
    }

    std::string DataGenerator::generate_one_word(std::size_t wd_length){
        static const std::string alphabet = "abcdefghijklmnopqrstuvwxyz" ;
        static std::default_random_engine rng( std::time(nullptr) ) ;
        static std::uniform_int_distribution<std::size_t> distribution( 0, alphabet.size() - 1 ) ;
        std::string str ;
        while( str.size() < wd_length ) str += alphabet[ distribution(rng) ] ;
        return str;
    }

    void DataGenerator::gen_normal_string(unsigned char* generated_data){

        srand( (unsigned)time(NULL));
        std::string sentence = "";
        for(int i=0; i < (int) gen_dt_bytes/4; ++i){
            size_t wd_length = std::rand() % 10 + 4;
            std::string one_word = generate_one_word(wd_length);
            sentence = sentence + one_word;
            sentence += " ";
        }
        sentence.copy(reinterpret_cast<char *>(generated_data), gen_dt_bytes);
    }


}
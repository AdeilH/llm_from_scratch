//
// Created by Adeel on 6/17/26.
//


#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "nlohmann/json.hpp"
import gpt_bpe;
import std;
TEST_CASE("Bytes To Unicode Test") {
    CHECK_EQ(*BPEHelpers::bytes_to_unicode_gpt_2_impl()[105].c_str(), 'i');
    auto json_data = BPEHelpers::read_encoder_data("../encoder.json");
    auto bpe_data = BPEHelpers::vocab_bpe_merges("../vocab.bpe");

    for (auto it = json_data->begin(); it != json_data->end(); it++ ) {
        std::cout << "key: " << it.key() << "  value " << it.value() << std::endl;
    }

}

//
// Created by Adeel on 6/17/26.
//


#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "nlohmann/json.hpp"
import gpt_bpe;
import std;

TEST_CASE("Test Get Pairs") {
    auto output = BPEHelpers::get_pairs("ABCD");
    std::set<std::pair<char, char>> expected_output = {{'A', 'B'}, {'B', 'C'}, {'C', 'D'}};
    CHECK_EQ(output, expected_output);
}

TEST_CASE("Test Bytes To UNICODE") {
    auto b2u = BPEHelpers::bytes_to_unicode_gpt_2_impl();

    // for (auto const [x, y]: b2u) {
    //     std::print("{} {}\n", x, reinterpret_cast<const char *>(y.c_str()));
    // }

    CHECK_EQ(*reinterpret_cast<const char *>(b2u[65].c_str()), 'A');
}


TEST_CASE("Bytes To Unicode Test") {
    auto json_data = BPEHelpers::read_encoder_data("../encoder.json");
    auto bpe_data = BPEHelpers::vocab_bpe_merges("../vocab.bpe");

    // for (auto it = json_data->begin(); it != json_data->end(); it++ ) {
    //     std::cout << "key: " << it.key() << "  value " << it.value() << std::endl;
    // }

    for (auto const [x, y]: bpe_data | std::views::enumerate) {
        std::print("(\'{}\', \'{}\'),", reinterpret_cast<const char *>(y.first.c_str()), reinterpret_cast<const char *>(y.second.c_str()));
    }
}

TEST_CASE("Encoder Class:  ") {
    auto encoder = GPT2BPE::Encoder("../encoder.json", "../vocab.bpe");
    auto value = "Hello";

    auto encoded = encoder.encode(value);

    CHECK(encoded == std::vector{15496});

    auto decoded = encoder.decode(encoded);

    CHECK(decoded == value);
}

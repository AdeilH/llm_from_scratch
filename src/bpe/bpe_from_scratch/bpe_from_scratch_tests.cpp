//
// Created by Adeel on 6/24/26.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

import std;
import bpe_from_scratch;
TEST_CASE("Scratch Tokenizer") {
    std::vector<uint8_t> expectedOutput = {
        84, 104, 105, 115, 32, 105, 115, 32, 115, 111, 109, 101, 32, 116, 101, 120, 116
    };
    std::string input = "This is some text";
    auto output = BPEFromScratch::convert_to_byte_array(input);
    CHECK(expectedOutput == output);

    //print("Number of characters:", len(text))
    CHECK(17 == output.size());
    CHECK(17 == input.size());

    auto verdict_stream = std::ifstream("../the-verdict.txt", std::ios::binary);
    std::string text{};
    std::string line{};

    while (std::getline(verdict_stream, line)) {
        text += line;
        text += '\n';
    }


    auto Tokenizer = BPEFromScratch::BPEFromScratch{};
    Tokenizer.train(text, 1000);

    CHECK(Tokenizer.get_vocab_size() == 1000);
    CHECK(Tokenizer.get_bpe_merges_size() == 742);


    auto input_text = "Jack embraced beauty through art and life.<|endoftext|>";
    auto token_ids = Tokenizer.encode(input_text);

    CHECK(Tokenizer.decode(token_ids) == input_text);

    for (auto const &token_id: token_ids) {
        std::print("{} ... {}\n", token_id, Tokenizer.decode({token_id}));
    }

    Tokenizer.save_vocab_and_merges("vocab.json", "bpe_merges.txt");

    auto Tokenizer2 = BPEFromScratch::BPEFromScratch();
    Tokenizer2.load_vocab_and_merges("vocab.json", "bpe_merges.txt");

    CHECK(Tokenizer2.decode(token_ids) == input_text);

    auto TokenizerGPT = BPEFromScratch::BPEFromScratch();
    TokenizerGPT.load_vocab_and_merges_from_openai("encoder.json", "vocab.bpe");



    CHECK(TokenizerGPT.decode(TokenizerGPT.encode(input_text)) == input_text);



}

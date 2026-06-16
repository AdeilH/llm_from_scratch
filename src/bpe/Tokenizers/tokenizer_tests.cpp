//
// Created by Adeel on 6/16/26.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

import tokenizer;
import std;
import bi_directional_map;
TEST_CASE("testing the tokenization") {
    auto output = Tokenizer::generate_tokens_version_1("Hello, world. This, is a test.");
    CHECK(output.size() == 15);

    output = Tokenizer::generate_tokens_version_2("Hello, world. This, is a test.");
    CHECK(output.size() == 6);

    output = Tokenizer::generate_tokens_version_3("Hello, world. This, is a test.");
    CHECK(output.size() == 10);

    output = Tokenizer::generate_tokens_version_3("Hello, world-- This, is a test.");
    CHECK(output.size() == 10);

    output = Tokenizer::generate_tokens_version_4("Hello, world, This, is a test.");
    CHECK(output.size() == 6);
}

TEST_CASE("testing preprocessed tokens of the verdict") {
    auto file_content = Tokenizer::read_file("../the-verdict.txt");
    auto output = Tokenizer::generate_tokens_version_3(file_content);
    CHECK(output.size() == 4690);
}

TEST_CASE("number of unique tokens in the verdict") {
    auto file_content = Tokenizer::read_file("../the-verdict.txt");
    auto output = Tokenizer::generate_tokens_version_3(file_content);
    auto output_map = Tokenizer::de_duplicate_and_sort(output);
    CHECK(output_map.size() == 1130);
}

TEST_CASE("number of tokens after adding special tokens") {
    auto file_content = Tokenizer::read_file("../the-verdict.txt");
    auto output = Tokenizer::generate_tokens_version_3(file_content);
    auto output_map = Tokenizer::de_duplicate_and_sort(output);
    auto updated_map = Tokenizer::add_special_tokens({"<|end_of_text|>", "<|unk|>"}, output_map);
    CHECK(updated_map.size() == 1132);
}

TEST_CASE("Test SimpleTokenizerV1") {
    auto file_content = Tokenizer::read_file("../the-verdict.txt");
    auto output = Tokenizer::generate_tokens_version_3(file_content);
    auto output_map = Tokenizer::de_duplicate_and_sort(output);

    auto tokenizer = Tokenizer::SimpleTokenizerV1(output_map);

    auto text = "\"It's the last he painted, you know,\" Mrs. Gisburn said with pardonable pride.";
    auto encoded = tokenizer.encode(text);
    auto expected = std::vector<int>{
        1, 56, 2, 850, 988, 602, 533, 746, 5, 1126, 596, 5, 1, 67, 7, 38, 851, 1108, 754, 793, 7
    };
    CHECK(expected == encoded);

    // Decode

    auto decoded = tokenizer.decode(encoded);

    text = "Hello, do you like tea. Is this-- a test?";

    CHECK_THROWS_AS(tokenizer.encode(text), std::out_of_range);
}

TEST_CASE("Test SimpleTokenizerV2") {
    auto file_content = Tokenizer::read_file("../the-verdict.txt");
    auto output = Tokenizer::generate_tokens_version_3(file_content);
    auto output_map = Tokenizer::de_duplicate_and_sort(output);
    auto specialized_tokens_map = Tokenizer::add_special_tokens({ SpecialTokens::EndOfText, SpecialTokens::Unknown},
                                                                output_map);

    auto tokenizer = Tokenizer::SimpleTokenizerV2(output_map);

    auto text = "\"It's the last he painted, you know,\" Mrs. Gisburn said with pardonable pride.";
    auto encoded = tokenizer.encode(text);
    auto expected = std::vector<int>{
        1, 56, 2, 850, 988, 602, 533, 746, 5, 1126, 596, 5, 1, 67, 7, 38, 851, 1108, 754, 793, 7
    };
    CHECK(expected == encoded);

    // Decode

    auto decoded = tokenizer.decode(encoded);

    text = "Hello, do you like tea? <|endoftext|> In the sunlit terraces of the palace.";

    auto specialized_encoding = tokenizer.encode(text);

    auto expected_specialized = std::vector{
        1131, 5, 355, 1126, 628, 975, 10, 1130, 55, 988, 956, 984, 722, 988, 1131, 7
    };

    CHECK(specialized_encoding == expected_specialized);

    auto specialized_decoded_text = tokenizer.decode(specialized_encoding);

    CHECK(specialized_decoded_text == "<|unk|> , do you like tea ? <|endoftext|> In the sunlit terraces of the <|unk|> . ");
}

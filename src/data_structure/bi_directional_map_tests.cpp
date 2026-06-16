//
// Created by Adeel on 6/17/26.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

import bi_directional_map;
import std;
import tokenizer;

TEST_CASE("Insert into Map") {
    std::string token = "[UNK]";
    int id = -1;

    DataStructures::BiDirectionalMap bd_map{};
    bd_map.insert(id, token);

    CHECK(bd_map.fetch_by_value(token) == id);
    CHECK(bd_map.fetch_by_key(id) == token);
}

TEST_CASE("Insert The Verdict") {
    DataStructures::BiDirectionalMap bd_map{};
    auto file_content = Tokenizer::read_file("../the-verdict.txt");
    auto tokens = Tokenizer::generate_tokens_version_3(file_content);
    auto tokenized_ids = Tokenizer::de_duplicate_and_sort(tokens);

    CHECK(tokenized_ids.fetch_by_key(1129) == "yourself");
}
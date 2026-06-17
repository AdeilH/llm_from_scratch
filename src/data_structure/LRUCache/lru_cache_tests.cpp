//
// Created by Adeel on 6/17/26.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

import lru_cache;

TEST_CASE("Test to check if cache is inserting at the front") {
    DataStructures::LRUCache<int, 5> cache;
    cache.insert(5);
    cache.insert(4);
    cache.insert(3);
    cache.insert(2);
    cache.insert(1);


    std::print(std::cout, "{}", cache);
}
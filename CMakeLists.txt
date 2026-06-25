cmake_minimum_required(VERSION 4.3.2)
include(FetchContent)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

project(llm_From_scratch LANGUAGES CXX)
set(CMAKE_CXX_MODULE_STD ON)
SET(BOOST_UT_CXX_MODULES ON)


set(CMAKE_CXX_STANDARD 26)


add_executable(llm_from_scratch main.cpp src/start_up.cppm src/bpe/Tokenizers/tokenizer.cppm src/data_structure/BiDirectionalMap/bi_directional_map.cppm)
target_compile_features(llm_from_scratch PRIVATE cxx_std_26)

target_sources(llm_from_scratch PUBLIC FILE_SET all_my_modules TYPE CXX_MODULES FILES
        src/start_up.cppm
        src/bpe/Tokenizers/tokenizer.cppm
        src/data_structure/BiDirectionalMap/bi_directional_map.cppm)

target_compile_options(llm_from_scratch PRIVATE -Wno-reserved-module-identifier)

set(DOCTEST_VERSION v2.4.11)
FetchContent_Declare(doctest
        URL https://raw.githubusercontent.com/doctest/doctest/${DOCTEST_VERSION}/doctest/doctest.h
        DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(doctest)
set(NLOHMANN_JSON_BUILD_MODULES ON CACHE BOOL "NLohmmann/json module support" FORCE)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

set(DOCTEST_INCLUDE_DIR ${doctest_SOURCE_DIR})
include_directories(${DOCTEST_INCLUDE_DIR})

add_executable(tokenizer_tests src/bpe/Tokenizers/tokenizer_tests.cpp src/bpe/Tokenizers/tokenizer.cppm src/data_structure/BiDirectionalMap/bi_directional_map.cppm)
target_compile_features(tokenizer_tests PRIVATE cxx_std_26)
target_sources(tokenizer_tests PUBLIC FILE_SET all_my_modules TYPE CXX_MODULES FILES src/bpe/Tokenizers/tokenizer.cppm src/data_structure/BiDirectionalMap/bi_directional_map.cppm)
target_compile_options(tokenizer_tests PRIVATE -Wno-reserved-module-identifier)
target_include_directories(tokenizer_tests PUBLIC ${DOCTEST_INCLUDE_DIR})


add_executable(bi_directional_map_tests src/data_structure/BiDirectionalMap/bi_directional_map_tests.cpp src/data_structure/BiDirectionalMap/bi_directional_map.cppm src/bpe/Tokenizers/tokenizer.cppm)
target_compile_features(bi_directional_map_tests PRIVATE cxx_std_26)
target_sources(bi_directional_map_tests PUBLIC FILE_SET all_my_modules TYPE CXX_MODULES FILES src/data_structure/BiDirectionalMap/bi_directional_map.cppm src/bpe/Tokenizers/tokenizer.cppm)
target_compile_options(bi_directional_map_tests PRIVATE -Wno-reserved-module-identifier)
target_include_directories(bi_directional_map_tests PUBLIC ${DOCTEST_INCLUDE_DIR})

add_executable(lru_cache_tests src/data_structure/LRUCache/lru_cache_tests.cpp src/data_structure/LRUCache/lru_cache.cppm)
target_compile_features(lru_cache_tests PRIVATE cxx_std_26)
target_sources(lru_cache_tests PUBLIC FILE_SET all_my_modules TYPE CXX_MODULES FILES src/data_structure/LRUCache/lru_cache.cppm)
target_compile_options(lru_cache_tests PRIVATE -Wno-reserved-module-identifier)
target_include_directories(lru_cache_tests PUBLIC ${DOCTEST_INCLUDE_DIR})

add_executable(gpt_bpe_tests src/bpe/gpt_bpe/gpt_bpe_test.cpp src/bpe/gpt_bpe/gpt_bpe.cppm src/data_structure/LRUCache/lru_cache.cppm)
target_compile_features(gpt_bpe_tests PRIVATE cxx_std_26)
target_sources(gpt_bpe_tests PUBLIC FILE_SET all_my_modules TYPE CXX_MODULES FILES src/bpe/gpt_bpe/gpt_bpe.cppm src/data_structure/LRUCache/lru_cache.cppm)
target_compile_options(gpt_bpe_tests PRIVATE -Wno-reserved-module-identifier)
target_include_directories(gpt_bpe_tests PUBLIC ${DOCTEST_INCLUDE_DIR})
target_link_libraries(gpt_bpe_tests PRIVATE nlohmann_json::nlohmann_json)
#target_link_libraries(gpt_bpe_tests PRIVATE nlohmann_json_modules)

add_executable(bpe_from_scratch_tests src/bpe/bpe_from_scratch/bpe_from_scratch_tests.cpp src/bpe/bpe_from_scratch/bpe_from_scratch.cppm src/data_structure/BiDirectionalMap/bi_directional_map.cppm src/utils/bpe_helpers.cppm)
target_compile_features(bpe_from_scratch_tests PRIVATE cxx_std_26)
target_sources(bpe_from_scratch_tests PUBLIC FILE_SET all_my_modules TYPE CXX_MODULES FILES src/bpe/bpe_from_scratch/bpe_from_scratch.cppm src/data_structure/BiDirectionalMap/bi_directional_map.cppm src/utils/bpe_helpers.cppm)
target_compile_options(bpe_from_scratch_tests PRIVATE -Wno-reserved-module-identifier)
target_include_directories(bpe_from_scratch_tests PUBLIC ${DOCTEST_INCLUDE_DIR})
target_link_libraries(bpe_from_scratch_tests PRIVATE nlohmann_json::nlohmann_json)
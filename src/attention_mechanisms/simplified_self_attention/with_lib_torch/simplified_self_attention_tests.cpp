//
// Created by Adeel on 6/26/26.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <torch/torch.h>
import simplified_self_attention;
import std;

TEST_CASE("Basic Lib Torch Stuff") {
    auto inputs = torch::tensor({
        0.43, 0.15, 0.89,
        0.55, 0.87, 0.66,
        0.57, 0.85, 0.64,
        0.22, 0.58, 0.33,
        0.77, 0.25, 0.10,
        0.05, 0.80, 0.55
    }).view({6, 3});
    // calculates attention
    auto output = SimplifiedSelfAttention::basic_attention_score<6, 3>(inputs);

    // attention weights through division
    // attention score average
    auto normalized = SimplifiedSelfAttention::normalize(output);
    std::cout << normalized << std::endl;
    std::cout << "Normalized by average weight: " << normalized.sum() << std::endl;

    // softmax naive
    auto softmax_naive = SimplifiedSelfAttention::softmax_naive(output);
    std::cout << softmax_naive << std::endl;
    std::cout << "Soft Max Naive Weights: " << softmax_naive.sum() << std::endl;

    // softmax through torch
    auto attention_weights_softmax_torch = torch::softmax(output, 0);
    std::cout << attention_weights_softmax_torch << std::endl;
    std::cout << attention_weights_softmax_torch.sum() << std::endl;

    auto context_vec = SimplifiedSelfAttention::compute_context_vector<6, 3>(inputs);
    std::cout << "Context Vec \n" << context_vec << std::endl;
}

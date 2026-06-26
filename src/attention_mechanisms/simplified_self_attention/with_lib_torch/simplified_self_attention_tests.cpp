//
// Created by Adeel on 6/26/26.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <torch/torch.h>
import simplified_self_attention;
TEST_CASE("Basic Lib Torch Stuff") {
    auto inputs = torch::tensor({
        0.43, 0.15, 0.89,
        0.55, 0.87, 0.66,
        0.57, 0.85, 0.64,
        0.22, 0.58, 0.33,
        0.77, 0.25, 0.10,
        0.05, 0.80, 0.55
    }).view({6, 3});
    auto output = SimplifiedSelfAttention::basic_attention_score<6, 3>(inputs);

    std::cout << output << std::endl;

}

#include <torch/torch.h>

import std;
import tokenizer;
int main() {

    std::string filepath = "../the-verdict.txt";

    std::print("{}", Tokenizer::generate_tokens_version_1(Tokenizer::read_file(filepath)));

    const torch::Tensor tensor = torch::rand({2, 3});
    std::cout << tensor << std::endl;
    const auto input = torch::Tensor{};
    std::cout << input << std::endl;

    return 0;
}

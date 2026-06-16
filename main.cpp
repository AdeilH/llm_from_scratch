import std;
import start_up;
import tokenizer;
int main() {

    std::string filepath = "../the-verdict.txt";

    std::print("{}", Tokenizer::generate_tokens_version_1(Tokenizer::read_file(filepath)));

    std::print("{}", add(3, 4));
    return 0;
}

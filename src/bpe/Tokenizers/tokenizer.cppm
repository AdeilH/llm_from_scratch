//
// Created by Adeel on 6/16/26.
//

export module tokenizer;


import std;
import bi_directional_map;
export namespace SpecialTokens {
    std::string Unknown = "<|unk|>";
    std::string BeginningOfSequence = "[BOS]";
    std::string EndOfSequence = "[EOS]";
    std::string Padding = "[PAD]";
    std::string EndOfText = "<|endoftext|>";
}

namespace Tokenizer {
    export auto read_file(const std::string &filepath) {
        if (!std::filesystem::exists(filepath)) {
            throw std::runtime_error("File Dows Not Exist");
        }
        auto file_size = std::filesystem::file_size(filepath);
        std::ifstream file_stream(filepath, std::ios::binary);
        if (!file_stream) {
            throw std::runtime_error("Could not open file");
        }
        std::string content(file_size, '\0');
        file_stream.read(content.data(), file_size);
        return content;
    }

    // with space as tokens

    export std::vector<std::string> generate_tokens_version_1(const std::string &text) {
        std::vector<std::string> tokens{};
        std::regex word_regex("(\\s+|[A-Za-z0-9_]+(?:'[A-Za-z0-9_]+)?|[^\\sA-Za-z0-9_]+)");
        auto words_begin =
                std::sregex_iterator(text.begin(), text.end(), word_regex);
        auto words_end = std::sregex_iterator();

        while (words_begin != words_end) {
            tokens.emplace_back(words_begin->str());
            ++words_begin;
        }
        return tokens;
    }

    // with no spaces and no punctuation as tokens
    export std::vector<std::string> generate_tokens_version_2(std::string_view text) {
        std::vector<std::string> tokens{};

        for (const auto &word_view: text | std::views::split(' ')) {
            tokens.emplace_back(std::string_view(word_view));
        }

        return tokens;
    }

    // with punctuation as tokens
    export std::vector<std::string> generate_tokens_version_3(std::string_view text) {
        std::vector<std::string> tokens{};

        // std::regex token_regex(
        //     R"TOKEN([A-Za-z0-9]+(?:['’-][A-Za-z0-9]+)*|--|[,.:;?_!"()'’])TOKEN"
        // );

        std::regex token_regex(
            R"TOKEN([A-Za-z0-9]+(?:-[A-Za-z0-9]+)*|--|[,.:;?_!"()'’])TOKEN"
        );
        for (const auto &word_view: text | std::views::split(' ')) {
            std::string word(word_view.begin(), word_view.end());

            auto begin = std::sregex_iterator(word.begin(), word.end(), token_regex);
            auto end = std::sregex_iterator();

            for (auto it = begin; it != end; ++it) {
                tokens.push_back(it->str());
            }
        }

        return tokens;
    }

    // generate token without space and punctuation
    export std::vector<std::string> generate_tokens_version_4(std::string_view text) {
        std::vector<std::string> tokens{};

        for (auto word_view: text | std::views::split(' ')) {
            auto word = std::string(word_view.begin(), word_view.end());
            std::erase_if(word, [](const char &c) {
                return !std::isalnum(c);
            });
            tokens.emplace_back(word);
        }
        return tokens;
    }

    export DataStructures::BiDirectionalMap de_duplicate_and_sort(std::vector<std::string> input) {
        std::ranges::sort(input);
        auto result = std::ranges::unique(input);
        input.erase(result.begin(), result.end());
        DataStructures::BiDirectionalMap output_map{};
        for (auto [i, v]: input | std::views::enumerate) {
            output_map.insert(i, v);
        }
        return output_map;
    }

    // with punctuation as tokens for specialized tokens
    export std::vector<std::string> generate_tokens_version_5(std::string_view text) {
        std::vector<std::string> tokens{};

        // std::regex token_regex(
        //     R"TOKEN([A-Za-z0-9]+(?:['’-][A-Za-z0-9]+)*|--|[,.:;?_!"()'’])TOKEN"
        // );

        std::regex token_regex(
            R"TOKEN(<\|[^|]+\|>|[A-Za-z0-9]+(?:-[A-Za-z0-9]+)*|--|[,.:;?_!"()'’])TOKEN"
        );
        for (const auto &word_view: text | std::views::split(' ')) {
            std::string word(word_view.begin(), word_view.end());

            auto begin = std::sregex_iterator(word.begin(), word.end(), token_regex);
            auto end = std::sregex_iterator();

            for (auto it = begin; it != end; ++it) {
                tokens.push_back(it->str());
            }
        }

        return tokens;
    }

    export DataStructures::BiDirectionalMap add_special_tokens(std::vector<std::string> special_tokens,
                                                               DataStructures::BiDirectionalMap &bd_map) {
        auto index = bd_map.size();
        for (auto const &special_token: special_tokens) {
            bd_map.insert(index, special_token);
            index++;
        }
        std::print("{}", bd_map);
        return bd_map;
    }

    export class SimpleTokenizerV1 {
        DataStructures::BiDirectionalMap m_vocab_ids{};

    public:
        explicit SimpleTokenizerV1(const DataStructures::BiDirectionalMap &vocab) : m_vocab_ids(vocab) {
        }

        [[nodiscard]] auto encode(std::string_view text) -> std::vector<int> {
            auto ids = std::vector<int>{};
            auto tokens = generate_tokens_version_3(text);

            for (auto const tok: tokens) {
                ids.emplace_back(m_vocab_ids.fetch_by_value(tok));
            }
            return ids;
        }

        [[nodiscard]] auto decode(std::vector<int> ids) -> std::string {
            std::string output{};
            for (auto const &id: ids) {
                output.append(m_vocab_ids.fetch_by_key(id) + " ");
            }
            return output;
        }
    };

    export class SimpleTokenizerV2 {
        DataStructures::BiDirectionalMap m_vocab_ids{};

    public:
        explicit SimpleTokenizerV2(const DataStructures::BiDirectionalMap &vocab) : m_vocab_ids(vocab) {
        }

        [[nodiscard]] auto encode(std::string_view text) -> std::vector<int> {
            auto ids = std::vector<int>{};
            auto tokens = generate_tokens_version_5(text);

            for (auto const tok: tokens) {
                if (m_vocab_ids.contains(tok)) {
                    ids.emplace_back(m_vocab_ids.fetch_by_value(tok));
                } else {
                    ids.emplace_back(m_vocab_ids.fetch_by_value(SpecialTokens::Unknown));
                }
            }
            return ids;
        }

        [[nodiscard]] auto decode(std::vector<int> ids) -> std::string {
            std::string output{};
            for (auto const &id: ids) {
                output.append(m_vocab_ids.fetch_by_key(id) + " ");
            }
            return output;
        }
    };
}

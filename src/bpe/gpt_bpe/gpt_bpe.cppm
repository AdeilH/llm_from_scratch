//
// Created by Adeel on 6/17/26.
//

module;

// replace with module in future
#include <nlohmann/json.hpp>
#include <utility>

export module gpt_bpe;

import std;
import lru_cache;
using json = nlohmann::json;

namespace BPEHelpers {
    export std::unordered_map<int, std::u8string> bytes_to_unicode_gpt_2_impl() {
        std::vector<int> bs;

        // 1. Replicate Python ranges: bs = list(range(ord("!"), ord("~")+1)) ...
        // ord('!') is 33, ord('~') is 126
        for (int i = 33; i <= 126; ++i) bs.push_back(i);
        // ord('¡') is 161, ord('¬') is 172ocket
        for (int i = 161; i <= 172; ++i) bs.push_back(i);
        // ord('®') is 174, ord('ÿ') is 255
        for (int i = 174; i <= 255; ++i) bs.push_back(i);

        // Clone bs into cs
        std::vector<int> cs = bs;

        int n = 0;
        for (int b = 0; b < 256; ++b) {
            if (std::ranges::find(bs, b) == bs.end()) {
                bs.push_back(b);
                cs.push_back(256 + n); // Map to shifted upper space
                n++;
            }
        }

        std::unordered_map<int, std::u8string> output;
        for (std::size_t i = 0; i < bs.size(); ++i) {
            const int code_point = cs[i];
            std::u8string utf8_char;

            if (code_point <= 127) {
                utf8_char.push_back(static_cast<char8_t>(code_point));
            } else {
                utf8_char.push_back(static_cast<char8_t>((code_point / 64) + 192));
                utf8_char.push_back(static_cast<char8_t>((code_point % 64) + 128));
            }
            output[bs[i]] = utf8_char;
        }
        return output;
    }

    export auto get_pairs(const std::string &word) {
        auto output = std::set<std::pair<char, char> >{};
        auto prev_char = word[0];
        for (auto const ch: word.substr(1)) {
            output.insert({prev_char, ch});
            prev_char = ch;
        }
        return output;
    }

    export std::unordered_map<int, std::u8string> bytes_to_unicode() {
        std::unordered_map<int, std::u8string> output{};

        for (int i = 0; i < 1114112; ++i) {
            // 1-Byte UTF-8 Range: 0 to 127
            if (i >= 0 && i <= 127) {
                std::u8string utf8_str;
                utf8_str.push_back(static_cast<char8_t>(i));
                output[i] = utf8_str;
            } else if (i >= 128 && i <= 2047) {
                std::u8string utf8_str;
                utf8_str.push_back(static_cast<char8_t>((i / 64) + 192));
                utf8_str.push_back(static_cast<char8_t>((i % 64) + 128));
                output[i] = utf8_str;
            } else if (i >= 2048 && i <= 65535) {
                std::u8string utf8_str;
                utf8_str.push_back(static_cast<char8_t>((i / 4096) + 224));
                utf8_str.push_back(static_cast<char8_t>(((i / 64) % 64) + 128));
                utf8_str.push_back(static_cast<char8_t>((i % 64) + 128));
                output[i] = utf8_str;
            } else if (i >= 65536 && i <= 1114111) {
                std::u8string utf8_str;
                utf8_str.push_back(static_cast<char8_t>((i / 262144) + 240));
                utf8_str.push_back(static_cast<char8_t>(((i / 4096) % 64) + 128));
                utf8_str.push_back(static_cast<char8_t>(((i / 64) % 64) + 128));
                utf8_str.push_back(static_cast<char8_t>((i % 64) + 128));
                output[i] = utf8_str;
            }

            // Out of official Unicode boundaries
            else {
                throw std::runtime_error("Something's fishy: code point exceeds Unicode limits");
            }
        }

        return output;
    }

    export std::unique_ptr<json> read_encoder_data(const std::filesystem::path &filepath) {
        if (!std::filesystem::exists(filepath)) {
            throw std::runtime_error("Encoder File Dos Not Exist");
        }
        std::ifstream file_stream(filepath, std::ios::binary);
        if (!file_stream) {
            throw std::runtime_error("Could not open Encoder file");
        }
        json data = json::parse(file_stream);
        file_stream.close();
        return std::make_unique<json>(data);
    }

    // with no spaces and no punctuation as tokens
    // Returns a vector of tuples containing the paired u8string merges
    export std::vector<std::pair<std::u8string, std::u8string> > vocab_bpe_merges(
        const std::filesystem::path &filepath) {
        if (!std::filesystem::exists(filepath)) {
            throw std::runtime_error("Vocab Bpe File Does Not Exist");
        }

        std::ifstream file_stream(filepath, std::ios::binary);
        if (!file_stream) {
            throw std::runtime_error("Could not open Vocab BPE file");
        }

        // 1. Read the entire file into a raw std::string buffer
        std::string text((std::istreambuf_iterator<char>(file_stream)),
                         std::istreambuf_iterator<char>());
        file_stream.close();

        std::vector<std::pair<std::u8string, std::u8string> > bpe_merges;

        // 2. Split into lines using C++20 ranges
        auto lines_view = text | std::views::split('\n');

        // We convert the view to a vector to easily drop the first and last lines like Python's [1:-1]
        std::vector<std::string_view> lines;
        for (auto &&line_range: lines_view) {
            lines.emplace_back(line_range.data(), line_range.size());
        }

        // Ensure we have lines to skip (header and trailing empty line)
        if (lines.size() <= 2) {
            return bpe_merges;
        }

        // 3. Process from index 1 to size() - 1
        for (size_t i = 1; i < lines.size() - 1; ++i) {
            std::string_view current_line = lines[i];
            if (current_line.empty()) continue;

            // Split the line by spaces to find the pair strings
            auto words_view = current_line | std::views::split(' ');
            auto it = words_view.begin();

            if (it != words_view.end()) {
                auto first_word_range = *it;
                ++it;
                if (it != words_view.end()) {
                    auto second_word_range = *it;

                    // Cast the string views directly to char8_t for std::u8string
                    std::u8string left(reinterpret_cast<const char8_t *>(first_word_range.data()),
                                       first_word_range.size());
                    std::u8string right(reinterpret_cast<const char8_t *>(second_word_range.data()),
                                        second_word_range.size());

                    bpe_merges.emplace_back(left, right);
                }
            }
        }

        return bpe_merges;
    }

    export auto get_pairs(const std::vector<std::u8string> &word) {
        std::set<std::pair<std::u8string, std::u8string> > output{};

        if (word.size() < 2) {
            return output;
        }

        for (std::size_t i = 0; i + 1 < word.size(); ++i) {
            output.insert({word[i], word[i + 1]});
        }

        return output;
    }
}

namespace GPT2BPE {
    // Custom hash function for std::pair
    struct pair_hash {
        template<class T1, class T2>
        std::size_t operator ()(const std::pair<T1, T2> &p) const {
            // Hash individual elements
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);

            // Combine hashes (Using a bitwise XOR and bit shift to reduce collisions)
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    export class Encoder {
        std::filesystem::path m_encoder_json_path{};
        std::filesystem::path m_vocab_bpe_path{};
        std::unordered_map<std::string, int> m_encoder{};
        std::unordered_map<int, std::string> m_decoder{};
        std::unordered_map<int, std::u8string> m_bytes_encoder{};
        std::unordered_map<std::u8string, int> m_bytes_decoder{};
        std::unordered_map<std::pair<std::u8string, std::u8string>, int, pair_hash> m_bpe_ranks{};
        std::unordered_map<std::string, std::string> m_cache{}; // lru is complicating so avoiding it for now

        auto bpe(const std::string &token) {
            if (m_cache.contains(token)) {
                return m_cache[token];
            }

            std::vector<std::u8string> word{};
            word.reserve(token.size());

            for (unsigned char ch: token) {
                word.emplace_back(1, static_cast<char8_t>(ch));
            }

            auto pairs = BPEHelpers::get_pairs(word);

            if (pairs.empty()) {
                m_cache[token] = token;
                return token;
            }

            while (true) {
                auto bigram_it = std::ranges::min_element(pairs, [this](const auto &lhs, const auto &rhs) {
                    constexpr int infinity = std::numeric_limits<int>::max();

                    auto lhs_it = m_bpe_ranks.find(lhs);
                    auto rhs_it = m_bpe_ranks.find(rhs);

                    int lhs_rank = lhs_it == m_bpe_ranks.end() ? infinity : lhs_it->second;
                    int rhs_rank = rhs_it == m_bpe_ranks.end() ? infinity : rhs_it->second;

                    return lhs_rank < rhs_rank;
                });

                if (!m_bpe_ranks.contains(*bigram_it)) {
                    break;
                }

                const auto &first = bigram_it->first;
                const auto &second = bigram_it->second;

                std::vector<std::u8string> new_word;
                std::size_t i = 0;

                while (i < word.size()) {
                    auto j_it = std::find(word.begin() + static_cast<std::ptrdiff_t>(i), word.end(), first);

                    if (j_it == word.end()) {
                        new_word.insert(new_word.end(), word.begin() + static_cast<std::ptrdiff_t>(i), word.end());
                        break;
                    }

                    auto j = static_cast<std::size_t>(std::distance(word.begin(), j_it));

                    new_word.insert(
                        new_word.end(),
                        word.begin() + static_cast<std::ptrdiff_t>(i),
                        word.begin() + static_cast<std::ptrdiff_t>(j)
                    );

                    i = j;
                    if (word[i] == first && i + 1 < word.size() && word[i + 1] == second) {
                        std::u8string merged = first;
                        merged += second;
                        new_word.push_back(std::move(merged));
                        i += 2;
                    } else {
                        new_word.push_back(word[i]);
                        i += 1;
                    }
                }

                word = std::move(new_word);

                if (word.size() == 1) {
                    break;
                }

                pairs = BPEHelpers::get_pairs(word);
            }

            std::string result{};

            for (std::size_t i = 0; i < word.size(); ++i) {
                if (i != 0) {
                    result += ' ';
                }

                result += std::string(
                    reinterpret_cast<const char *>(word[i].data()),
                    word[i].size()
                );
            }

            m_cache[token] = result;
            return result;
        }

    public:
        explicit Encoder(const std::filesystem::path &encoder_path,
                         const std::filesystem::path &vocab_path) : m_encoder_json_path(encoder_path),
                                                                    m_vocab_bpe_path(vocab_path) {
            auto encoder_json_data = BPEHelpers::read_encoder_data(encoder_path);
            for (auto it = encoder_json_data->begin(); it != encoder_json_data->end(); ++it) {
                m_encoder.insert({it.key(), it.value()});
                m_decoder.insert({it.value(), it.key()});
            }

            m_bytes_encoder = BPEHelpers::bytes_to_unicode_gpt_2_impl();
            for (const auto &[key, value]: m_bytes_encoder) {
                m_bytes_decoder.insert({value, key});
            }
            auto const bpe_data = BPEHelpers::vocab_bpe_merges(m_vocab_bpe_path);

            for (auto const [x, y]: bpe_data | std::views::enumerate) {
                m_bpe_ranks.insert({
                    y, x
                });
            }
            // for (auto const& [x, y]: m_bpe_ranks) {
            //     std::print(R"(('{}', '{}'): {},)", reinterpret_cast<const char *>(x.first.c_str()), reinterpret_cast<const char *>(x.second.c_str()), y);
            // }
        }

        auto encode(const std::string &text) -> std::vector<int> {
            std::vector<int> bpe_tokens{};

            std::regex pat(
                R"('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^[:space:][:alpha:][:digit:]]+|[[:space:]]+(?!\S)|[[:space:]]+)"
            );

            auto begin = std::sregex_iterator(text.begin(), text.end(), pat);
            auto end = std::sregex_iterator{};

            for (auto it = begin; it != end; ++it) {
                std::string token = it->str();

                std::string encoded_token{};

                for (unsigned char byte: token) {
                    const auto &mapped = m_bytes_encoder.at(static_cast<int>(byte));
                    encoded_token += std::string(
                        reinterpret_cast<const char *>(mapped.data()),
                        mapped.size()
                    );
                }

                std::string bpe_result = bpe(encoded_token);

                auto parts = bpe_result | std::views::split(' ');

                for (auto part: parts) {
                    std::string bpe_token{part.begin(), part.end()};
                    bpe_tokens.push_back(m_encoder.at(bpe_token));
                }
            }

            return bpe_tokens;
        }

        auto decode(const std::vector<int>& tokens) -> std::string {
            std::string text{};

            for (int token : tokens) {
                text += m_decoder.at(token);
            }

            std::string decoded{};

            for (std::size_t i = 0; i < text.size();) {
                std::u8string matched{};
                int matched_byte = -1;
                std::size_t matched_size = 0;

                for (const auto& [unicode_string, byte_value] : m_bytes_decoder) {
                    std::string candidate{
                        reinterpret_cast<const char*>(unicode_string.data()),
                        unicode_string.size()
                    };

                    if (
                        text.size() - i >= candidate.size() &&
                        text.compare(i, candidate.size(), candidate) == 0
                    ) {
                        if (candidate.size() > matched_size) {
                            matched_byte = byte_value;
                            matched_size = candidate.size();
                        }
                    }
                }

                if (matched_byte == -1) {
                    throw std::runtime_error("Could not decode byte-unicode token");
                }

                decoded.push_back(static_cast<char>(matched_byte));
                i += matched_size;
            }

            return decoded;
        }
    };
}

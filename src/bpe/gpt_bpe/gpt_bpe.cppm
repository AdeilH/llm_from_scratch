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

    export class Encoder {
    };

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
    export std::vector<std::tuple<std::u8string, std::u8string> > vocab_bpe_merges(
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

        std::vector<std::tuple<std::u8string, std::u8string> > bpe_merges;

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
}

namespace GPT2BPE {
    export class Encoder {
        std::filesystem::path m_encoder_json_path{};
        std::filesystem::path m_vocab_bpe_path{};
        std::unordered_map<std::string, int> m_encoder{};
        std::unordered_map<int, std::string> m_decoder{};
        std::unordered_map<int, std::u8string> m_bytes_encoder{};
        std::unordered_map<std::u8string, int> m_bytes_decoder{};
        std::unordered_map<std::string, std::vector<std::string>> m_cache; // lru is complicating so avoiding it for now

    public:
        explicit Encoder(std::filesystem::path encoder_path,
                         std::filesystem::path vocab_path) : m_encoder_json_path(std::move(encoder_path)),
                                                             m_vocab_bpe_path(std::move(vocab_path)) {
            auto encoder_json_data = BPEHelpers::read_encoder_data(encoder_path);
            for (auto it = encoder_json_data->begin(); it < encoder_json_data->end(); it++) {
                m_encoder.insert({it.key(), it.value()});
                m_decoder.insert({it.value(), it.key()});
            }

            m_bytes_encoder = BPEHelpers::bytes_to_unicode_gpt_2_impl();
            for (const auto &[key, value]: m_bytes_encoder) {
                m_bytes_decoder.insert({value, key});
            }
        }
        std::vector<std::string> bpe(const std::string& token) {
            if (m_cache.find(token) != m_cache.end()) {
                return m_cache[token];
            }

            // Initialize word as an array of individual symbols
            std::vector<std::string> word;
            std::string current = "";
            for (size_t i = 0; i < token.length();) {
                unsigned char c = token[i];
                if ((c & 0x80) == 0) { current = token.substr(i, 1); i += 1; }
                else if ((c & 0xE0) == 0xC0) { current = token.substr(i, 2); i += 2; }
                else if ((c & 0xF0) == 0xE0) { current = token.substr(i, 3); i += 3; }
                else { current = token.substr(i, 4); i += 4; }
                word.push_back(current);
            }

            std::set<std::pair<std::string, std::string>> pairs = BPEHelpers::get_pairs(word);
            if (pairs.empty()) return {token};
            

            while (true) {
                // Find the bigram with the minimum rank (highest priority)
                std::pair<std::string, std::string> bigram = *pairs.begin();
                int min_rank = 1e9;
                bool found = false;

                for (const auto& pair : pairs) {
                    if (bpe_ranks.find(pair) != bpe_ranks.end()) {
                        if (bpe_ranks[pair] < min_rank) {
                            min_rank = bpe_ranks[pair];
                            bigram = pair;
                            found = true;
                        }
                    }
                }

                if (!found) break; // No more rankable merges available

                std::string first = bigram.first;
                std::string second = bigram.second;
                std::vector<std::string> new_word;

                for (size_t i = 0; i < word.size(); ++i) {
                    if (word[i] == first && i < word.size() - 1 && word[i + 1] == second) {
                        new_word.push_back(first + second);
                        i++; // Skip the second token of the bigram
                    } else {
                        new_word.push_back(word[i]);
                    }
                }

                word = new_word;
                if (word.size() == 1) break;
                pairs = get_pairs(word);
            }

            cache[token] = word;
            return word;
        }

    };
}

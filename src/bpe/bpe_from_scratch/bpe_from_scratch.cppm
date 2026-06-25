//
// Created by Adeel on 6/24/26.
//

module;
#include <nlohmann/json.hpp>

export module bpe_from_scratch;

import std;
import bi_directional_map;
import bpe_helpers;
using json = nlohmann::json;

namespace BPEFromScratch {
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

    export std::vector<std::uint8_t> convert_to_byte_array(const std::string &text) {
        std::vector<std::uint8_t> output{};
        for (auto const &ch: text) {
            output.push_back(static_cast<std::uint8_t>(ch));
        }
        return output;
    }

    export std::vector<std::u8string> pre_tokenize_text(const std::string &text) {
        std::vector<std::u8string> output = {};
        auto push_ascii = [&output](const std::string &value) {
            output.emplace_back(
                reinterpret_cast<const char8_t *>(value.data()),
                value.size()
            );
        };
        auto push_space_marker = [&output]() {
            output.push_back(u8"Ġ");
        };

        auto push_space_prefixed_word = [&output](const std::string &word) {
            std::u8string token = u8"Ġ";
            token.append(
                reinterpret_cast<const char8_t *>(word.data()),
                word.size()
            );
            output.push_back(std::move(token));
        };

        std::size_t i = 0;

        while (i < text.size()) {
            if (text[i] == '\r') {
                if (i + 1 < text.size() && text[i + 1] == '\n') {
                    push_ascii("\r");
                    push_ascii("\n");
                    i += 2;
                } else {
                    push_ascii("\r");
                    ++i;
                }
                continue;
            }

            if (text[i] == '\n') {
                push_ascii("\n");
                ++i;
                continue;
            }

            std::size_t chunk_start = i;
            while (i < text.size() && text[i] != '\r' && text[i] != '\n') {
                ++i;
            }
            std::string_view part{text.data() + chunk_start, i - chunk_start};
            int pending_spaces = 0;
            std::size_t j = 0;

            while (j < part.size()) {
                if (part[j] == ' ') {
                    ++pending_spaces;
                    ++j;
                    continue;
                }

                std::size_t word_start = j;
                while (j < part.size() && part[j] != ' ') {
                    ++j;
                }

                std::string word{part.substr(word_start, j - word_start)};

                if (pending_spaces > 0) {
                    for (int k = 0; k < pending_spaces - 1; ++k) {
                        push_space_marker();
                    }

                    push_space_prefixed_word(word);
                    pending_spaces = 0;
                } else {
                    push_ascii(word);
                }
            }

            for (int k = 0; k < pending_spaces; ++k) {
                push_space_marker();
            }
        }
        return output;
    }

    export enum class mode: std::uint8_t {
        Most,
        Least
    };

    using BPEMerges = std::pair<int, int>;
    using BPERanks = std::pair<std::u8string, std::u8string>;

    static std::optional<BPEMerges> find_freq_pair(
        const std::vector<std::vector<int> > &token_id_sequences
    ) {
        std::unordered_map<BPEMerges, int, pair_hash> pair_counts{};

        for (const auto &sequence: token_id_sequences) {
            if (sequence.size() < 2) {
                continue;
            }

            for (std::size_t i = 0; i + 1 < sequence.size(); ++i) {
                pair_counts[{sequence[i], sequence[i + 1]}]++;
            }
        }

        if (pair_counts.empty()) {
            return std::nullopt;
        }

        auto best_pair = std::ranges::max_element(
            pair_counts,
            [](const auto &lhs, const auto &rhs) {
                return lhs.second < rhs.second;
            }
        );

        return best_pair->first;
    }

    std::vector<std::vector<int> > replace_pair(
        const std::vector<std::vector<int> > &token_id_sequences,
        BPEMerges pair_id,
        int new_id
    ) {
        std::vector<std::vector<int> > output{};

        for (const auto &sequence: token_id_sequences) {
            std::vector<int> new_sequence{};

            for (std::size_t i = 0; i < sequence.size();) {
                if (
                    i + 1 < sequence.size() &&
                    sequence[i] == pair_id.first &&
                    sequence[i + 1] == pair_id.second
                ) {
                    new_sequence.push_back(new_id);
                    i += 2;
                } else {
                    new_sequence.push_back(sequence[i]);
                    i += 1;
                }
            }

            output.push_back(std::move(new_sequence));
        }

        return output;
    }

    std::vector<std::u8string> split_into_utf8_chars(const std::u8string &text) {
        std::vector<std::u8string> chars{};

        for (std::size_t i = 0; i < text.size();) {
            auto byte = static_cast<unsigned char>(text[i]);

            std::size_t char_size = 1;
            if ((byte & 0b11100000) == 0b11000000) {
                char_size = 2;
            } else if ((byte & 0b11110000) == 0b11100000) {
                char_size = 3;
            } else if ((byte & 0b11111000) == 0b11110000) {
                char_size = 4;
            }

            chars.push_back(text.substr(i, char_size));
            i += char_size;
        }

        return chars;
    }


    export class BPEFromScratch {
        DataStructures::GenericBiDirectionalMap<int, std::u8string> m_vocab_inverse_vocab{};
        std::unordered_map<BPEMerges, int, pair_hash> m_bpe_merges{};
        std::unordered_map<BPERanks, int, pair_hash> m_bpe_ranks{}; // lower rank higher priority
    public:
        std::size_t get_vocab_size() {
            return m_vocab_inverse_vocab.size();
        }
        void train(
            const std::string &text,
            int vocab_size,
            const std::set<std::u8string> &allowed_special = {u8"<|endoftext|>"}
        ) {
            const auto tokens = pre_tokenize_text(text);
            std::vector<std::u8string> unique_chars{};

            for (int i = 0; i < 256; ++i) {
                unique_chars.emplace_back(1, static_cast<char8_t>(i));
            }

            std::set<std::u8string> chars_from_text{};

            for (const auto &token: tokens) {
                for (const auto &ch: split_into_utf8_chars(token)) {
                    chars_from_text.insert(ch);
                }
            }

            for (const auto &ch: chars_from_text) {
                if (!std::ranges::contains(unique_chars, ch)) {
                    unique_chars.push_back(ch);
                }
            }

            if (!std::ranges::contains(unique_chars, u8"Ġ")) {
                unique_chars.push_back(u8"Ġ");
            }

            m_vocab_inverse_vocab.clear();
            m_bpe_merges.clear();

            for (auto [id, ch]: unique_chars | std::views::enumerate) {
                m_vocab_inverse_vocab.insert(id, ch);
            }

            for (auto &special_token: allowed_special) {
                if (!m_vocab_inverse_vocab.contains_value(special_token)) {
                    int new_id = static_cast<int>(m_vocab_inverse_vocab.size());
                    m_vocab_inverse_vocab.insert(new_id, special_token);
                }
            }

            std::vector<std::vector<int> > token_id_sequences{};

            for (const auto &token: tokens) {
                std::vector<int> ids{};

                for (const auto &ch: split_into_utf8_chars(token)) {
                    ids.push_back(m_vocab_inverse_vocab.fetch_by_value(ch));
                }

                token_id_sequences.push_back(std::move(ids));
            }

            for (int new_id = static_cast<int>(m_vocab_inverse_vocab.size()); new_id < vocab_size; ++new_id) {
                auto pair_id = find_freq_pair(token_id_sequences);

                if (!pair_id.has_value()) {
                    break;
                }

                token_id_sequences = replace_pair(token_id_sequences, pair_id.value(), new_id);
                m_bpe_merges[pair_id.value()] = new_id;
            }

            for (int new_id = static_cast<int>(m_vocab_inverse_vocab.size()); new_id < vocab_size; ++new_id) {
                auto pair_id = find_freq_pair(token_id_sequences);

                if (!pair_id.has_value()) {
                    break;
                }

                std::u8string merged_token =
                        m_vocab_inverse_vocab.fetch_by_key(pair_id->first) +
                        m_vocab_inverse_vocab.fetch_by_key(pair_id->second);

                token_id_sequences = replace_pair(token_id_sequences, pair_id.value(), new_id);
                m_bpe_merges[pair_id.value()] = new_id;
                m_vocab_inverse_vocab.insert(new_id, merged_token);
            }
        }

        void load_vocab_and_merges_from_openai(const std::filesystem::path &vocab_path,
                                               const std::filesystem::path &bpe_merges_path) {
            std::ifstream vpcab_file_stream(vocab_path, std::ios::binary);
            if (!vpcab_file_stream) {
                throw std::runtime_error("Could not open Encoder file");
            }

            json loaded_vocab = json::parse(vpcab_file_stream);

            m_vocab_inverse_vocab.clear();
            m_bpe_merges.clear();
            m_bpe_ranks.clear();

            for (auto it = loaded_vocab.begin(); it != loaded_vocab.end(); ++it) {
                std::u8string token = BPEHelpers::to_u8string(it.key());
                int id = it.value().get<int>();

                m_vocab_inverse_vocab.insert(id, token);
            }

            if (
                !m_vocab_inverse_vocab.contains_value(u8"Ċ") ||
                m_vocab_inverse_vocab.fetch_by_value(u8"Ċ") != 198
            ) {
                throw std::runtime_error("Vocabulary missing GPT-2 newline glyph 'Ċ' at id 198.");
            }

            if (
                !m_vocab_inverse_vocab.contains_value(u8"<|endoftext|>") ||
                m_vocab_inverse_vocab.fetch_by_value(u8"<|endoftext|>") != 50256
            ) {
                throw std::runtime_error("Vocabulary missing <|endoftext|> at id 50256.");
            }

            if (!m_vocab_inverse_vocab.contains_value(u8"\n")) {
                m_vocab_inverse_vocab.insert(
                    m_vocab_inverse_vocab.fetch_by_value(u8"Ċ"),
                    u8"\n"
                );
            }

            if (!m_vocab_inverse_vocab.contains_value(u8"\r")) {
                if (m_vocab_inverse_vocab.contains_key(201)) {
                    m_vocab_inverse_vocab.insert(201, u8"\r");
                } else {
                    throw std::runtime_error("Vocabulary missing carriage return token at id 201.");
                }
            }

            std::ifstream merges_file_stream(bpe_merges_path, std::ios::binary);
            if (!merges_file_stream) {
                throw std::runtime_error("Could not open BPE merges file");
            }

            std::string line{};
            int rank = 0;

            while (std::getline(merges_file_stream, line)) {
                if (line.empty()) {
                    continue;
                }

                if (line.starts_with("#")) {
                    continue;
                }

                std::istringstream line_stream(line);

                std::string token1_string{};
                std::string token2_string{};
                std::string extra{};

                line_stream >> token1_string >> token2_string;

                if (token1_string.empty() || token2_string.empty()) {
                    continue;
                }

                if (line_stream >> extra) {
                    continue;
                }
                std::u8string token1 = BPEHelpers::to_u8string(token1_string);
                std::u8string token2 = BPEHelpers::to_u8string(token2_string);

                auto bpe_ranks = BPERanks(token1, token2);

                if (
                    m_vocab_inverse_vocab.contains_value(token1) &&
                    m_vocab_inverse_vocab.contains_value(token2)
                ) {
                    m_bpe_ranks.insert({bpe_ranks, rank});
                    ++rank;
                }
            }
        }

        std::vector<int> tokenize_with_bpe(const std::u8string &token) {
            std::vector<int> token_ids{};

            for (const auto &ch: split_into_utf8_chars(token)) {
                if (!m_vocab_inverse_vocab.contains_value(ch)) {
                    throw std::runtime_error("Character not found in vocab.");
                }

                token_ids.push_back(m_vocab_inverse_vocab.fetch_by_value(ch));
            }

            if (m_bpe_ranks.empty()) {
                bool can_merge = true;

                while (can_merge && token_ids.size() > 1) {
                    can_merge = false;
                    std::vector<int> new_tokens{};

                    std::size_t i = 0;

                    while (i + 1 < token_ids.size()) {
                        BPEMerges pair{token_ids[i], token_ids[i + 1]};

                        if (m_bpe_merges.contains(pair)) {
                            int merged_token_id = m_bpe_merges.at(pair);

                            new_tokens.push_back(merged_token_id);
                            i += 2;
                            can_merge = true;
                        } else {
                            new_tokens.push_back(token_ids[i]);
                            i += 1;
                        }
                    }

                    if (i < token_ids.size()) {
                        new_tokens.push_back(token_ids[i]);
                    }

                    token_ids = std::move(new_tokens);
                }

                return token_ids;
            }

            std::vector<std::u8string> symbols{};

            for (int token_id: token_ids) {
                symbols.push_back(m_vocab_inverse_vocab.fetch_by_key(token_id));
            }

            while (true) {
                std::set<BPERanks> pairs{};

                for (std::size_t i = 0; i + 1 < symbols.size(); ++i) {
                    pairs.insert({symbols[i], symbols[i + 1]});
                }

                if (pairs.empty()) {
                    break;
                }

                int min_rank = std::numeric_limits<int>::max();
                std::optional<BPERanks> bigram = std::nullopt;

                for (const auto &pair: pairs) {
                    if (m_bpe_ranks.contains(pair)) {
                        int rank = m_bpe_ranks.at(pair);

                        if (rank < min_rank) {
                            min_rank = rank;
                            bigram = pair;
                        }
                    }
                }

                if (!bigram.has_value()) {
                    break;
                }

                const auto &[first, second] = bigram.value();

                std::vector<std::u8string> new_symbols{};

                std::size_t i = 0;

                while (i < symbols.size()) {
                    if (
                        i + 1 < symbols.size() &&
                        symbols[i] == first &&
                        symbols[i + 1] == second
                    ) {
                        new_symbols.push_back(first + second);
                        i += 2;
                    } else {
                        new_symbols.push_back(symbols[i]);
                        i += 1;
                    }
                }

                symbols = std::move(new_symbols);

                if (symbols.size() == 1) {
                    break;
                }
            }

            std::vector<int> merged_ids{};

            for (const auto &symbol: symbols) {
                merged_ids.push_back(m_vocab_inverse_vocab.fetch_by_value(symbol));
            }

            return merged_ids;
        }

        std::vector<int> encode(
            const std::string &text,
            const std::optional<std::set<std::u8string> > &allowed_special = std::nullopt
        ) {
            std::vector<int> token_ids{};

            std::vector<std::u8string> specials_in_vocab{};
            for (const auto &token: m_vocab_inverse_vocab.values()) {
                if (BPEHelpers::is_special_token(token)) {
                    specials_in_vocab.push_back(token);
                }
            }

            std::vector<std::string> disallowed{};
            for (const auto &special_token: specials_in_vocab) {
                bool token_is_in_text = text.contains(BPEHelpers::to_string(special_token));
                bool token_is_allowed = allowed_special.has_value() && allowed_special->contains(special_token);

                if (token_is_in_text && !token_is_allowed) {
                    disallowed.push_back(BPEHelpers::to_string(special_token));
                }
            }

            if (!disallowed.empty()) {
                throw std::runtime_error(
                    std::format("Disallowed special tokens encountered in text: {}", disallowed)
                );
            }

            std::string remaining_text = text;

            if (allowed_special.has_value() && !allowed_special->empty()) {
                std::vector<std::u8string> sorted_specials{
                    allowed_special->begin(),
                    allowed_special->end()
                };

                std::ranges::sort(
                    sorted_specials,
                    [](const auto &lhs, const auto &rhs) {
                        return lhs.size() > rhs.size();
                    }
                );

                std::string special_pattern = "(";

                for (std::size_t i = 0; i < sorted_specials.size(); ++i) {
                    if (i != 0) {
                        special_pattern += "|";
                    }

                    special_pattern += BPEHelpers::regex_escape(BPEHelpers::to_string(sorted_specials[i]));
                }

                special_pattern += ")";

                std::regex pattern(special_pattern);

                std::size_t last_index = 0;
                auto begin = std::sregex_iterator(text.begin(), text.end(), pattern);
                auto end = std::sregex_iterator{};

                for (auto it = begin; it != end; ++it) {
                    const auto &match = *it;

                    std::string prefix = text.substr(
                        last_index,
                        static_cast<std::size_t>(match.position()) - last_index
                    );

                    auto prefix_ids = encode(prefix, std::nullopt);
                    token_ids.insert(token_ids.end(), prefix_ids.begin(), prefix_ids.end());

                    std::u8string special_token = BPEHelpers::to_u8string(match.str());

                    if (!m_vocab_inverse_vocab.contains_value(special_token)) {
                        throw std::runtime_error(
                            std::format("Special token {} not found in vocabulary.", match.str())
                        );
                    }

                    token_ids.push_back(m_vocab_inverse_vocab.fetch_by_value(special_token));

                    last_index = static_cast<std::size_t>(match.position() + match.length());
                }

                remaining_text = text.substr(last_index);
            }

            auto tokens = pre_tokenize_text(remaining_text);

            for (const auto &token: tokens) {
                if (m_vocab_inverse_vocab.contains_value(token)) {
                    token_ids.push_back(m_vocab_inverse_vocab.fetch_by_value(token));
                } else {
                    auto bpe_ids = tokenize_with_bpe(token);
                    token_ids.insert(token_ids.end(), bpe_ids.begin(), bpe_ids.end());
                }
            }

            return token_ids;
        }

        std::string decode(const std::vector<int> &token_ids) {
            std::u8string output{};

            for (int token_id: token_ids) {
                if (!m_vocab_inverse_vocab.contains_key(token_id)) {
                    throw std::runtime_error(
                        std::format("Token ID {} not found in vocab.", token_id)
                    );
                }

                std::u8string token = m_vocab_inverse_vocab.fetch_by_key(token_id);

                if (token_id == 198 || token == u8"\n" || token == u8"Ċ") {
                    output += u8"\n";
                } else if (token_id == 201 || token == u8"\r") {
                    output += u8"\r";
                } else if (token.starts_with(u8"Ġ")) {
                    output += u8" ";
                    output += token.substr(std::u8string{u8"Ġ"}.size());
                } else {
                    output += token;
                }
            }

            return std::string(
                reinterpret_cast<const char *>(output.data()),
                output.size()
            );
        }

        void save_vocab_and_merges(
            const std::filesystem::path &vocab_path,
            const std::filesystem::path &bpe_merges_path
        ) {
            json vocab_json = json::object();

            for (const auto &[id, token]: m_vocab_inverse_vocab.items()) {
                std::string token_string{
                    reinterpret_cast<const char *>(token.data()),
                    token.size()
                };

                vocab_json[std::to_string(id)] = token_string;
            }

            std::ofstream vocab_file(vocab_path, std::ios::binary);
            if (!vocab_file) {
                throw std::runtime_error("Could not open vocab file for writing.");
            }

            vocab_file << vocab_json.dump(2);

            json merges_json = json::array();

            for (const auto &[pair, new_id]: m_bpe_merges) {
                merges_json.push_back({
                    {"pair", {pair.first, pair.second}},
                    {"new_id", new_id}
                });
            }

            std::ofstream merges_file(bpe_merges_path, std::ios::binary);
            if (!merges_file) {
                throw std::runtime_error("Could not open BPE merges file for writing.");
            }

            merges_file << merges_json.dump(2);
        }

        void load_vocab_and_merges(
            const std::filesystem::path &vocab_path,
            const std::filesystem::path &bpe_merges_path
        ) {
            std::ifstream vocab_file(vocab_path, std::ios::binary);
            if (!vocab_file) {
                throw std::runtime_error("Could not open vocab file.");
            }

            json vocab_json = json::parse(vocab_file);

            m_vocab_inverse_vocab.clear();
            m_bpe_merges.clear();

            for (auto it = vocab_json.begin(); it != vocab_json.end(); ++it) {
                int id = std::stoi(it.key());
                std::string token_string = it.value().get<std::string>();

                std::u8string token{
                    reinterpret_cast<const char8_t *>(token_string.data()),
                    token_string.size()
                };

                m_vocab_inverse_vocab.insert(id, token);
            }

            std::ifstream merges_file(bpe_merges_path, std::ios::binary);
            if (!merges_file) {
                throw std::runtime_error("Could not open BPE merges file.");
            }

            json merges_json = json::parse(merges_file);

            for (const auto &merge: merges_json) {
                auto pair_json = merge.at("pair");

                BPEMerges pair{
                    pair_json.at(0).get<int>(),
                    pair_json.at(1).get<int>()
                };

                int new_id = merge.at("new_id").get<int>();

                m_bpe_merges[pair] = new_id;
            }
        }

        auto get_special_token_id(const std::u8string &token) {
            return m_vocab_inverse_vocab.fetch_by_value(token);
        }
    };
}

//
// Created by Adeel on 6/17/26.
//

export module bi_directional_map;

import std;

namespace DataStructures {
    export class BiDirectionalMap {
        std::unordered_map<std::string, int> m_vocab_to_id{};
        std::unordered_map<int, std::string> m_id_to_vocab{};
        std::string m_unknown_token{"[UNK]"};
        int m_unknown_id{-1};

    public:
        explicit BiDirectionalMap() = default;

        void insert(int key, const std::string &value) {
            auto [value_it, value_inserted] = m_vocab_to_id.emplace(value, key);
            auto [key_it, key_inserted] = m_id_to_vocab.emplace(key, value);

            if (!value_inserted || !key_inserted) {
                throw std::runtime_error("bi_directional_map insert failed");
            }
        }

        bool contains(const std::string& value) {
            return m_vocab_to_id.contains(value);
        }

        [[nodiscard]] std::string fetch_by_key(int key) {
            return m_id_to_vocab.at(key);
        }

        [[nodiscard]] int fetch_by_value(const std::string &value) {
            return m_vocab_to_id.at(value);
        }

        [[nodiscard]] std::size_t size() {
            if (m_id_to_vocab.size() != m_vocab_to_id.size()) {
                throw std::runtime_error("Something is wrong");
            }
            return m_id_to_vocab.size();
        }

        [[nodiscard]] std::string to_string() const {
            std::string output = "{";

            bool first = true;
            for (const auto& [id, token] : m_id_to_vocab) {
                if (!first) {
                    output += ", ";
                }

                output += std::format("{}: \"{}\"", id, token);
                first = false;
            }

            output += "}";
            return output;
        }
    };
}


template <>
struct std::formatter<DataStructures::BiDirectionalMap> : std::formatter<std::string> {
    auto format(const DataStructures::BiDirectionalMap& map, format_context& ctx) const {
        return std::formatter<std::string>::format(map.to_string(), ctx);
    }
};
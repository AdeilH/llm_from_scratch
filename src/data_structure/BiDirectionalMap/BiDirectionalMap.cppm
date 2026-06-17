//
// Created by Adeel on 6/17/26.
//

export module bi_directional_map;

import std;

// make generic for gpt bpe class to easily use it with multiple encoder decoder places
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
    export template<typename Key, typename Value>
    requires std::formattable<Key, char> && std::formattable<Value, char>
    class GenericBiDirectionalMap {
        std::unordered_map<Key, Value> m_key_to_value{};
        std::unordered_map<Value, Key> m_value_to_key{};

    public:
        explicit GenericBiDirectionalMap() = default;

        void insert(int key, const std::string &value) {
            auto [value_it, value_inserted] = m_key_to_value.emplace(key, value);
            auto [key_it, key_inserted] = m_value_to_key.emplace(value, key);

            if (!value_inserted || !key_inserted) {
                throw std::runtime_error("bi_directional_map insert failed");
            }
        }

        bool contains(const Value& value) {
            return m_key_to_value.contains(value);
        }

        [[nodiscard]] Value fetch_by_key(int key) {
            return m_key_to_value.at(key);
        }

        [[nodiscard]] Key fetch_by_value(const Value& value) {
            return m_value_to_key.at(value);
        }

        [[nodiscard]] std::size_t size() {
            if (m_value_to_key.size() != m_key_to_value.size()) {
                throw std::runtime_error("Something is wrong");
            }
            return m_value_to_key.size();
        }

        [[nodiscard]] std::string to_string() const {
            std::string output = "{";

            bool first = true;
            for (const auto& [id, token] : m_value_to_key) {
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
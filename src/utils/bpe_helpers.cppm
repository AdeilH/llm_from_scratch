//
// Created by Adeel on 6/25/26.
//

module;

// replace with module in future
#include <nlohmann/json.hpp>

export module bpe_helpers;
import std;

using json = nlohmann::json;

export namespace BPEHelpers {
    std::unique_ptr<json> read_encoder_data(const std::filesystem::path &filepath) {
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
    auto to_u8string(const std::string &value) {
        return std::u8string(
            reinterpret_cast<const char8_t *>(value.data()),
            value.size()
        );
    }

    std::string to_string(const std::u8string &value) {
        return std::string(
            reinterpret_cast<const char *>(value.data()),
            value.size()
        );
    }

    bool is_special_token(const std::u8string &token) {
        const std::string token_string = to_string(token);

        return token_string.starts_with("<|") && token_string.ends_with("|>");
    }

    bool contains_text(const std::string &text, const std::u8string &token) {
        return text.contains(to_string(token));
    }

    std::string regex_escape(const std::string &text) {
        std::string output{};

        for (char ch: text) {
            if (
                ch == '\\' || ch == '.' || ch == '^' || ch == '$' ||
                ch == '|' || ch == '(' || ch == ')' || ch == '[' ||
                ch == ']' || ch == '{' || ch == '}' || ch == '*' ||
                ch == '+' || ch == '?' || ch == '/'
            ) {
                output.push_back('\\');
            }

            output.push_back(ch);
        }

        return output;
    }

}
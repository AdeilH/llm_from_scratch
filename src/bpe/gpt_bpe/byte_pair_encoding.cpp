// auto encode(const std::string& text) -> std::vector<int> {
//     std::vector<int> bpe_tokens{};
//
//     std::regex pat(
//         R"('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^[:space:][:alpha:][:digit:]]+|[[:space:]]+(?!\S)|[[:space:]]+)"
//     );
//
//     auto begin = std::sregex_iterator(text.begin(), text.end(), pat);
//     auto end = std::sregex_iterator{};
//
//     for (auto it = begin; it != end; ++it) {
//         std::string token = it->str();
//
//         std::string encoded_token{};
//
//         for (unsigned char byte : token) {
//             const auto& mapped = m_bytes_encoder.at(static_cast<int>(byte));
//             encoded_token += std::string(
//                 reinterpret_cast<const char*>(mapped.data()),
//                 mapped.size()
//             );
//         }
//
//         std::string bpe_result = bpe(encoded_token);
//
//         auto parts = bpe_result | std::views::split(' ');
//
//         for (auto part : parts) {
//             std::string bpe_token{part.begin(), part.end()};
//             bpe_tokens.push_back(m_encoder.at(bpe_token));
//         }
//     }
//
//     return bpe_tokens;
// }

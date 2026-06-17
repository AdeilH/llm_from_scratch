//
// Created by Adeel on 6/17/26.
//

export module lru_cache;

import std;

// // 1 2 3 4 5 6 7
// 2 1 3 4 5 6 7
namespace DataStructures {
    export template<typename T, std::size_t N>
    class LRUCache {
        std::deque<T> m_cache{};

    public:
        void insert(T elem) {
            if (m_cache.size() > N) {
                m_cache.pop_back();
            }
            auto loc = std::ranges::find(m_cache, elem);
            if (loc != m_cache.end()) {
                std::print("{}", m_cache);
                std::ranges::rotate(m_cache.begin(), loc, loc + 1);
            } else {
                m_cache.emplace_front(elem);
            }
        }

        bool contains(T elem) {
            auto iter = std::ranges::find(m_cache, elem);
            if (iter != m_cache.end()) {
                return true;
            }
            return false;
        }

        [[nodiscard]] std::string to_string() const {
            std::string output = "{";

            bool first = true;
            for (const auto &key: m_cache) {
                if (!first) {
                    output += ",";
                }

                output += std::format("{} ", key);
                first = false;
            }

            output += "}";
            return output;
        }
    };

    template<std::size_t N>
    class LRUCache<std::pair<std::string, int>, N> {

        // Stores pairs of {std::string key, int value}
        std::list<std::pair<std::string, int>> cacheList;

        // Maps the string key to its exact iterator position inside cacheList
        std::unordered_map<std::string, std::list<std::pair<std::string, int>>::iterator> cacheMap;

    public:
        LRUCache() {}

        int get(const std::string& key) {
            // Step 1: Look for the key in the hash map
            auto mapIt = cacheMap.find(key);
            if (mapIt == cacheMap.end()) {
                return -1; // Not found
            }

            // Step 2: Get the iterator pointing to the list node
            auto listIt = mapIt->second;

            // Step 3: Move node to the front (Most Recently Used) in O(1) time
            cacheList.splice(cacheList.begin(), cacheList, listIt);

            // Step 4: Return the integer value
            return listIt->second;
        }

        void put(const std::string& key, int value) {
            auto mapIt = cacheMap.find(key);

            // Scenario 1: Key already exists. Update value and move node to front.
            if (mapIt != cacheMap.end()) {
                auto listIt = mapIt->second;
                listIt->second = value;
                cacheList.splice(cacheList.begin(), cacheList, listIt);
                return;
            }

            // Scenario 2: Cache is at full capacity. Evict the Least Recently Used item.
            if (cacheList.size() == N) {
                // Get the pair at the back of the list
                auto lruPair = cacheList.back();

                // Erase the string key from the map and drop the node from the list
                cacheMap.erase(lruPair.first);
                cacheList.pop_back();
            }

            // Scenario 3: Insert the new string-int pair at the front of the list
            cacheList.push_front({key, value});

            // Save the new iterator address in the hash map
            cacheMap[key] = cacheList.begin();
        }
    };
}

template<typename Key, int Value>
struct std::formatter<DataStructures::LRUCache<Key, Value> > : std::formatter<std::string> {
    auto format(const DataStructures::LRUCache<Key, Value> &cache, std::format_context &ctx) const {
        return std::formatter<std::string>::format(cache.to_string(), ctx);
    }
};

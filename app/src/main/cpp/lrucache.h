//
// Created by Hoque, Mohammad on 03/02/2019.
//

#ifndef QOSMOS_LRUCACHE_H
#define QOSMOS_LRUCACHE_H


#include <unordered_map>
#include <list>
#include <cstddef>
#include <stdexcept>

namespace cache {

    template<typename key_t, typename value_t>
    class lru_cache {
    public:
        typedef typename std::pair<key_t, value_t> key_value_pair_t;
        typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

        lru_cache(size_t max_size) :
                _max_size(max_size) {
        }

        void put(const key_t& key, const value_t& value) {
            auto it = _cache_items_map.find(key);
            if(it != _cache_items_map.end()) {
                _cache_items_list.erase(it->second);
                _cache_items_map.erase(it);
            }

            _cache_items_list.push_front(key_value_pair_t(key, value));
            _cache_items_map[key] = _cache_items_list.begin();

            if(_cache_items_map.size() > _max_size) {
                auto last = _cache_items_list.end();
                last--;
                _cache_items_map.erase(last->first);
                _cache_items_list.pop_back();
            }
        }

        value_t& get(const key_t& key) {
            auto it = _cache_items_map.find(key);
            if(it == _cache_items_map.end()) {
                throw std::range_error("There is no such key in cache");
            } else {
                _cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
                return it->second->second;
            }
        }

        void erase_if_exists(const key_t& key) {
            auto it = _cache_items_map.find(key);
            if (it != _cache_items_map.end()) {
                _cache_items_list.erase(it->second);
                _cache_items_map.erase(it);
            }
        }

        template<typename F>
        void erase_if(const F &f) {
            for (auto it = std::begin(_cache_items_map); it != std::end(_cache_items_map);) {
                if (f(it->second->second)) {
                    _cache_items_list.erase(it->second);
                    it = _cache_items_map.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        void clear() {
            _cache_items_map.clear();
            _cache_items_list.clear();
        }

        bool exists(const key_t& key) const {
            return _cache_items_map.find(key) != _cache_items_map.end();
        }

        size_t size() const {
            return _cache_items_map.size();
        }

    private:
        std::list<key_value_pair_t> _cache_items_list;
        std::unordered_map<key_t, list_iterator_t> _cache_items_map;
        size_t _max_size;
    };

} // namespace lru

#endif //QOSMOS_LRUCACHE_H

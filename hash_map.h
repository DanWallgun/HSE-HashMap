#include <algorithm>
#include <stdexcept>
#include <vector>
#include <utility>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
  public:
    class iterator {
      public:
        iterator()
        : hashmap_(nullptr)
        {}
        iterator& operator=(const iterator& other) {
            i = other.i;
            j = other.j;
            hashmap_ = other.hashmap_;
            return *this;
        }
        iterator(size_t i, size_t j, const HashMap<KeyType, ValueType, Hash>& h)
        : i(i)
        , j(j)
        , hashmap_(&h)
        {}
        bool operator==(const iterator& other) const {
            return i == other.i && j == other.j;
        }
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
        iterator& operator++() {
            if (i == hashmap_->capacity_) {
                return *this;
            }
            if (j + 1 >= hashmap_->data_[i].size()) {
                ++i;
                j = 0;
                if (i < hashmap_->data_.size()) {
                    while (hashmap_->data_[i].empty()) {
                        ++i;
                        if (i == hashmap_->data_.size()) break;
                    }
                }
            } else {
                ++j;
            }
            return *this;
        }
        iterator operator++(int) {
            iterator it = *this;
            ++*this;
            return it;
        }
        std::pair<const KeyType, ValueType>& operator*() {
            return reinterpret_cast<std::pair<const KeyType, ValueType>&>(
                hashmap_->data_[i][j]);
        }
        std::pair<const KeyType, ValueType>* operator->() {
            return reinterpret_cast<std::pair<const KeyType, ValueType>*>(
                &hashmap_->data_[i][j]);
        }

      private:
        size_t i, j;
        HashMap<KeyType, ValueType, Hash>* hashmap_;
    };

    class const_iterator {
      public:
        const_iterator()
        : hashmap_(nullptr)
        {}
        const_iterator& operator=(const const_iterator& other) {
            i = other.i;
            j = other.j;
            hashmap_ = other.hashmap_;
            return *this;
        }
        const_iterator(size_t i, size_t j,
                       const HashMap<KeyType, ValueType, Hash>& h)
        : i(i)
        , j(j)
        , hashmap_(&h)
        {}
        bool operator==(const const_iterator& other) const {
            return i == other.i && j == other.j;
        }
        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }
        const_iterator& operator++() {
            if (i == hashmap_->capacity_) {
                return *this;
            }
            if (j + 1 >= hashmap_->data_[i].size()) {
                ++i;
                j = 0;
                if (i < hashmap_->data_.size()) {
                    while (hashmap_->data_[i].empty()) {
                        ++i;
                        if (i == hashmap_->data_.size()) break;
                    }
                }
            } else {
                ++j;
            }
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator it = *this;
            ++*this;
            return it;
        }
        const std::pair<const KeyType, ValueType>& operator*() {
            return
                reinterpret_cast<const std::pair<const KeyType, ValueType>&>(
                    hashmap_->data_[i][j]);
        }
        const std::pair<const KeyType, ValueType>* operator->() {
            return
                reinterpret_cast<const std::pair<const KeyType, ValueType>*>(
                    &hashmap_->data_[i][j]);
        }

      private:
        size_t i, j;
        const HashMap<KeyType, ValueType, Hash>* hashmap_;
    };
    HashMap(Hash hasher = Hash())
    : hash_(hasher)
    , data_(capacity_)
    {}
    template<class It>
    HashMap(It b, It e, Hash hasher = Hash())
    : hash_(hasher)
    , data_(capacity_) {
        while (b != e) {
            insert(*b);
            ++b;
        }
    }
    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list,
            Hash hasher = Hash())
    : hash_(hasher)
    , data_(capacity_) {
        for (const auto& it : list) {
            insert(it);
        }
    }
    iterator begin() {
        size_t i = 0, j = 0;
        while (i < capacity_ && data_[i].empty()) {
            ++i;
        }
        return iterator(i, j, *this);
    }
    const_iterator begin() const {
        size_t i = 0, j = 0;
        while (i < capacity_ && data_[i].empty()) {
            ++i;
        }
        return const_iterator(i, j, *this);
    }
    iterator end() {
        return iterator(capacity_, 0, *this);
    }
    const_iterator end() const {
        return const_iterator(capacity_, 0, *this);
    }
    void insert(const std::pair<KeyType, ValueType>& element) {
        size_t h = get_hash_(element.first);
        for (size_t i = 0; i < data_[h].size(); ++i)
            if (data_[h][i].first == element.first)
                return;
        data_[h].push_back(element);
        ++size_;
        check_rehash();
    }
    iterator find(const KeyType& key) {
        size_t h = get_hash_(key);
        for (size_t i = 0; i < data_[h].size(); ++i) {
            if (data_[h][i].first == key)
                return iterator(h, i, *this);
        }
        return iterator(capacity_, 0, *this);
    }
    const_iterator find(const KeyType& key) const {
        size_t h = get_hash_(key);
        for (size_t i = 0; i < data_[h].size(); ++i)
            if (data_[h][i].first == key)
                return const_iterator(h, i, *this);
        return const_iterator(capacity_, 0, *this);
    }
    void erase(const KeyType& key) {
        size_t h = get_hash_(key);
        auto it = std::remove_if(data_[h].begin(), data_[h].end(),
            [&key] (const std::pair<KeyType, ValueType>& p) {
            return p.first == key;
            });
        size_t prev_size = data_[h].size();
        data_[h].erase(
            it,
            data_[h].end());
        size_t new_size = data_[h].size();
        size_ -= prev_size - new_size;
        check_rehash();
    }
    ValueType& operator[](const KeyType& key) {
        size_t h = get_hash_(key);
        for (size_t i = 0; i < data_[h].size(); ++i)
            if (data_[h][i].first == key)
                return data_[h][i].second;
        insert({key, ValueType()});
        return (*this)[key];
    }
    const ValueType& at(const KeyType& key) const {
        size_t h = get_hash_(key);
        for (size_t i = 0; i < data_[h].size(); ++i)
            if (data_[h][i].first == key)
                return data_[h][i].second;
        throw std::out_of_range("");
    }
    Hash hash_function() const {
        return hash_;
    }
    void clear() {
        for (auto& bucket : data_) {
            bucket.clear();
        }
        size_ = 0;
    }
    size_t size() const {
        return size_;
    }
    bool empty() const {
        return size_ == 0;
    }

  private:
    size_t size_ = 0;
    size_t capacity_ = (size_t) 5e5;
    Hash hash_;
    std::vector<std::vector<std::pair<KeyType, ValueType>>> data_;
    size_t get_hash_(const KeyType& key) const {
        return hash_(key) % capacity_;
    }
    void check_rehash() {
        if (size_ > capacity_ || 3 * size_ < capacity_) {
            rehash();
        }
    }
    void rehash() {
        std::vector<std::pair<KeyType, ValueType>> arr;
        for (size_t i = 0; i < capacity_; ++i) {
            for (size_t j = 0; j < data_[i].size(); ++j) {
                arr.push_back(data_[i][j]);
            }
        }
        capacity_ = 2 * (size_ + 1);
        data_.clear();
        data_.resize(capacity_);
        for (const auto& i : arr) {
            data_[get_hash_(i.first)].push_back(i);
        }
    }
};

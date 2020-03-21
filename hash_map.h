#include <algorithm>
#include <stdexcept>
#include <vector>
#include <utility>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
    using Element = std::pair<KeyType, ValueType>;
    using ConstKeyElement = std::pair<const KeyType, ValueType>;
  public:
    class iterator {
      public:
        iterator()
        : hashmap_(nullptr)
        {}
        iterator& operator=(const iterator& other) {
            bucketPointer = other.bucketPointer;
            innerPointer = other.innerPointer;
            hashmap_ = other.hashmap_;
            return *this;
        }
        iterator(size_t bPtr, size_t iPtr, HashMap<KeyType, ValueType, Hash>& h)
        : bucketPointer(bPtr)
        , innerPointer(iPtr)
        , hashmap_(&h)
        {}
        bool operator==(const iterator& other) const {
            return bucketPointer == other.bucketPointer &&
                   innerPointer == other.innerPointer;
        }
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
        iterator& operator++() {
            if (bucketPointer == hashmap_->capacity_) {
                return *this;
            }
            if (innerPointer + 1 >= hashmap_->data_[bucketPointer].size()) {
                ++bucketPointer;
                innerPointer = 0;
                if (bucketPointer < hashmap_->data_.size()) {
                    while (hashmap_->data_[bucketPointer].empty()) {
                        ++bucketPointer;
                        if (bucketPointer == hashmap_->data_.size()) break;
                    }
                }
            } else {
                ++innerPointer;
            }
            return *this;
        }
        iterator operator++(int) {
            iterator it = *this;
            ++*this;
            return it;
        }
        ConstKeyElement& operator*() {
            return reinterpret_cast<ConstKeyElement&>(
                hashmap_->data_[bucketPointer][innerPointer]);
        }
        ConstKeyElement* operator->() {
            return reinterpret_cast<ConstKeyElement*>(
                &hashmap_->data_[bucketPointer][innerPointer]);
        }

      private:
        size_t bucketPointer, innerPointer;
        HashMap<KeyType, ValueType, Hash>* hashmap_;
    };

    class const_iterator {
      public:
        const_iterator()
        : hashmap_(nullptr)
        {}
        const_iterator& operator=(const const_iterator& other) {
            bucketPointer = other.bucketPointer;
            innerPointer = other.innerPointer;
            hashmap_ = other.hashmap_;
            return *this;
        }
        const_iterator(size_t bPtr, size_t iPtr,
                       const HashMap<KeyType, ValueType, Hash>& h)
        : bucketPointer(bPtr)
        , innerPointer(iPtr)
        , hashmap_(&h)
        {}
        bool operator==(const const_iterator& other) const {
            return bucketPointer == other.bucketPointer &&
                   innerPointer == other.innerPointer;
        }
        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }
        const_iterator& operator++() {
            if (bucketPointer == hashmap_->capacity_) {
                return *this;
            }
            if (innerPointer + 1 >= hashmap_->data_[bucketPointer].size()) {
                ++bucketPointer;
                innerPointer = 0;
                if (bucketPointer < hashmap_->data_.size()) {
                    while (hashmap_->data_[bucketPointer].empty()) {
                        ++bucketPointer;
                        if (bucketPointer == hashmap_->data_.size()) {
                            break;
                        }
                    }
                }
            } else {
                ++innerPointer;
            }
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator it = *this;
            ++*this;
            return it;
        }
        const ConstKeyElement& operator*() {
            return
                reinterpret_cast<const ConstKeyElement&>(
                    hashmap_->data_[bucketPointer][innerPointer]);
        }
        const ConstKeyElement* operator->() {
            return
                reinterpret_cast<const ConstKeyElement*>(
                    &hashmap_->data_[bucketPointer][innerPointer]);
        }

      private:
        size_t bucketPointer, innerPointer;
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
    HashMap(std::initializer_list<Element> list,
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
    void insert(const Element& element) {
        size_t elementHash = get_hash(element.first);
        for (size_t i = 0; i < data_[elementHash].size(); ++i) {
            if (data_[elementHash][i].first == element.first) {
                return;
            }
        }
        data_[elementHash].push_back(element);
        ++size_;
        check_rehash();
    }
    iterator find(const KeyType& key) {
        size_t elementHash = get_hash(key);
        for (size_t i = 0; i < data_[elementHash].size(); ++i) {
            if (data_[elementHash][i].first == key) {
                return iterator(elementHash, i, *this);
            }
        }
        return iterator(capacity_, 0, *this);
    }
    const_iterator find(const KeyType& key) const {
        size_t elementHash = get_hash(key);
        for (size_t i = 0; i < data_[elementHash].size(); ++i) {
            if (data_[elementHash][i].first == key) {
                return const_iterator(elementHash, i, *this);
            }
        }
        return const_iterator(capacity_, 0, *this);
    }
    void erase(const KeyType& key) {
        size_t elementHash = get_hash(key);
        auto it = std::remove_if(data_[elementHash].begin(), data_[elementHash].end(),
            [&key] (const Element& p) {
                return p.first == key;
            });
        size_t prev_size = data_[elementHash].size();
        data_[elementHash].erase(
            it,
            data_[elementHash].end());
        size_t new_size = data_[elementHash].size();
        size_ -= prev_size - new_size;
        check_rehash();
    }
    ValueType& operator[](const KeyType& key) {
        size_t elementHash = get_hash(key);
        for (size_t i = 0; i < data_[elementHash].size(); ++i) {
            if (data_[elementHash][i].first == key) {
                return data_[elementHash][i].second;
            }
        }
        insert({key, ValueType()});
        return (*this)[key];
    }
    const ValueType& at(const KeyType& key) const {
        size_t elementHash = get_hash(key);
        for (size_t i = 0; i < data_[elementHash].size(); ++i) {
            if (data_[elementHash][i].first == key) {
                return data_[elementHash][i].second;
            }
        }
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
    static const size_t CAPACITY_MULTIPLIER = 2;
    size_t size_ = 0;
    size_t capacity_ = (size_t) 5e5;
    Hash hash_;
    std::vector<std::vector<Element>> data_;
    size_t get_hash(const KeyType& key) const {
        return hash_(key) % capacity_;
    }
    void check_rehash() {
        if (size_ > capacity_ || 3 * size_ < capacity_) {
            rehash();
        }
    }
    void rehash() {
        std::vector<Element> arr;
        for (size_t i = 0; i < capacity_; ++i) {
            for (size_t j = 0; j < data_[i].size(); ++j) {
                arr.push_back(data_[i][j]);
            }
        }
        capacity_ = CAPACITY_MULTIPLIER * (size_ + 1);
        data_.clear();
        data_.resize(capacity_);
        for (const auto& i : arr) {
            data_[get_hash(i.first)].push_back(i);
        }
    }
};

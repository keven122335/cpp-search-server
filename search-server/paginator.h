#pragma once
#include <iostream> 
#include <vector>
#include <algorithm>

template<typename Iterator>
class IteratorRange {
public:
    explicit IteratorRange(Iterator iterator_begin, Iterator iterator_end, size_t page_size)
        :begin_(iterator_begin), end_(iterator_end), size_(page_size) {};

    Iterator begin() {
        return begin_;
    }

    Iterator end() {
        return end_;
    }

    size_t size() {
        return size_;
    }
private:
    Iterator begin_;
    Iterator end_;
    size_t size_;

};

template<typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator begin, Iterator end, size_t list_size) {

        for (size_t i = distance(begin, end); i > 0;) {
            const size_t current_list = std::min(list_size, i);
            const Iterator next_list_end = next(begin, current_list);
            IteratorRange<Iterator>it_range(begin, next_list_end, list_size);
            doc_finds_.push_back(it_range);

            i -= current_list;
            begin = next_list_end;
        }
    }

    auto begin() const {
        return doc_finds_.begin();
    }

    auto end() const {
        return doc_finds_.end();
    }

    size_t size() const {
        return doc_finds_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> doc_finds_;
};

template <typename Iterator>
std::ostream& operator <<(std::ostream& out, IteratorRange<Iterator> it_range) {
    for (auto it = it_range.begin(); it != it_range.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator<typename Container::const_iterator>(c.begin(), c.end(), page_size);
}



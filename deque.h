#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include "exceptions.h"

#include <cstddef>
#include <cmath>

// T没有默认构造函数
namespace sjtu {
    template<class T>
    class deque {
    public:
        class Block {
        public:
            T **data;
            size_t capacity; // 当前块的容量。注意到数组元素的个数可以通过head和tail算出来
            size_t size;
            size_t head, tail; // 循环数组的头尾指针,尾指针在最后一个元素的后面
            Block *next;
            Block *pre;

            Block(size_t capa = 128): capacity(capa), size(0), head(0), tail(0), pre(nullptr), next(nullptr) {
                data = new T *[capacity];
                for (size_t i = 0; i < capacity; i++) {
                    data[i] = nullptr;
                }
            }

            ~Block() {
                for (size_t i = 0; i < size; i++) {
                    delete data[(i + head) % capacity];
                }
                delete[] data;
            }

            size_t get_size() const {
                return size;
            }

            bool isFull() const {
                return size == capacity;
            }

            bool isUnderflow() const {
                // 不到0.25时合并
                return 4 * size <= capacity;
            }
        };

        Block *head_block;
        Block *tail_block;
        size_t total_size; // 总元素数量
        size_t block_count; // 块的数量

        // 理想的块容量 2\sqrt{n}
        size_t idealCapacity() const {
            return std::max(static_cast<size_t>(2 * std::sqrt(total_size)), static_cast<size_t>(128));
        }

        // 块分裂
        void splitBlock(Block *block) {
            Block *new_block = new Block(block->capacity);
            size_t mid = (block->size) >> 1;

            // 迁移后面一半的元素到新块中,但是必须重新分配内存
            for (size_t i = 0; i < (block->size % 2 == 0 ? mid : mid + 1); i++) {
                T *temp = block->data[(block->head + i + mid) % block->capacity];
                new_block->data[new_block->tail++] = new T(*temp);
                delete temp;
                block->data[(block->head + i + mid
                            ) % block->capacity] = nullptr;
            }
            new_block->size = block->size - mid;
            block->tail = (block->head + mid) % block->capacity;
            block->size = mid;

            // 链接
            new_block->next = block->next;
            new_block->pre = block;
            if (block->next != nullptr) {
                block->next->pre = new_block;
            }
            block->next = new_block;

            // 尾块修改
            if (block == tail_block) {
                tail_block = new_block;
            }
            block_count++;
        }

        // 合并块
        Block *mergeBlock(Block *left, Block *right) {
            size_t new_size = left->size + right->size;
            size_t p = static_cast<size_t>(log2(new_size)) + 1;
            Block *new_block = new Block(static_cast<size_t>(std::pow(2, p)));
            for (size_t i = 0; i < left->size; i++) {
                T *temp = left->data[(left->head + i) % left->capacity];
                new_block->data[new_block->tail++] = new T(*temp);
            }
            for (size_t i = 0; i < right->size; i++) {
                T *temp = right->data[(right->head + i) % right->capacity];
                new_block->data[new_block->tail++] = new T(*temp);
                // delete temp;
                // right->data[(right->head + i) % right->capacity] = nullptr;
            }
            new_block->size = left->size + right->size;
            new_block->next = right->next;
            new_block->pre = left->pre;
            if (left->pre != nullptr) {
                left->pre->next = new_block;
            }
            if (right->next != nullptr) {
                right->next->pre = new_block;
            }

            if (head_block == left) {
                head_block = new_block;
            }
            if (tail_block == right) {
                tail_block = new_block;
            }

            // delete[] left->data;
            // delete[] right->data;
            delete left;
            delete right;
            block_count--;
            return new_block;
        }

        // 容积扩充,且block不失效
        Block *doubleSpace(Block *block) {
            size_t new_capacity = block->capacity << 1;
            Block *new_block = new Block(new_capacity);
            for (size_t i = 0; i < block->size; i++) {
                T *temp = block->data[(block->head + i) % block->capacity];
                new_block->data[new_block->tail++] = new T(*temp);
                // delete temp;
                // block->data[(block->head + i) % block->capacity] = nullptr;
            }
            new_block->size = block->size;
            new_block->next = block->next;
            new_block->pre = block->pre;
            if (block->pre != nullptr) {
                block->pre->next = new_block;
            }
            if (block->next != nullptr) {
                block->next->pre = new_block;
            }
            if (block == head_block) {
                head_block = new_block;
            }
            if (block == tail_block) {
                tail_block = new_block;
            }

            // delete[] block->data;
            delete block;
            return new_block;
        }

        void check() {
            if (block_count > 1) {
                Block *current = head_block;
                if (current->size > 4 * idealCapacity()) {
                    splitBlock(current);
                }
                current = current->next;
                while (current != nullptr) {
                    Block *next_block = current->next;
                    if (idealCapacity() / 2 > current->size + current->pre->size) {
                        current = mergeBlock(current->pre, current);
                    }
                    if (current->size > 4 * idealCapacity()) {
                        splitBlock(current);
                    }
                    current = next_block;
                }
            }
        }

        class const_iterator;

        class iterator {
        public:
            /**
             * add data members.
             * just add whatever you want.
             */
            Block *cur_block;
            size_t index; // 块内索引,0表示指向head
            size_t cur; // 当前元素的索引
            deque *parent; // 用于验证是不是同一个deque
            bool is_end;

            iterator(Block *cur_block = nullptr, size_t index = 0, size_t cur = 0,
                     deque *parent = nullptr, bool is_end = false) : cur_block(cur_block), index(index), cur(cur),
                                                                     parent(parent), is_end(is_end) {
            }

            /**
             * return a new iterator which points to the n-next element.
             * if there are not enough elements, the behaviour is undefined.
             * same for operator-.
             */
            iterator operator+(const int &n) {
                iterator temp = *this;
                if ((int) cur + n > (int) parent->total_size || (int) cur + n < 0) {
                    throw index_out_of_bound();
                }
                if (n < 0) {
                    temp -= -n;
                } else {
                    temp += n;
                }
                return temp;
            }

            iterator operator-(const int &n) {
                iterator temp = *this;
                if ((int) cur - n < 0 || (int) cur - n > (int) parent->total_size) {
                    throw index_out_of_bound();
                }
                if (n < 0) {
                    temp += -n;
                } else {
                    temp -= n;
                }
                return temp;
            }

            /**
             * return the distance between two iterators.
             * if they point to different vectors, throw
             * invaild_iterator.
             */
            int operator-(const iterator &rhs) const {
                // if (parent != rhs.parent || cur_block != rhs.cur_block) {
                //     throw invalid_iterator();
                // }
                if (parent != rhs.parent) {
                    throw invalid_iterator();
                }

                int a1 = is_end ? parent->total_size : cur;
                int a2 = rhs.is_end ? rhs.parent->total_size : rhs.cur;

                return a1 - a2;
            }

            iterator &operator+=(const int &n) {
                if ((int) cur + n > (int) parent->total_size || (int) cur + n < 0) {
                    throw index_out_of_bound();
                }
                if (n < 0) {
                    return *this -= -n;
                }
                // if (cur_block==nullptr) {
                //     if (n>0) {
                //         throw index_out_of_bound();
                //     }
                //     else {
                //         return *this;
                //     }
                // }
                if (n == 0) {
                    return *this;
                }
                if (is_end) {
                    throw index_out_of_bound();
                }
                if (n + index < cur_block->size) {
                    cur += n;
                    index += n;
                    return *this;
                } else {
                    int m = n;
                    cur += n;
                    if (cur == parent->total_size) {
                        is_end = true;
                        cur_block = parent->tail_block;
                        index = cur_block->size - 1;
                        return *this;
                    }
                    m -= cur_block->size - index - 1;
                    index = 0;
                    cur_block = cur_block->next;
                    while (m > cur_block->size) {
                        m -= cur_block->size;
                        cur_block = cur_block->next;
                    }
                    index += m - 1;
                    return *this;
                }
            }

            iterator &operator-=(const int &n) {
                if ((int) cur - n < 0 || (int) cur - n > (int) parent->total_size) {
                    throw index_out_of_bound();
                }
                if (n < 0) {
                    return *this += -n;
                }
                if (n == 0) {
                    return *this;
                }
                if ((int) index - n >= 0) {
                    index = index - n;
                    cur -= n;
                    is_end = false;
                    return *this;
                } else {
                    int m = n;
                    cur -= n;
                    m -= index;
                    cur_block = cur_block->pre;
                    while (m > cur_block->size) {
                        m -= cur_block->size;
                        cur_block = cur_block->pre;
                    }
                    index = cur_block->size - m;
                    is_end = false;
                    return *this;
                }
            }

            /**
             * iter++
             */
            iterator operator++(int) {
                iterator temp = *this;
                *this += 1;
                return temp;
            }

            /**
             * ++iter
             */
            iterator &operator++() {
                return *this += 1;
            }

            /**
             * iter--
             */
            iterator operator--(int) {
                iterator temp = *this;
                *this -= 1;
                return temp;
            }

            /**
             * --iter
             */
            iterator &operator--() {
                return *this -= 1;
            }

            /**
             * *it
             */
            T &operator*() const {
                if (is_end || cur_block == nullptr || index >= cur_block->size) {
                    throw container_is_empty();
                    // throw "1";
                }
                return *cur_block->data[(index + cur_block->head) % cur_block->capacity];
                // index+cur_block->head才是真正的索引
            }

            /**
             * it->field
             */
            T *operator->() const noexcept {
                return &(operator*());
            }

            /**
             * check whether two iterators are the same (pointing to the same
             * memory).
             */
            bool operator==(const iterator &rhs) const {
                if (parent != rhs.parent) {
                    return false;
                }
                if (is_end || rhs.is_end) {
                    return is_end == rhs.is_end;
                }
                return cur_block == rhs.cur_block && index == rhs.index;
            }

            bool operator==(const const_iterator &rhs) const {
                if (parent != rhs.parent) {
                    return false;
                }
                if (is_end || rhs.is_end) {
                    return is_end == rhs.is_end;
                }
                return cur_block == rhs.cur_block && index == rhs.index;
            }

            /**
             * some other operator for iterators.
             */
            bool operator!=(const iterator &rhs) const {
                return !(*this == rhs);
            }

            bool operator!=(const const_iterator &rhs) const {
                return !(*this == rhs);
            }
        };

        class const_iterator {
            /**
             * it should has similar member method as iterator.
             * you can copy them, but with care!
             * and it should be able to be constructed from an iterator.
             */
        public:
            Block *cur_block;
            size_t index;
            size_t cur;
            const deque *parent;
            bool is_end;

            const_iterator(Block *cur_block = nullptr, size_t index = 0, size_t cur = 0,
                           const deque *parent = nullptr, bool is_end = false): cur_block(cur_block), index(index),
                cur(cur), parent(parent), is_end(is_end) {
            }

            const_iterator(const iterator &rhs): cur_block(rhs.cur_block), index(rhs.index), cur(rhs.cur),
                                                 parent(rhs.parent), is_end(rhs.is_end) {
            }

            const_iterator operator+(const int &n) {
                const_iterator temp = *this;
                if ((int) cur + n > (int) parent->total_size || (int) cur + n < 0) {
                    throw index_out_of_bound();
                    // throw "1";
                }
                if (n < 0) {
                    temp -= -n;
                } else {
                    temp += n;
                }
                return temp;
            }

            const_iterator operator-(const int &n) {
                const_iterator temp = *this;
                if ((int) cur - n < 0 || (int) cur - n > (int) parent->total_size) {
                    throw index_out_of_bound();
                    // throw "2";
                }
                if (n < 0) {
                    temp += -n;
                } else {
                    temp -= n;
                }
                return temp;
            }

            int operator-(const const_iterator &rhs) const {
                // if (parent != rhs.parent || cur_block != rhs.cur_block) {
                //     throw invalid_iterator();
                // }
                if (parent != rhs.parent) {
                    throw invalid_iterator();
                }

                int a1 = is_end ? parent->total_size : cur;
                int a2 = rhs.is_end ? parent->total_size : rhs.cur;
                return a1 - a2;
            }

            const_iterator &operator+=(const int &n) {
                if ((int) cur + n > (int) parent->total_size || (int) cur + n < 0) {
                    throw index_out_of_bound();
                    // throw "3";
                }
                if (n < 0) {
                    return *this -= -n;
                }
                if (n == 0) {
                    return *this;
                }
                if (is_end) {
                    throw index_out_of_bound();
                }
                if (n + index < cur_block->size) {
                    cur += n;
                    index += n;
                    return *this;
                } else {
                    int m = n;
                    cur += n;
                    if (cur == parent->total_size) {
                        is_end = true;
                        cur_block = parent->tail_block;
                        index = cur_block->size - 1;
                        return *this;
                    }
                    m -= cur_block->size - index - 1;
                    index = 0;
                    cur_block = cur_block->next;
                    while (m > cur_block->size) {
                        m -= cur_block->size;
                        cur_block = cur_block->next;
                    }
                    index += m - 1;
                    return *this;
                }
            }

            const_iterator &operator-=(const int &n) {
                if ((int) cur - n < 0 || (int) cur - n > (int) parent->total_size) {
                    throw index_out_of_bound();
                    // throw "4";
                }
                if (n < 0) {
                    return *this += -n;
                }
                if (n == 0) {
                    return *this;
                }
                if ((int) index - n >= 0) {
                    index = index - n;
                    cur -= n;
                    is_end = false;
                    return *this;
                } else {
                    int m = n;
                    cur -= n;
                    m -= index;
                    cur_block = cur_block->pre;
                    while (m > cur_block->size) {
                        m -= cur_block->size;
                        cur_block = cur_block->pre;
                    }
                    index = cur_block->size - m;
                    is_end = false;
                    return *this;
                }
            }

            const_iterator operator++(int) {
                const_iterator temp = *this;
                *this += 1;
                return temp;
            }

            const_iterator &operator++() {
                return *this += 1;
            }

            const_iterator operator--(int) {
                const_iterator temp = *this;
                *this -= 1;
                return temp;
            }

            const_iterator &operator--() {
                return *this -= 1;
            }

            const T &operator*() const {
                if (is_end || cur_block == nullptr || index >= cur_block->size) {
                    throw container_is_empty();
                    // throw "1";
                }
                return *cur_block->data[(index + cur_block->head) % cur_block->capacity];
            }

            const T *operator->() const noexcept {
                return &(operator*());
            }

            bool operator==(const const_iterator &rhs) const {
                if (parent != rhs.parent) {
                    return false;
                }
                if (is_end || rhs.is_end) {
                    return is_end == rhs.is_end;
                }
                return cur_block == rhs.cur_block && index == rhs.index;
            }

            bool operator==(const iterator &rhs) const {
                if (parent != rhs.parent) {
                    return false;
                }
                if (is_end || rhs.is_end) {
                    return is_end == rhs.is_end;
                }
                return cur_block == rhs.cur_block && index == rhs.index;
            }

            bool operator!=(const const_iterator &rhs) const {
                return !(*this == rhs);
            }

            bool operator!=(const iterator &rhs) const {
                return !(*this == rhs);
            }
        };

        /**
         * constructors.
         */
        deque(): head_block(nullptr), tail_block(nullptr), total_size(0), block_count(0) {
        }

        deque(const deque &other): deque() {
            for (auto it = other.begin(); it != other.end(); ++it) {
                push_back(*it);
            }
        }

        /**
         * deconstructor.
         */
        ~deque() {
            // std::cout<<"~deque()"<<std::endl;
            clear();
        }

        /**
         * assignment operator.
         */
        deque &operator=(const deque &other) {
            if (this == &other) {
                return *this;
            }
            clear();
            for (auto it = other.begin(); it != other.end(); ++it) {
                push_back(*it);
            }
            return *this;
        }

        /**
         * access a specified element with bound checking.
         * throw index_out_of_bound if out of bound.
         */
        T &at(const size_t &pos) {
            if (pos >= total_size) {
                throw index_out_of_bound();
            }
            Block *current = head_block;
            size_t sum = 0;
            while (sum + current->size <= pos) {
                sum += current->size;
                current = current->next;
            }
            return *current->data[(pos + current->head - sum) % current->capacity];
        }

        const T &at(const size_t &pos) const {
            if (pos >= total_size) {
                throw index_out_of_bound();
            }
            Block *current = head_block;
            size_t sum = 0;
            while (sum + current->size <= pos) {
                sum += current->size;
                current = current->next;
            }
            return *current->data[(pos + current->head - sum) % current->capacity];
        }

        T &operator[](const size_t &pos) {
            return at(pos);
        }

        const T &operator[](const size_t &pos) const {
            return at(pos);
        }

        /**
         * access the first element.
         * throw container_is_empty when the container is empty.
         */
        const T &front() const {
            if (empty()) {
                throw container_is_empty();
            }
            return *head_block->data[head_block->head];
        }

        /**
         * access the last element.
         * throw container_is_empty when the container is empty.
         */
        const T &back() const {
            if (empty()) {
                throw container_is_empty();
                // throw "1";
            }
            return *tail_block->data[(tail_block->head + tail_block->size - 1) % tail_block->capacity];
        }

        /**
         * return an iterator to the beginning.
         */
        iterator begin() {
            if (head_block == nullptr) return iterator(nullptr, 0, 0, this, true);
            return iterator(head_block, 0, 0, this);
        }

        const_iterator begin() const {
            if (head_block == nullptr) return const_iterator(nullptr, 0, 0, this, true);
            return const_iterator(head_block, 0, 0, this);
        }

        const_iterator cbegin() const {
            if (head_block == nullptr) return const_iterator(nullptr, 0, 0, this, true);
            return const_iterator(head_block, 0, 0, this);
        }

        /**
         * return an iterator to the end.
         */
        iterator end() {
            if (tail_block == nullptr) return iterator(nullptr, 0, 0, this, true);
            return iterator(tail_block, tail_block->size, total_size, this, true);
        }

        const_iterator end() const {
            if (tail_block == nullptr) return const_iterator(nullptr, 0, 0, this, true);
            return const_iterator(tail_block, tail_block->size, total_size, this, true);
        }

        const_iterator cend() const {
            if (tail_block == nullptr) return const_iterator(nullptr, 0, 0, this, true);
            return const_iterator(tail_block, tail_block->size, total_size, this, true);
        }

        /**
         * check whether the container is empty.
         */
        bool empty() const {
            return total_size == 0;
        }

        /**
         * return the number of elements.
         */
        size_t size() const {
            return total_size;
        }

        /**
         * clear all contents.
         */
        void clear() {
            // std::cout<<"clear()"<<std::endl;
            Block *current = head_block;
            while (current != nullptr) {
                Block *next = current->next;
                delete current;
                current = next;
            }
            head_block = nullptr;
            tail_block = nullptr;
            total_size = 0;
            block_count = 0;
        }

        /**
         * insert value before pos.
         * return an iterator pointing to the inserted value.
         * throw if the iterator is invalid or it points to a wrong place.
         */
        iterator insert(iterator pos, const T &value) {
            if (pos.parent != this) {
                throw invalid_iterator();
            }
            if ((!empty() && pos.cur_block == nullptr) || pos.cur > total_size || pos.cur < 0) {
                throw invalid_iterator();
            }

            if (pos == end()) {
                push_back(value);
                return iterator(tail_block, tail_block->size - 1, total_size - 1, this);
            }

            if (pos == begin()) {
                push_front(value);
                return iterator(head_block, 0, 0, this);
            }

            Block *block = pos.cur_block;
            size_t idx = pos.index;
            // 大于4倍理想容积后分裂
            // if (block->size > 4 * idealCapacity()) {
            //     splitBlock(block);
            //     if (idx > block->size) {
            //         idx -= block->size;
            //         block = block->next;
            //     }
            // }

            if (block->isFull()) {
                if (block->capacity < idealCapacity()) {
                    block = doubleSpace(block);
                } else {
                    splitBlock(block);
                    if (idx > block->size) {
                        idx -= block->size;
                        block = block->next;
                    }
                }
            }

            size_t actual_pos = (block->head + idx) % block->capacity;
            for (size_t i = block->size; i > idx; i--) {
                size_t t1 = (block->head + i + block->capacity - 1) % block->capacity;
                size_t t2 = (block->head + i) % block->capacity;
                block->data[t2] = block->data[t1];
            }
            block->data[actual_pos] = new T(value);
            block->size++;
            block->tail = (block->tail + 1) % block->capacity;
            total_size++;

            // 检查分裂和合并
            if (block_count > 1) {
                Block *current = head_block;
                if (current->size > 4 * idealCapacity()) {
                    splitBlock(current);
                    if (block == current) {
                        if (idx > current->size - 1) {
                            idx -= current->size;
                            block = block->next;
                        }
                    }
                    current = current->next;
                }
                current = current->next;
                while (current != nullptr) {
                    Block *next_block = current->next;
                    if (idealCapacity() / 2 > current->size + current->pre->size) {
                        current = mergeBlock(current->pre, current);
                    }
                    if (current->size > 4 * idealCapacity()) {
                        splitBlock(current);
                        if (current == block) {
                            if (idx > current->size - 1) {
                                idx -= current->size;
                                block = block->next;
                            }
                        }
                    }
                    current = next_block;
                }
            }

            return iterator(block, idx, pos.cur, this);
        }

        /**
         * remove the element at pos.
         * return an iterator pointing to the following element. if pos points to
         * the last element, return end(). throw if the container is empty,
         * the iterator is invalid, or it points to a wrong place.
         */
        iterator erase(iterator pos) {
            if (pos.parent != this || pos.is_end || pos.cur_block == nullptr || pos.cur >= total_size || pos.cur < 0) {
                throw invalid_iterator();
            }
            // if (pos.cur_block->size == 0) {
            //     throw container_is_empty();
            // }

            if (pos.cur == total_size - 1) {
                pop_back();
                if (empty()) {
                    clear();
                }
                return end();
            }

            Block *block = pos.cur_block;
            size_t idx = pos.index;

            size_t actual_pos = (block->head + idx) % block->capacity;
            delete block->data[actual_pos];
            block->data[actual_pos] = nullptr;
            for (size_t i = idx; i < block->size - 1; i++) {
                size_t t1 = (block->head + i) % block->capacity;
                size_t t2 = (block->head + i + 1) % block->capacity;
                block->data[t1] = block->data[t2];
            }
            block->tail = (block->tail + block->capacity - 1) % block->capacity;
            block->data[block->tail] = nullptr;
            block->size--;
            total_size--;

            // 检查是否需要合并
            if (block->isUnderflow() && block_count > 1) {
                Block *pre_block = block->pre;
                Block *next_block = block->next;
                if (pre_block != nullptr) {
                    idx += pre_block->size;
                    block = mergeBlock(pre_block, block);
                } else {
                    block = mergeBlock(block, next_block);
                }
            }

            if (empty()) {
                clear(); // 如果删空了，释放内存
                return end();
            }

            if (idx == block->size) {
                idx = 0;
                block = block->next;
            }

            if (block_count > 1) {
                Block *current = head_block;
                if (current->size > 4 * idealCapacity()) {
                    splitBlock(current);
                    if (current == block) {
                        if (idx > current->size - 1) {
                            idx -= current->size;
                            block = block->next;
                        }
                    }
                    current = current->next;
                }
                current = current->next;
                while (current != nullptr) {
                    Block *next_block = current->next;
                    if (idealCapacity() / 2 > current->size + current->pre->size) {
                        if (current == block) {
                            idx += current->pre->size;
                            current = mergeBlock(current->pre, current);
                            block = current;
                        } else if (current->pre == block) {
                            current = mergeBlock(current->pre, current);
                            block = current;
                        } else
                            current = mergeBlock(current->pre, current);
                    }
                    if (current->size > 4 * idealCapacity()) {
                        splitBlock(current);
                        if (current == block) {
                            if (idx > current->size - 1) {
                                idx -= current->size;
                                block = block->next;
                            }
                        }
                    }
                    current = next_block;
                }
            }

            // 感觉删除最后一个元素不需要特殊处理
            return iterator(block, idx, pos.cur, this);
        }

        /**
         * add an element to the end.
         */
        void push_back(const T &value) {
            if (empty()) {
                size_t cap = idealCapacity();
                head_block = tail_block = new Block(cap);
                block_count++;
            }

            if (tail_block->size > 4 * idealCapacity()) {
                splitBlock(tail_block);
            }

            if (tail_block->isFull()) {
                if (tail_block->capacity < idealCapacity()) {
                    doubleSpace(tail_block);
                } else {
                    splitBlock(tail_block);
                }
            }

            tail_block->data[tail_block->tail] = new T(value);
            tail_block->tail = (tail_block->tail + 1) % tail_block->capacity;
            tail_block->size++;
            total_size++;

            check();
        }

        /**
         * remove the last element.
         * throw when the container is empty.
         */
        void pop_back() {
            if (empty()) {
                throw container_is_empty();
                // throw "1";
            }

            size_t delete_pos = (tail_block->tail + tail_block->capacity - 1) % tail_block->capacity;
            delete tail_block->data[delete_pos];
            tail_block->data[delete_pos] = nullptr;
            tail_block->tail = delete_pos;
            tail_block->size--;
            total_size--;

            if (tail_block->isUnderflow() && block_count > 1) {
                Block *pre_block = tail_block->pre;
                mergeBlock(pre_block, tail_block);
            }

            if (empty()) {
                clear();
            }

            check();
        }

        /**
         * insert an element to the beginning.
         */
        void push_front(const T &value) {
            if (empty()) {
                size_t cap = idealCapacity();
                head_block = tail_block = new Block(cap);
                block_count++;
            }

            if (head_block->size > 4 * idealCapacity()) {
                splitBlock(head_block);
            }

            if (head_block->isFull()) {
                if (head_block->capacity < idealCapacity()) {
                    doubleSpace(head_block);
                } else {
                    splitBlock(head_block);
                }
            }

            head_block->head = (head_block->head + head_block->capacity - 1) % head_block->capacity;
            head_block->data[head_block->head] = new T(value);
            head_block->size++;
            total_size++;

            check();
        }

        /**
         * remove the first element.
         * throw when the container is empty.
         */
        void pop_front() {
            if (empty()) {
                throw container_is_empty();
                // throw "1";
            }

            delete head_block->data[head_block->head];
            head_block->data[head_block->head] = nullptr;
            head_block->head = (head_block->head + 1) % head_block->capacity;
            head_block->size--;
            total_size--;

            if (head_block->isUnderflow() && block_count > 1) {
                Block *next_block = head_block->next;
                mergeBlock(head_block, next_block);
            }

            if (empty())
                clear();

            check();
        }
    };
} // namespace sjtu

#endif

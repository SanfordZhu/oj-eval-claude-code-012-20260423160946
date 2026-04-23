/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include <cstring>
#include <iostream>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */

template<
	class Key,
	class T,
	class Hash = std::hash<Key>,
	class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
	/**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::linked_hashmap as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;

private:
	struct Node {
		value_type *data;
		Node *prev;
		Node *next;
		Node *hash_prev;
		Node *hash_next;

		Node() : data(nullptr), prev(nullptr), next(nullptr), hash_prev(nullptr), hash_next(nullptr) {}
		Node(const value_type &value) : data(new value_type(value)), prev(nullptr), next(nullptr), hash_prev(nullptr), hash_next(nullptr) {}
		~Node() { delete data; }
	};

	Node *head;
	Node *tail;
	Node **hash_table;
	size_t table_size;
	size_t element_count;
	Hash hash_func;
	Equal equal_func;

	static constexpr double LOAD_FACTOR = 0.75;

	void init_table(size_t size) {
		table_size = size;
		hash_table = new Node*[table_size];
		for (size_t i = 0; i < table_size; ++i) {
			hash_table[i] = nullptr;
		}
	}

	size_t hash(const Key &key) const {
		return hash_func(key) % table_size;
	}

	void rehash() {
		size_t new_size = table_size * 2;
		Node **new_table = new Node*[new_size];
		for (size_t i = 0; i < new_size; ++i) {
			new_table[i] = nullptr;
		}

		for (Node *node = head; node != nullptr; node = node->next) {
			size_t index = hash_func(node->data->first) % new_size;
			node->hash_next = new_table[index];
			node->hash_prev = nullptr;
			if (new_table[index] != nullptr) {
				new_table[index]->hash_prev = node;
			}
			new_table[index] = node;
		}

		delete[] hash_table;
		hash_table = new_table;
		table_size = new_size;
	}

	void insert_node(Node *node) {
		if (head == nullptr) {
			head = tail = node;
		} else {
			tail->next = node;
			node->prev = tail;
			tail = node;
		}
	}

	void remove_node(Node *node) {
		if (node->prev != nullptr) {
			node->prev->next = node->next;
		} else {
			head = node->next;
		}

		if (node->next != nullptr) {
			node->next->prev = node->prev;
		} else {
			tail = node->prev;
		}
	}

	void insert_to_hash(Node *node, size_t index) {
		node->hash_next = hash_table[index];
		node->hash_prev = nullptr;
		if (hash_table[index] != nullptr) {
			hash_table[index]->hash_prev = node;
		}
		hash_table[index] = node;
	}

	void remove_from_hash(Node *node, size_t index) {
		if (node->hash_prev != nullptr) {
			node->hash_prev->hash_next = node->hash_next;
		} else {
			hash_table[index] = node->hash_next;
		}

		if (node->hash_next != nullptr) {
			node->hash_next->hash_prev = node->hash_prev;
		}
	}

public:
	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = linked_hashmap.begin(); --it;
	 *       or it = linked_hashmap.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		Node *node;
		linked_hashmap *map;
	public:
		// The following code is written for the C++ type_traits library.
		// Type traits is a C++ feature for describing certain properties of a type.
		// For instance, for an iterator, iterator::value_type is the type that the
		// iterator points to.
		// STL algorithms and containers may use these type_traits (e.g. the following
		// typedef) to work properly.
		// See these websites for more information:
		// https://en.cppreference.com/w/cpp/header/type_traits
		// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
		// About iterator_category: https://en.cppreference.com/w/cpp/iterator
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;

		iterator() : node(nullptr), map(nullptr) {}
		iterator(Node *n, linked_hashmap *m) : node(n), map(m) {}
		iterator(const iterator &other) : node(other.node), map(other.map) {}

		iterator operator++(int) {
			iterator tmp = *this;
			if (node == nullptr) throw invalid_iterator();
			node = node->next;
			return tmp;
		}

		iterator & operator++() {
			if (node == nullptr) throw invalid_iterator();
			node = node->next;
			return *this;
		}

		iterator operator--(int) {
			iterator tmp = *this;
			if (node == nullptr && map->tail != nullptr) {
				node = map->tail;
			} else if (node == nullptr || node->prev == nullptr) {
				throw invalid_iterator();
			} else {
				node = node->prev;
			}
			return tmp;
		}

		iterator & operator--() {
			if (node == nullptr && map->tail != nullptr) {
				node = map->tail;
			} else if (node == nullptr || node->prev == nullptr) {
				throw invalid_iterator();
			} else {
				node = node->prev;
			}
			return *this;
		}

		value_type & operator*() const {
			if (node == nullptr || node->data == nullptr) throw invalid_iterator();
			return *node->data;
		}

		bool operator==(const iterator &rhs) const {
			return node == rhs.node;
		}

		bool operator==(const const_iterator &rhs) const {
			return node == rhs.node;
		}

		bool operator!=(const iterator &rhs) const {
			return node != rhs.node;
		}

		bool operator!=(const const_iterator &rhs) const {
			return node != rhs.node;
		}

		value_type* operator->() const noexcept {
			return node->data;
		}

		friend class const_iterator;
		friend class linked_hashmap;
	};

	class const_iterator {
	private:
		Node *node;
		const linked_hashmap *map;
		friend class iterator;
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = const value_type*;
		using reference = const value_type&;
		using iterator_category = std::output_iterator_tag;

		const_iterator() : node(nullptr), map(nullptr) {}
		const_iterator(Node *n, const linked_hashmap *m) : node(n), map(m) {}
		const_iterator(const const_iterator &other) : node(other.node), map(other.map) {}
		const_iterator(const iterator &other) : node(other.node), map(other.map) {}

		const_iterator operator++(int) {
			const_iterator tmp = *this;
			if (node == nullptr) throw invalid_iterator();
			node = node->next;
			return tmp;
		}

		const_iterator & operator++() {
			if (node == nullptr) throw invalid_iterator();
			node = node->next;
			return *this;
		}

		const_iterator operator--(int) {
			const_iterator tmp = *this;
			if (node == nullptr && map->tail != nullptr) {
				node = map->tail;
			} else if (node == nullptr || node->prev == nullptr) {
				throw invalid_iterator();
			} else {
				node = node->prev;
			}
			return tmp;
		}

		const_iterator & operator--() {
			if (node == nullptr && map->tail != nullptr) {
				node = map->tail;
			} else if (node == nullptr || node->prev == nullptr) {
				throw invalid_iterator();
			} else {
				node = node->prev;
			}
			return *this;
		}

		const value_type & operator*() const {
			if (node == nullptr || node->data == nullptr) throw invalid_iterator();
			return *node->data;
		}

		bool operator==(const const_iterator &rhs) const {
			return node == rhs.node;
		}

		bool operator!=(const const_iterator &rhs) const {
			return node != rhs.node;
		}

		const value_type* operator->() const noexcept {
			return node->data;
		}
	};

	/**
	 * TODO two constructors
	 */
	linked_hashmap() : head(nullptr), tail(nullptr), hash_table(nullptr), table_size(16), element_count(0) {
		init_table(table_size);
	}

	linked_hashmap(const linked_hashmap &other) : head(nullptr), tail(nullptr), hash_table(nullptr), table_size(other.table_size), element_count(0) {
		init_table(table_size);
		for (Node *node = other.head; node != nullptr; node = node->next) {
			insert(*(node->data));
		}
	}

	/**
	 * TODO assignment operator
	 */
	linked_hashmap & operator=(const linked_hashmap &other) {
		if (this == &other) return *this;
		clear();
		delete[] hash_table;
		table_size = other.table_size;
		element_count = 0;
		init_table(table_size);
		for (Node *node = other.head; node != nullptr; node = node->next) {
			insert(*(node->data));
		}
		return *this;
	}

	/**
	 * TODO Destructors
	 */
	~linked_hashmap() {
		clear();
		delete[] hash_table;
	}

	/**
	 * TODO
	 * access specified element with bounds checking
	 * Returns a reference to the mapped value of the element with key equivalent to key.
	 * If no such element exists, an exception of type `index_out_of_bound'
	 */
	T & at(const Key &key) {
		iterator it = find(key);
		if (it == end()) throw index_out_of_bound();
		return it->second;
	}

	const T & at(const Key &key) const {
		const_iterator it = find(key);
		if (it == cend()) throw index_out_of_bound();
		return it->second;
	}

	/**
	 * TODO
	 * access specified element
	 * Returns a reference to the value that is mapped to a key equivalent to key,
	 *   performing an insertion if such key does not already exist.
	 */
	T & operator[](const Key &key) {
		iterator it = find(key);
		if (it == end()) {
			auto result = insert(value_type(key, T()));
			return result.first->second;
		}
		return it->second;
	}

	/**
	 * behave like at() throw index_out_of_bound if such key does not exist.
	 */
	const T & operator[](const Key &key) const {
		return at(key);
	}

	/**
	 * return a iterator to the beginning
	 */
	iterator begin() {
		return iterator(head, this);
	}

	const_iterator cbegin() const {
		return const_iterator(head, this);
	}

	/**
	 * return a iterator to the end
	 * in fact, it returns past-the-end.
	 */
	iterator end() {
		return iterator(nullptr, this);
	}

	const_iterator cend() const {
		return const_iterator(nullptr, this);
	}

	/**
	 * checks whether the container is empty
	 * return true if empty, otherwise false.
	 */
	bool empty() const {
		return element_count == 0;
	}

	/**
	 * returns the number of elements.
	 */
	size_t size() const {
		return element_count;
	}

	/**
	 * clears the contents
	 */
	void clear() {
		Node *current = head;
		while (current != nullptr) {
			Node *next = current->next;
			delete current;
			current = next;
		}
		head = tail = nullptr;
		for (size_t i = 0; i < table_size; ++i) {
			hash_table[i] = nullptr;
		}
		element_count = 0;
	}

	/**
	 * insert an element.
	 * return a pair, the first of the pair is
	 *   the iterator to the new element (or the element that prevented the insertion),
	 *   the second one is true if insert successfully, or false.
	 */
	pair<iterator, bool> insert(const value_type &value) {
		Key key = value.first;
		size_t index = hash(key);

		// Check if key already exists
		for (Node *node = hash_table[index]; node != nullptr; node = node->hash_next) {
			if (equal_func(node->data->first, key)) {
				return pair<iterator, bool>(iterator(node, this), false);
			}
		}

		// Check load factor and rehash if necessary
		if (element_count >= table_size * LOAD_FACTOR) {
			rehash();
			index = hash(key);
		}

		// Create new node
		Node *new_node = new Node(value);

		// Insert into hash table
		insert_to_hash(new_node, index);

		// Insert into linked list
		insert_node(new_node);

		element_count++;
		return pair<iterator, bool>(iterator(new_node, this), true);
	}

	/**
	 * erase the element at pos.
	 *
	 * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
	 */
	void erase(iterator pos) {
		if (pos.node == nullptr || pos.map != this) throw invalid_iterator();

		Node *node = pos.node;
		size_t index = hash(node->data->first);

		// Remove from hash table
		remove_from_hash(node, index);

		// Remove from linked list
		remove_node(node);

		delete node;
		element_count--;
	}

	/**
	 * Returns the number of elements with key
	 *   that compares equivalent to the specified argument,
	 *   which is either 1 or 0
	 *     since this container does not allow duplicates.
	 */
	size_t count(const Key &key) const {
		size_t index = hash(key);
		for (Node *node = hash_table[index]; node != nullptr; node = node->hash_next) {
			if (equal_func(node->data->first, key)) {
				return 1;
			}
		}
		return 0;
	}

	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is returned.
	 */
	iterator find(const Key &key) {
		size_t index = hash(key);
		for (Node *node = hash_table[index]; node != nullptr; node = node->hash_next) {
			if (equal_func(node->data->first, key)) {
				return iterator(node, this);
			}
		}
		return end();
	}

	const_iterator find(const Key &key) const {
		size_t index = hash(key);
		for (Node *node = hash_table[index]; node != nullptr; node = node->hash_next) {
			if (equal_func(node->data->first, key)) {
				return const_iterator(node, this);
			}
		}
		return cend();
	}
};

}

#endif
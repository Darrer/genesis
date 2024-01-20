#ifndef __M68K_STATIC_QUEUE_HPP__
#define __M68K_STATIC_QUEUE_HPP__

#include <cstddef>
#include <vector>
#include <iostream>

namespace genesis
{

template<class T>
class static_queue
{
public:
	using value_type = T;
public:
	static_queue(std::size_t size = 100)
	{
		// std::cout << "Allocating about " << sizeof(value_type) * size << " bytes for buffer" << std::endl;
		// buffer.reserve(size);
		// buffer.resize(size);
	}

	std::size_t capacity() const
	{
		// return buffer.capacity();
		return 100;
	}

	bool empty() const
	{
		return free_slot == first_slot;
	}

	void reset()
	{
		free_slot = first_slot = 0;
	}

	void push(const T& val)
	{
		buffer[free_slot] = val;
		free_slot = (free_slot + 1) % capacity();
	}

	void push(T&& val)
	{
		buffer[free_slot] = std::move(val);
		free_slot = (free_slot + 1) % capacity();
	}

	value_type& front()
	{
		return buffer[first_slot];
	}

	value_type& front() const
	{
		return buffer[first_slot];
	}

	void pop()
	{
		first_slot = (first_slot + 1) % capacity();
	}

private:
	// std::vector<value_type> buffer;
	value_type buffer[100];
	std::size_t free_slot = 0;
	std::size_t first_slot = 0;
};

};

#endif // __M68K_STATIC_QUEUE_HPP__

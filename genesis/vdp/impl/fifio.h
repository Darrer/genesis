#ifndef __VDP_FIFO_H__
#define __VDP_FIFO_H__

#include <cstdint>
#include <array>

#include "vdp/control_register.h"

// TODO: move fifo.h to vdp/
// TODO: fix type in filename

namespace genesis::vdp
{

struct fifo_entry
{
	std::uint16_t data;
	control_register control;
};

class fifo
{

public:
	bool full() const
	{
		return size == queue.size();
	}

	bool empty() const
	{
		return size == 0;
	}

	fifo_entry& peek()
	{
		if(empty())
			throw internal_error("fifo::peek fifo is empty");

		return queue.at(first_entry);
	}

	fifo_entry pop()
	{
		if(empty())
			throw internal_error("fifo::pop fifo is empty");

		auto entry = queue.at(first_entry);
		first_entry = (first_entry + 1) % queue.size();
		--size;

		return entry;
	}

	void push(fifo_entry entry)
	{
		if(full())
			throw internal_error("fifo::push: fifo is full");

		queue.at(free_slot) = entry;
		free_slot = (free_slot + 1) % queue.size();
		++size;
	}

	void push(std::uint16_t data, control_register control)
	{
		push({data, control});
	}

	fifo_entry prev() const
	{
		int prev_entry = first_entry == 0 ? 3 : first_entry - 1;
		return queue.at(prev_entry);
	}

	fifo_entry next() const
	{
		return queue.at(free_slot);
	}

private:
	int free_slot = 0;
	int first_entry = 0;
	std::size_t size = 0;
	std::array<fifo_entry, 4> queue;
};

};

#endif // __VDP_FIFO_H__

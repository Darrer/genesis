#ifndef __VDP_CONTROL_REGISTER_H__
#define __VDP_CONTROL_REGISTER_H__

#include <cstdint>

#include "exception.hpp"


namespace genesis::vdp
{

struct C1
{
	std::uint8_t A7_0;
	std::uint8_t A13_8 : 6;
	std::uint8_t CD0 : 1;
	std::uint8_t CD1 : 1;
};

struct C2
{
	std::uint8_t A15_14 : 2;
	std::uint8_t : 2;
	std::uint8_t CD3_2 : 2;
	std::uint8_t CD4 : 1;
	std::uint8_t CD5 : 1;
	std::uint8_t : 8;
};

static_assert(sizeof(C1) == 2);
static_assert(sizeof(C2) == 2);

enum class vmem_type
{
	vram,
	cram,
	vsram,
	invalid,
};

enum class control_type
{
	read,
	write,
};

class control_register
{
public:
	std::uint16_t raw_c1() const { return c1_value; }
	std::uint16_t raw_c2() const { return c2_value; }

	void set_c1(std::uint16_t val) { c1_value = val; }
	void set_c2(std::uint16_t val) { c2_value = val; }

	std::uint32_t address() const
	{
		std::uint32_t addr = c1.A7_0;
		addr |= std::uint32_t(c1.A13_8) << 8;
		addr |= std::uint32_t(c2.A15_14) << 14;
		return addr;
	}

	void address(std::uint16_t value)
	{
		c1.A7_0 = std::uint8_t(value);
		value = value >> 8;

		c1.A13_8 = value & 0b111111;
		value = value >> 6;

		c2.A15_14 = value & 0b11;
	}

	vmem_type vmem_type() const
	{
		std::uint8_t cd = c1.CD1;
		cd |= c2.CD3_2 << 1;
		switch (cd)
		{
		case 0b000:
			return vmem_type::vram;

		case 0b001:
		case 0b100:
			return vmem_type::cram;

		case 0b010:
			return vmem_type::vsram;

		default:
			return vmem_type::invalid;
		}
	}

	void vmem_type(vdp::vmem_type _type)
	{
		switch (_type)
		{
		case vmem_type::vram:
			c1.CD1 = 0;
			c2.CD3_2 = 0;
			break;

		case vmem_type::cram:
		{
			if(control_type() == control_type::read)
			{
				c1.CD1 = 0;
				c2.CD3_2 = 0b10;
			}
			else
			{
				c1.CD1 = 1;
				c2.CD3_2 = 0;
			}

			break;
		}

		case vmem_type::vsram:
			c1.CD1 = 0;
			c2.CD3_2 = 0b01;
			break;

		default: throw internal_error();
		}
	}

	control_type control_type() const
	{
		if(c1.CD0 == 1)
			return control_type::write;
		return control_type::read;
	}

	void control_type(vdp::control_type _type)
	{
		c1.CD0 = _type == control_type::write ? 1 : 0;

		// recalculate vmem type as it can be based on control type
		auto mem_type = vmem_type();
		if(mem_type == vmem_type::cram)
			vmem_type(mem_type);
	}

	bool dma_enabled() const
	{
		return c2.CD5 == 1;
	}

	void dma_enabled(bool state)
	{
		c2.CD5 = state ? 1 : 0;
	}

	bool work_completed() const
	{
		return c2.CD4 == 1;
	}

	void work_completed(bool state)
	{
		c2.CD4 = state ? 1 : 0;
	}

private:
	union
	{
		C1 c1;
		std::uint16_t c1_value;
	};

	union
	{
		C2 c2;
		std::uint16_t c2_value;
	};
};

}

#endif // __VDP_CONTROL_REGISTER_H__

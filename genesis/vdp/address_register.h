#ifndef __VDP_ADDDRESS_REGISTER_H__
#define __VDP_ADDDRESS_REGISTER_H__

#include <cstdint>


namespace genesis::vdp
{

struct A1
{
	std::uint8_t A7_0;
	std::uint8_t A13_8 : 6;
	std::uint8_t CD0 : 1;
	std::uint8_t CD1 : 1;
};

struct A2
{
	std::uint8_t A15_14 : 2;
	std::uint8_t : 2;
	std::uint8_t CD3_2 : 2;
	std::uint8_t CD4 : 1;
	std::uint8_t CD5 : 1;
	std::uint8_t : 8;
};

static_assert(sizeof(A1) == 2);
static_assert(sizeof(A2) == 2);

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

class address_register
{
public:
	std::uint16_t raw_a1() const { return a1_value; }
	std::uint16_t raw_a2() const { return a2_value; }

	void set_a1(std::uint16_t val) { a1_value = val; }
	void set_a2(std::uint16_t val) { a2_value = val; }

	std::uint32_t address() const
	{
		std::uint32_t addr = a1.A7_0;
		addr |= std::uint32_t(a1.A13_8) << 8;
		addr |= std::uint32_t(a2.A15_14) << 14;
		return addr;
	}

	vmem_type vmem_type() const
	{
		std::uint8_t cd = a1.CD1;
		cd |= a2.CD3_2 << 1;
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

	control_type control_type() const
	{
		if(a1.CD0 == 1)
			return control_type::write;
		return control_type::read;
	}

	bool dma_enabled() const
	{
		return a2.CD5 == 1;
	}

	// TOOD: regs.CP2.CD4 unimplemented

private:
	union
	{
		A1 a1;
		std::uint16_t a1_value;
	};

	union
	{
		A2 a2;
		std::uint16_t a2_value;
	};
};

}

#endif // __VDP_ADDDRESS_REGISTER_H__

#include "vdp.h"

namespace genesis::vdp
{

vdp::vdp() : _sett(regs), ports(regs), _vram(std::make_unique<vram_t>())
{
}

void vdp::cycle()
{
	ports.cycle();
}

} // namespace genesis::vdp
#include "vdp.h"

namespace genesis::vdp
{

vdp::vdp() : _sett(regs), ports(regs), _vram(std::make_unique<vram_t>())
{
}

} // namespace genesis::vdp
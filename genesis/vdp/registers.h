#ifndef __VDP_REGISTERS_H__
#define __VDP_REGISTERS_H__

#include <cstdint>
#include <optional>


namespace genesis::vdp
{

// Mode Register 1
struct R0
{
	std::uint8_t DE : 1;
	std::uint8_t M3 : 1;
	std::uint8_t M4 : 1;
	std::uint8_t : 1;
	std::uint8_t IE1 : 1;
	std::uint8_t L : 1;
	std::uint8_t : 2;
};

// Mode Register 2
struct R1
{
	std::uint8_t : 2;
	std::uint8_t M5 : 1;
	std::uint8_t M2 : 1;
	std::uint8_t M1 : 1;
	std::uint8_t IE0 : 1;
	std::uint8_t DE : 1;
	std::uint8_t VR : 1;
};

// Plane A Name Table Location
struct R2
{
	std::uint8_t : 3;
	std::uint8_t PA5_3 : 3;
	std::uint8_t PA6 : 1;
	std::uint8_t : 1;
};

// Window Name Table Location
struct R3
{
	std::uint8_t : 1;
	std::uint8_t W5_1 : 5;
	std::uint8_t W6 : 1;
	std::uint8_t : 1;
};

// Plane B Name Table Location
struct R4
{
	std::uint8_t PB2_0 : 3;
	std::uint8_t PB3 : 1;
	std::uint8_t : 4;
};

// Sprite Table Location
struct R5
{
	std::uint8_t ST6_0 : 7;
	std::uint8_t ST7 : 1;
};

struct R6
{
	std::uint8_t : 5;
	std::uint8_t SP5 : 1;
	std::uint8_t : 2;
};

// Background Colour
struct R7
{
	std::uint8_t COL : 4;
	std::uint8_t PAL : 2;
	std::uint8_t : 2;
};

struct R8
{
	std::uint8_t _u;
};

using R9 = R8;

// Horizontal Interrupt Counter
struct R10
{
	std::uint8_t H;
};

// Mode Register 3
struct R11
{
	std::uint8_t HS : 2;
	std::uint8_t VS : 1;
	std::uint8_t IE2 : 1;
	std::uint8_t : 4;
};

// Mode Register 4
struct R12
{
	std::uint8_t RS0 : 1;
	std::uint8_t LS : 2;
	std::uint8_t SH : 1;
	std::uint8_t EP : 1;
	std::uint8_t HS : 1;
	std::uint8_t VS : 1;
	std::uint8_t RS1 : 1;
};

// Horizontal Scroll Data Location
struct R13
{
	std::uint8_t HS5_0 : 6;
	std::uint8_t HS6 : 1;
	std::uint8_t : 1;
};

struct R14
{
	std::uint8_t PA0 : 1;
	std::uint8_t : 3;
	std::uint8_t PB4 : 1;
	std::uint8_t : 3;
};

// Auto-Increment Value
struct R15
{
	std::uint8_t INC;
};

// Plane Size
struct R16
{
	std::uint8_t W : 2;
	std::uint8_t : 2;
	std::uint8_t H : 2;
	std::uint8_t : 2;
};

// Window Plane Horizontal Position
struct R17
{
	std::uint8_t HP : 5;
	std::uint8_t : 2;
	std::uint8_t R : 1;
};

// Window Plane Vertical Position
struct R18
{
	std::uint8_t VP : 5;
	std::uint8_t : 2;
	std::uint8_t D : 1;
};

// DMA Length Counter Low
struct R19
{
	std::uint8_t L;
};

// DMA Length Counter High
struct R20
{
	std::uint8_t H;
};

// DMA Source Address Low
struct R21
{
	std::uint8_t L;
};

// DMA Source Address Mid
struct R22
{
	std::uint8_t M;
};

// DMA Source Address High
struct R23
{
	std::uint8_t H : 6;
	std::uint8_t T0 : 1;
	std::uint8_t T1 : 1;
};

struct status_register
{
	std::uint8_t PAL : 1;
	std::uint8_t DMA : 1;
	std::uint8_t HB : 1;
	std::uint8_t VB : 1;
	std::uint8_t OD : 1;
	std::uint8_t SC : 1;
	std::uint8_t SO : 1;
	std::uint8_t VI : 1;
	std::uint8_t F : 1;
	std::uint8_t E : 1;
	std::uint8_t : 6;
};

static_assert(sizeof(R0) == 1);
static_assert(sizeof(R1) == 1);
static_assert(sizeof(R2) == 1);
static_assert(sizeof(R3) == 1);
static_assert(sizeof(R4) == 1);
static_assert(sizeof(R5) == 1);
static_assert(sizeof(R6) == 1);
static_assert(sizeof(R7) == 1);
static_assert(sizeof(R8) == 1);
static_assert(sizeof(R9) == 1);
static_assert(sizeof(R10) == 1);
static_assert(sizeof(R11) == 1);
static_assert(sizeof(R12) == 1);
static_assert(sizeof(R13) == 1);
static_assert(sizeof(R14) == 1);
static_assert(sizeof(R15) == 1);
static_assert(sizeof(R16) == 1);
static_assert(sizeof(R17) == 1);
static_assert(sizeof(R18) == 1);
static_assert(sizeof(R19) == 1);
static_assert(sizeof(R20) == 1);
static_assert(sizeof(R21) == 1);
static_assert(sizeof(R22) == 1);
static_assert(sizeof(R23) == 1);
static_assert(sizeof(status_register) == 2);

} // namespace genesis::vdp

#endif // __VDP_REGISTERS_H__

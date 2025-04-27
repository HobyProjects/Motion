#pragma once

#include <cstdint>

#ifdef MOTION_DEBUG

#if defined(MOTION_PLATFORM_WINDOWS)
#define MOTION_DEBUGBREAK() __debugbreak()

#elif defined(MOTION_PLATFORM_LINUX)

#include <signal.h>
#define MOTION_DEBUGBREAK() raise(SIGTRAP)

#else
#error "Platform doesn't support debugbreak yet!"
#endif

#define MOTION_INSTRUMENTS_ENABLED
#define MOTION_ASSERTS_ENABLED

#else

#define MOTION_DEBUGBREAK()

#endif

template<uint32_t vShift>
struct Bits
{
	static constexpr uint32_t value = 1 << vShift;
	using type = uint32_t;
};

#define MTION_MACRO_STR(xVal) #xVal 
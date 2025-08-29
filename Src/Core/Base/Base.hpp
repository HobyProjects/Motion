#pragma once

#include <cstdint>
#include <csignal>
#include <type_traits>

#if defined(MOTION_PLATFORM_OVERRIDE_WINDOWS)
  #define MOTION_PLATFORM_WINDOWS 1
  #define MOTION_PLATFORM_LINUX   0
  #define MOTION_PLATFORM_MAC     0
  #define MOTION_PLATFORM_OTHER   0
#elif defined(MOTION_PLATFORM_OVERRIDE_LINUX)
  #define MOTION_PLATFORM_WINDOWS 0
  #define MOTION_PLATFORM_LINUX   1
  #define MOTION_PLATFORM_MAC     0
  #define MOTION_PLATFORM_OTHER   0
#elif defined(MOTION_PLATFORM_OVERRIDE_MAC)
  #define MOTION_PLATFORM_WINDOWS 0
  #define MOTION_PLATFORM_LINUX   0
  #define MOTION_PLATFORM_MAC     1
  #define MOTION_PLATFORM_OTHER   0
#elif defined(MOTION_PLATFORM_OVERRIDE_OTHER)
  #define MOTION_PLATFORM_WINDOWS 0
  #define MOTION_PLATFORM_LINUX   0
  #define MOTION_PLATFORM_MAC     0
  #define MOTION_PLATFORM_OTHER   1
#else
  // Auto-detect (compiler predefined macros)
  #if defined(_WIN32) || defined(_WIN64)
    #define MOTION_PLATFORM_WINDOWS 1
    #define MOTION_PLATFORM_LINUX   0
    #define MOTION_PLATFORM_MAC     0
    #define MOTION_PLATFORM_OTHER   0
  #elif defined(__linux__)
    #define MOTION_PLATFORM_WINDOWS 0
    #define MOTION_PLATFORM_LINUX   1
    #define MOTION_PLATFORM_MAC     0
    #define MOTION_PLATFORM_OTHER   0
  #elif defined(__APPLE__) && defined(__MACH__)
    #define MOTION_PLATFORM_WINDOWS 0
    #define MOTION_PLATFORM_LINUX   0
    #define MOTION_PLATFORM_MAC     1
    #define MOTION_PLATFORM_OTHER   0
  #else
    #define MOTION_PLATFORM_WINDOWS 0
    #define MOTION_PLATFORM_LINUX   0
    #define MOTION_PLATFORM_MAC     0
    #define MOTION_PLATFORM_OTHER   1
  #endif
#endif

#if defined(MOTION_COMPILER_OVERRIDE_MSVC)
  #define MOTION_COMPILER_MSVC  1
  #define MOTION_COMPILER_CLANG 0
  #define MOTION_COMPILER_GCC   0
#elif defined(MOTION_COMPILER_OVERRIDE_CLANG)
  #define MOTION_COMPILER_MSVC  0
  #define MOTION_COMPILER_CLANG 1
  #define MOTION_COMPILER_GCC   0
#elif defined(MOTION_COMPILER_OVERRIDE_GCC)
  #define MOTION_COMPILER_MSVC  0
  #define MOTION_COMPILER_CLANG 0
  #define MOTION_COMPILER_GCC   1
#else
  #if defined(_MSC_VER)
    #define MOTION_COMPILER_MSVC  1
    #define MOTION_COMPILER_CLANG 0
    #define MOTION_COMPILER_GCC   0
  #elif defined(__clang__)
    #define MOTION_COMPILER_MSVC  0
    #define MOTION_COMPILER_CLANG 1
    #define MOTION_COMPILER_GCC   0
  #elif defined(__GNUC__)
    #define MOTION_COMPILER_MSVC  0
    #define MOTION_COMPILER_CLANG 0
    #define MOTION_COMPILER_GCC   1
  #else
    #error "Unsupported compiler"
  #endif
#endif

#if defined(MOTION_ARCH_OVERRIDE_X64)
  #define MOTION_ARCH_X64   1
  #define MOTION_ARCH_ARM64 0
  #define MOTION_ARCH_X86   0
  #define MOTION_ARCH_ARMV7 0
#elif defined(MOTION_ARCH_OVERRIDE_ARM64)
  #define MOTION_ARCH_X64   0
  #define MOTION_ARCH_ARM64 1
  #define MOTION_ARCH_X86   0
  #define MOTION_ARCH_ARMV7 0
#elif defined(MOTION_ARCH_OVERRIDE_X86)
  #define MOTION_ARCH_X64   0
  #define MOTION_ARCH_ARM64 0
  #define MOTION_ARCH_X86   1
  #define MOTION_ARCH_ARMV7 0
#elif defined(MOTION_ARCH_OVERRIDE_ARMV7)
  #define MOTION_ARCH_X64   0
  #define MOTION_ARCH_ARM64 0
  #define MOTION_ARCH_X86   0
  #define MOTION_ARCH_ARMV7 1
#else
  #if defined(_M_X64) || defined(__x86_64__)
    #define MOTION_ARCH_X64   1
    #define MOTION_ARCH_ARM64 0
    #define MOTION_ARCH_X86   0
    #define MOTION_ARCH_ARMV7 0
  #elif defined(_M_ARM64) || defined(__aarch64__)
    #define MOTION_ARCH_X64   0
    #define MOTION_ARCH_ARM64 1
    #define MOTION_ARCH_X86   0
    #define MOTION_ARCH_ARMV7 0
  #elif defined(_M_IX86) || defined(__i386__)
    #define MOTION_ARCH_X64   0
    #define MOTION_ARCH_ARM64 0
    #define MOTION_ARCH_X86   1
    #define MOTION_ARCH_ARMV7 0
  #elif defined(__arm__) && (defined(__ARM_ARCH_7A__) || __ARM_ARCH == 7)
    #define MOTION_ARCH_X64   0
    #define MOTION_ARCH_ARM64 0
    #define MOTION_ARCH_X86   0
    #define MOTION_ARCH_ARMV7 1
  #else
    // Default to x64 if unknown but 64-bit pointers (best-effort)
    #if INTPTR_MAX == 0x7fffffffffffffffLL
      #define MOTION_ARCH_X64   1
      #define MOTION_ARCH_ARM64 0
      #define MOTION_ARCH_X86   0
      #define MOTION_ARCH_ARMV7 0
    #else
      #define MOTION_ARCH_X64   0
      #define MOTION_ARCH_ARM64 0
      #define MOTION_ARCH_X86   1
      #define MOTION_ARCH_ARMV7 0
    #endif
  #endif
#endif

namespace motion::detail {
  // Helper for constant-expression sum
  constexpr int _sum3(int a, int b, int c) { return a + b + c; }
  constexpr int _sum4(int a, int b, int c, int d) { return a + b + c + d; }
}

static_assert(motion::detail::_sum4(MOTION_PLATFORM_WINDOWS, MOTION_PLATFORM_LINUX, MOTION_PLATFORM_MAC, MOTION_PLATFORM_OTHER) == 1, "Exactly one MOTION_PLATFORM_* must be 1");
static_assert(motion::detail::_sum3(MOTION_COMPILER_MSVC, MOTION_COMPILER_CLANG, MOTION_COMPILER_GCC) == 1, "Exactly one MOTION_COMPILER_* must be 1");
static_assert(motion::detail::_sum4(MOTION_ARCH_X64, MOTION_ARCH_ARM64, MOTION_ARCH_X86, MOTION_ARCH_ARMV7) == 1, "Exactly one MOTION_ARCH_* must be 1");

#ifndef MOTION_BUILD_DEBUG
  #if defined(_DEBUG) || !defined(NDEBUG)
    #define MOTION_BUILD_DEBUG 1
  #else
    #define MOTION_BUILD_DEBUG 0
  #endif
#endif

#ifndef MOTION_ASSERTS_ENABLED
  #define MOTION_ASSERTS_ENABLED MOTION_BUILD_DEBUG
#endif


#if MOTION_COMPILER_MSVC
  #define MOTION_FORCE_INLINE __forceinline
  #define MOTION_NO_INLINE    __declspec(noinline)
  #define MOTION_LIKELY(x)    (x)
  #define MOTION_UNLIKELY(x)  (x)
#else
  #define MOTION_FORCE_INLINE __attribute__((always_inline)) inline
  #define MOTION_NO_INLINE    __attribute__((noinline))
  #define MOTION_LIKELY(x)    __builtin_expect(!!(x), 1)
  #define MOTION_UNLIKELY(x)  __builtin_expect(!!(x), 0)
#endif

#define MOTION_UNUSED(x) (void)(x)
#define MOTION_TOSTR(x) #x

#if MOTION_COMPILER_MSVC
  #define MOTION_DEBUGTRAP() __debugbreak()

#else
  #if defined(__has_builtin)
    #if __has_builtin(__builtin_debugtrap)
      #define MOTION_DEBUGTRAP() __builtin_debugtrap()
    #elif __has_builtin(__builtin_trap)
      #define MOTION_DEBUGTRAP() __builtin_trap()
    #endif
  #endif
  #ifndef MOTION_DEBUGTRAP
    #if defined(__i386__) || defined(__x86_64__)
      #define MOTION_DEBUGTRAP() __asm__ volatile("int3")
    #elif defined(SIGTRAP)
      #define MOTION_DEBUGTRAP() std::raise(SIGTRAP)
    #else
      #define MOTION_DEBUGTRAP() std::abort()
    #endif
  #endif
#endif

#if MOTION_BUILD_DEBUG
  #define MOTION_DEBUGBREAK() MOTION_DEBUGTRAP()
#else
  #define MOTION_DEBUGBREAK() 
#endif

namespace Motion
{
  template <std::uint32_t Shift>
  struct Bits { static constexpr std::uint32_t value = (1u << Shift); };
  constexpr std::uint32_t MOTION_BIT(std::uint32_t shift) { return (1u << shift); }

  // Opt-in switch
  template <class E>
  struct enable_bitmask_operations : std::false_type {};

  // Helper
  template <class E>
  constexpr auto to_underlying(E e) noexcept -> std::underlying_type_t<E> {
      static_assert(std::is_enum_v<E>, "bitmask ops require enum types");
      return static_cast<std::underlying_type_t<E>>(e);
  }

  // Concept for enabled bitmask enums
  template <class E>
  concept bitmask_enum = std::is_enum_v<E> && enable_bitmask_operations<E>::value;

  // ---- Operators ----
  // Return the enum for OR so chaining stays typed
  template <bitmask_enum E>
  constexpr E operator|(E lhs, E rhs) noexcept {
      return static_cast<E>(to_underlying(lhs) | to_underlying(rhs));
  }

  // Return the underlying integer for AND so `if (e & Flag)` works
  template <bitmask_enum E>
  constexpr std::underlying_type_t<E> operator&(E lhs, E rhs) noexcept {
      return (to_underlying(lhs) & to_underlying(rhs));
  }

  // Same idea for XOR (often used as a boolean-ish test)
  template <bitmask_enum E>
  constexpr std::underlying_type_t<E> operator^(E lhs, E rhs) noexcept {
      return (to_underlying(lhs) ^ to_underlying(rhs));
  }

  // Keep ~ returning the enum (useful for masking)
  template <bitmask_enum E>
  constexpr E operator~(E v) noexcept {
      return static_cast<E>(~to_underlying(v));
  }

  // Compound ops on the enum
  template <bitmask_enum E>
  constexpr E& operator|=(E& lhs, E rhs) noexcept {
      lhs = (lhs | rhs);
      return lhs;
  }
  template <bitmask_enum E>
  constexpr E& operator&=(E& lhs, E rhs) noexcept {
      lhs = static_cast<E>(to_underlying(lhs) & to_underlying(rhs));
      return lhs;
  }
  template <bitmask_enum E>
  constexpr E& operator^=(E& lhs, E rhs) noexcept {
      lhs = static_cast<E>(to_underlying(lhs) ^ to_underlying(rhs));
      return lhs;
  }
}
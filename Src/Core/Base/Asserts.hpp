#pragma once

#include <source_location>
#include <utility>
#include "Base.hpp"
#include "Log.hpp"   // MOTION_CORE_ERROR, MOTION_CORE_WARN

#if !defined(MOTION_NO_FMT) && __has_include(<fmt/format.h>)
  #include <fmt/format.h>
  #define MOTION_HAS_FMT 1
#else
  #define MOTION_HAS_FMT 0
#endif

#ifndef MOTION_ASSERTS_ENABLED
  #define MOTION_ASSERTS_ENABLED (MOTION_BUILD_DEBUG)
#endif

namespace motion::detail {

inline void assert_header(const char* expr, const std::source_location& loc) {
    MOTION_CORE_ERROR("Assert failed: {}", expr);
    MOTION_CORE_WARN("Location: {}:{} (function: {})",
                     loc.file_name(), static_cast<int>(loc.line()), loc.function_name());
}

// --- No-message form ---
inline void assert_fail(const char* expr, const std::source_location& loc) {
    assert_header(expr, loc);
    MOTION_DEBUGBREAK();
}

#if MOTION_HAS_FMT
// --- With {fmt} available: formatted message sink ---
template <class... Args>
inline void assert_fail(const char* expr,
                        const std::source_location& loc,
                        fmt::format_string<Args...> fmt_str,
                        Args&&... args) {
    assert_header(expr, loc);
    MOTION_CORE_ERROR(fmt_str, std::forward<Args>(args)...);
    MOTION_DEBUGBREAK();
}
#else
// --- No {fmt}: message sink (prints literal; extra args ignored) ---
template <class... Args>
inline void assert_fail(const char* expr,
                        const std::source_location& loc,
                        const char* msg,
                        Args&&... /*unused*/) {
    assert_header(expr, loc);
    if (msg && *msg) MOTION_CORE_ERROR("{}", msg);
    MOTION_DEBUGBREAK();
}
#endif

} // namespace motion::detail

// ======================= ASSERT =======================
#if MOTION_ASSERTS_ENABLED
  #define MOTION_ASSERT(expr, ...)                                                     \
    do {                                                                               \
      const bool _ok_ = static_cast<bool>(expr);                                       \
      if (MOTION_UNLIKELY(!_ok_)) {                                                    \
        ::motion::detail::assert_fail(#expr, std::source_location::current(), ##__VA_ARGS__); \
      }                                                                                \
    } while (0)

  #if MOTION_HAS_FMT
    #define MOTION_ASSERTF(expr, fmt_str, ...)                                         \
      do {                                                                             \
        const bool _ok_ = static_cast<bool>(expr);                                      \
        if (MOTION_UNLIKELY(!_ok_)) {                                                  \
          ::motion::detail::assert_fail(#expr, std::source_location::current(),        \
                                        fmt::format_string{fmt_str}, ##__VA_ARGS__);   \
        }                                                                               \
      } while (0)
  #else
    #define MOTION_ASSERTF(expr, fmt_str, ...)  MOTION_ASSERT((expr), (fmt_str), ##__VA_ARGS__)
  #endif
#else
  #define MOTION_ASSERT(expr, ...)   do { (void)sizeof(expr); } while (0)
  #define MOTION_ASSERTF(expr, ...)  do { (void)sizeof(expr); } while (0)
#endif

// ======================= VERIFY =======================
#if MOTION_ASSERTS_ENABLED
  #define MOTION_VERIFY(expr, ...)                                                     \
    do {                                                                               \
      const bool _ok_ = static_cast<bool>(expr);                                       \
      if (MOTION_UNLIKELY(!_ok_)) {                                                    \
        ::motion::detail::assert_fail(#expr, std::source_location::current(), ##__VA_ARGS__); \
      }                                                                                \
    } while (0)

  #if MOTION_HAS_FMT
    #define MOTION_VERIFYF(expr, fmt_str, ...)                                         \
      do {                                                                             \
        const bool _ok_ = static_cast<bool>(expr);                                      \
        if (MOTION_UNLIKELY(!_ok_)) {                                                  \
          ::motion::detail::assert_fail(#expr, std::source_location::current(),        \
                                        fmt::format_string{fmt_str}, ##__VA_ARGS__);   \
        }                                                                               \
      } while (0)
  #else
    #define MOTION_VERIFYF(expr, fmt_str, ...)  MOTION_VERIFY((expr), (fmt_str), ##__VA_ARGS__)
  #endif
#else
  #define MOTION_VERIFY(expr, ...)   do { (void)(expr); } while (0)
  #define MOTION_VERIFYF(expr, ...)  do { (void)(expr); } while (0)
#endif

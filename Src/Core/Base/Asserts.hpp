#pragma once

#include <source_location>

#include "Base.hpp"
#include "Log.hpp"

#ifdef MOTION_ASSERTS_ENABLED

#define MOTION_ASSERT(condition, ...) \
if(!(condition)){ \
MOTION_CORE_ERROR("Assert failed: {0}", fmt::format(__VA_ARGS__));\
MOTION_CORE_WARN("Assert Info: FILE :- {0} | LINE :- {1}", std::source_location::current().file_name(), std::source_location::current().line());\
MOTION_DEBUGBREAK();\
}

#else

#define MOTION_ASSERT(condition, ...)

#endif 
#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "set"
#include <memory>
#include <functional>
#include <string.h>
#include <optional>
#include <assert.h>

#define VULKAN_PRINT(x)                 \
    {                                   \
        std::cout << "[VULKAN]: " << x; \
    }
#define VULKAN_PRINTLN(x)                       \
    {                                           \
        std::cout << "[VULKAN]: " << x << '\n'; \
    }
#define VULKAN_ERROR(x)                                           \
    {                                                             \
        std::cerr << "[VULKAN]: " << x << "\n"; \
        std::abort();\
}
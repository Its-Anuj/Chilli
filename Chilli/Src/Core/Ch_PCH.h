#pragma once

// Pre Compiled HEader of chilli Engine

#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include "set"
#include "map"
#include <memory>
#include <functional>
#include <optional>

#include "Log.h"

#include <stdexcept> // for std::out_of_range

#if CHILLI_ENABLE_JOLT == true
#include "Jolt/Jolt.h"
#else

#endif
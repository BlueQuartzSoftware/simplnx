#pragma once

#include <nlohmann/json.hpp>

namespace nx::core::ColorTable
{
static const char k_DefaultColorTableJsonCharArray[] = {@COLOR_TABLE_HEX@};

static const nlohmann::json k_DefaultColorTableJson = nlohmann::json::parse(k_DefaultColorTableJsonCharArray);
} // namespace nx::core::ColorTable

#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

namespace nx::core::ColorTable
{
constexpr StringLiteral k_DefaultColorTableJsonStr = R"~(@COLOR_TABLE_JSON@)~";
static const nlohmann::json k_DefaultColorTableJson = R"~(@COLOR_TABLE_JSON@)~"_json;
}
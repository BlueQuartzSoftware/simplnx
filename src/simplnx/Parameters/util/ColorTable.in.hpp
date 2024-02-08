#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace nx::core::ColorTable
{
SIMPLNX_EXPORT constexpr StringLiteral k_ColorTableJsonStr = R"~(@COLOR_TABLE_JSON@)~";
SIMPLNX_EXPORT static const nlohmann::json k_ColorTableJson = R"~(@COLOR_TABLE_JSON@)~"_json;
}
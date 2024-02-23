#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>



namespace nx::core::ColorTable
{
static const nlohmann::json k_DefaultColorTableJson = R"~(@COLOR_TABLE_FLAT@)~"_json;

static const unsigned char k_DefaultColorTableJsonCharArray[] = {
@COLOR_TABLE_HEX@
};

} // namespace nx::core::ColorTable

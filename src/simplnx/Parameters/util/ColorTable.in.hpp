#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace 
{
constexpr nx::core::StringLiteral k_ColorTableJsonStr = R"~(@COLOR_TABLE_JSON@)~";
}

namespace nx::core
{
/**
 * @brief ColorTable
 */
class SIMPLNX_EXPORT ColorTable
{
     
};

}
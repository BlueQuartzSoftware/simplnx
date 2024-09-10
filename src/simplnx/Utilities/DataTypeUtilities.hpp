#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <set>

namespace nx::core
{

using DataTypeSetType = std::set<DataType>;

/**
 * @brief
 */
SIMPLNX_EXPORT const DataTypeSetType& GetAllDataTypes();

/**
 * @brief
 */
SIMPLNX_EXPORT const DataTypeSetType& GetAllNumericTypes();

/**
 * @brief
 */
SIMPLNX_EXPORT const DataTypeSetType& GetIntegerDataTypes();

}; // namespace nx::core

#pragma once

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/Filter/IParameter.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace nx::core::detail
{
// Return the first error for this array. The multi-selection parameter permits the Any wildcard.
inline std::optional<Error> ValidateArraySelection(const IArray& array, const DataPath& path, const std::set<IArray::ArrayType>& allowedArrayTypes, const DataTypeSetType& allowedDataTypes,
                                                   const std::vector<ShapeType>& requiredShapes, std::string_view prefix)
{
  const auto arrayType = array.getArrayType();
  if(!allowedArrayTypes.contains(IArray::ArrayType::Any) && !allowedArrayTypes.contains(arrayType))
  {
    const auto actualTypes = IArray::StringListFromArrayType({arrayType});
    const auto allowedTypes = IArray::StringListFromArrayType(allowedArrayTypes);
    return Error{filter_parameter::constants::k_Validate_ArrayType_Error,
                 fmt::format("{}Array at path '{}' has kind {}. Select an array of one of these kinds: {}.", prefix, path.toString(), fmt::join(actualTypes, ", "), fmt::join(allowedTypes, ", "))};
  }

  std::optional<DataType> elementType;
  if(const auto* dataArrayPtr = dynamic_cast<const IDataArray*>(&array); dataArrayPtr != nullptr)
  {
    elementType = dataArrayPtr->getDataType();
  }
  else if(const auto* neighborListPtr = dynamic_cast<const INeighborList*>(&array); neighborListPtr != nullptr)
  {
    elementType = neighborListPtr->getDataType();
  }
  if(elementType.has_value() && !allowedDataTypes.contains(*elementType))
  {
    const auto actualTypes = IArray::StringListFromArrayType({arrayType});
    const auto allowedTypes = allowedDataTypes | std::views::transform([](DataType type) { return DataTypeToString(type).view(); });
    return Error{filter_parameter::constants::k_ValidateAllowedTypeError,
                 fmt::format("{}{} at path '{}' has element type '{}'. Select an array with one of these element types: {}.", prefix, fmt::join(actualTypes, ", "), path.toString(),
                             DataTypeToString(*elementType).view(), fmt::join(allowedTypes, ", "))};
  }

  if(!requiredShapes.empty())
  {
    const ShapeType componentShape = array.getComponentShape();
    if(std::ranges::find(requiredShapes, componentShape) == requiredShapes.end())
    {
      std::vector<std::string> shapeStrings;
      shapeStrings.reserve(requiredShapes.size());
      for(const auto& shape : requiredShapes)
      {
        shapeStrings.push_back(fmt::format("{}", fmt::join(shape, " x ")));
      }
      return Error{filter_parameter::constants::k_ValidateTupleShapeValue, fmt::format("{}Array at path '{}' has component shape [{}]. Required component shape: {}.", prefix, path.toString(),
                                                                                       fmt::join(componentShape, " x "), fmt::join(shapeStrings, " or "))};
    }
  }

  return std::nullopt;
}
} // namespace nx::core::detail

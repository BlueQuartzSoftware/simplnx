#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <any>
#include <string>
#include <vector>

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx result assertion macros.
namespace image_integration
{
using namespace nx::core;

void ExecuteChecked(const IFilter& filter, DataStructure& dataStructure, const Arguments& args);
DataPath ConvertToFloat32(DataStructure& dataStructure, const DataPath& input);
DataPath Components(DataStructure& dataStructure, const DataPath& input, const std::string& name);
void SetGeometry(DataStructure& dataStructure);
void CheckGeometry(DataStructure& dataStructure, const SizeVec3Type& dimensions);

template <class T, class Scope>
std::vector<T> Read(DataStructure& dataStructure, const DataPath& path, const ShapeType& shape, const Scope& scope)
{
  const DataArray<T>* arrayPtr = nullptr;
  REQUIRE_NOTHROW(arrayPtr = &dataStructure.getDataRefAs<DataArray<T>>(path));
  REQUIRE(arrayPtr->getDataType() == GetDataType<T>());
  REQUIRE(arrayPtr->getTupleShape() == shape);
  REQUIRE(arrayPtr->getComponentShape() == ShapeType{1});
  scope.requireExpectedStore(*arrayPtr);
  const auto& store = arrayPtr->getDataStoreRef();
  if(store.getStoreType() == IDataStore::StoreType::OutOfCore)
  {
    unit_test::RequireDiskBackedStore(*arrayPtr);
  }
  std::vector<T> values(arrayPtr->getSize());
  const auto result = store.copyIntoBuffer(0, nonstd::span<T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  return values;
}

template <class Filter>
Arguments ImageArguments(const Filter& filter, const DataPath& input, const std::string& output)
{
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(Filter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(Filter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(Filter::k_OutputImageArrayName_Key, std::make_any<std::string>(output));
  return args;
}

template <class Filter, class Scope>
void Execute(const Filter& filter, DataStructure& dataStructure, const Arguments& args, Scope& scope)
{
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  const auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

} // namespace image_integration
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

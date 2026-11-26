#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Output.hpp"

#include <catch2/catch.hpp>

#include <memory>
#include <string_view>
#include <utility>

namespace nx::core
{
class IGridGeometry;

namespace UnitTest
{
/** @brief Sets the grid dimensions and attaches Cell Data with the test's tuple shape. */
void SetGridDimensionsAndCellData(IGridGeometry* grid, const SizeVec3& dimensions, const ShapeType& cellShape);

/** @brief Creates the scalar input in a separate, top-level Attribute Matrix named Other Data. */
template <typename T>
DataPath CreateCellTupleCountInput(DataStructure& dataStructure, const ShapeType& tupleShape, T initialValue)
{
  const DataPath path({"Other Data", "Values"});
  auto* matrix = AttributeMatrix::Create(dataStructure, path[0], tupleShape);
  REQUIRE(matrix != nullptr);
  auto store = std::make_shared<DataStore<T>>(tupleShape, ShapeType{1}, initialValue);
  REQUIRE(DataArray<T>::Create(dataStructure, path[1], std::move(store), matrix->getId()) != nullptr);
  return path;
}

/** @brief Checks that preflight reports one tuple-count error for the expected path and geometry. */
void RequireCellTupleCountError(const Result<OutputActions>& result, int32 expectedCode, const DataPath& path, usize tuples, std::string_view geometryMessage);
} // namespace UnitTest
} // namespace nx::core

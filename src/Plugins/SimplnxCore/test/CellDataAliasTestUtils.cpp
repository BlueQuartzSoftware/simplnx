#include "CellDataAliasTestUtils.hpp"

#include "FeatureRemovalTestUtils.hpp"

#include "simplnx/DataStructure/DataGroup.hpp"

#include <array>

namespace nx::core::CellDataAliasTest
{
AttributeMatrix& BuildStrictInput(DataStructure& dataStructure)
{
  const ShapeType shape{1, 2, 2};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_StrictGeometry.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cell Data", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{3}, imageGeomPtr->getId()) != nullptr);
  auto* otherPtr = AttributeMatrix::Create(dataStructure, "Other Data", shape, imageGeomPtr->getId());
  REQUIRE(otherPtr != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "Feature Ids", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 1), cellsPtr->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "Other Values", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 1), otherPtr->getId()) != nullptr);
  return *cellsPtr;
}

void CheckStrictPreflight(const IFilter::PreflightResult& result, int32 expectedCode)
{
  if(expectedCode == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    return;
  }
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  const auto& error = result.outputActions.errors().front();
  CHECK(error.code == expectedCode);
  CHECK(error.message.find(k_OtherValues.toString()) != std::string::npos);
  CHECK(error.message.find(k_StrictCells.toString()) != std::string::npos);
  CHECK(error.message.find(k_StrictGeometry.toString()) != std::string::npos);
}

void BuildAliasInput(DataStructure& dataStructure)
{
  const ShapeType shape{1, 1, 4};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_AliasGeometry.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 1, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  auto* alternateParentPtr = DataGroup::Create(dataStructure, "Alternate Parent");
  REQUIRE(alternateParentPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(cellsPtr->getId(), alternateParentPtr->getId()));
  REQUIRE(imageGeomPtr->getCellDataPath() == k_CanonicalCells);
  auto* idsPtr = Int32Array::Create(dataStructure, "Ids", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 0), cellsPtr->getId());
  auto* ignoredPtr = Int32Array::Create(dataStructure, "Ignored", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 0), cellsPtr->getId());
  REQUIRE(idsPtr != nullptr);
  REQUIRE(ignoredPtr != nullptr);
  constexpr std::array<int32, 4> originalIds{1, 0, 0, 1};
  for(usize index = 0; index < 4; index++)
  {
    (*idsPtr)[index] = originalIds[index];
    (*ignoredPtr)[index] = static_cast<int32>(10 * (index + 1));
  }
}

void CheckAliasPreflight(const IFilter::PreflightResult& result)
{
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  const auto& modified = result.outputActions.value().modifiedActions;
  REQUIRE(modified.size() == 1);
  CHECK(modified.front().modifiedPath == k_CanonicalCells.createChildPath("Ids"));
}

void CheckAliasOutput(const DataStructure& dataStructure, const std::vector<int32>& expectedIds)
{
  CHECK(FeatureRemovalTest::ReadInt32(dataStructure, k_CanonicalCells.createChildPath("Ids")) == expectedIds);
  CHECK(FeatureRemovalTest::ReadInt32(dataStructure, k_CanonicalCells.createChildPath("Ignored")) == std::vector<int32>{10, 20, 30, 40});
  REQUIRE(dataStructure.getId(k_CanonicalCells).has_value());
  CHECK(dataStructure.getId(k_AliasCells) == dataStructure.getId(k_CanonicalCells));
}

void CheckFeatureDataWarning(const IFilter::PreflightResult& result)
{
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(FeatureRemovalTest::HasWarningCode(result.outputActions.warnings(), -14600));
  CHECK(std::any_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.name == "Feature Data Modification Warning"; }));
}
} // namespace nx::core::CellDataAliasTest

#include "FeatureRemovalTestUtils.hpp"

#include "simplnx/DataStructure/DataGroup.hpp"

namespace nx::core::FeatureRemovalTest
{
namespace StrictFeatureIds
{
template <FeatureValue T>
ParentIds BuildInput(DataStructure& dataStructure)
{
  const ShapeType shape{1, 1, 4};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_GeometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 1, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Features", {4}, imageGeomPtr->getId());
  REQUIRE(featuresPtr != nullptr);
  auto* otherPtr = AttributeMatrix::Create(dataStructure, "Other Cells", shape, imageGeomPtr->getId());
  REQUIRE(otherPtr != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Ids", shape, {1}, cellsPtr->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Other Values", shape, {1}, otherPtr->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<T>(dataStructure, "Feature Values", {4}, {1}, featuresPtr->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<T>(dataStructure, "Cell Feature Values", shape, {1}, cellsPtr->getId()) != nullptr);
  auto* elsewherePtr = DataGroup::Create(dataStructure, "Elsewhere");
  REQUIRE(elsewherePtr != nullptr);
  auto* externalFeaturesPtr = AttributeMatrix::Create(dataStructure, "External Features", {4}, elsewherePtr->getId());
  REQUIRE(externalFeaturesPtr != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<T>(dataStructure, "Feature Values", {4}, {1}, externalFeaturesPtr->getId()) != nullptr);
  return {cellsPtr->getId(), elsewherePtr->getId()};
}

template ParentIds BuildInput<int32>(DataStructure&);
template ParentIds BuildInput<bool>(DataStructure&);

void CheckPreflight(const IFilter::PreflightResult& result, int32 expectedCode, const DataPath& offendingPath)
{
  if(expectedCode == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    for(const auto& modification : result.outputActions.value().modifiedActions)
    {
      CHECK(modification.modifiedPath.getParent() != DataPath({"Elsewhere", "Cells"}));
    }
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors().front().code == expectedCode);
    CHECK(result.outputActions.errors().front().message.find(offendingPath.toString()) != std::string::npos);
    CHECK(result.outputActions.errors().front().message.find(k_CellPath.toString()) != std::string::npos);
    CHECK(result.outputActions.errors().front().message.find(k_GeometryPath.toString()) != std::string::npos);
  }
}
} // namespace StrictFeatureIds

namespace SharedCellPaths
{
template <FeatureValue T>
void BuildInput(DataStructure& dataStructure, const std::array<T, 3>& featureValues)
{
  const ShapeType shape{1, 1, 4};
  auto* earlierParentPtr = DataGroup::Create(dataStructure, "Earlier Parent");
  REQUIRE(earlierParentPtr != nullptr);
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_GeometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 1, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  REQUIRE(dataStructure.setAdditionalParent(cellsPtr->getId(), earlierParentPtr->getId()));
  REQUIRE(imageGeomPtr->getCellDataPath() == k_CellPath);
  auto* idsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Ids", shape, {1}, cellsPtr->getId());
  auto* valuesPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Values", shape, {1}, cellsPtr->getId());
  auto* ignoredPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Ignored", shape, {1}, cellsPtr->getId());
  REQUIRE(idsPtr != nullptr);
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(ignoredPtr != nullptr);
  const std::array<int32, 4> originalIds{1, 2, 1, 1};
  for(usize index = 0; index < 4; index++)
  {
    (*idsPtr)[index] = originalIds[index];
    (*valuesPtr)[index] = static_cast<int32>(10 + index);
    (*ignoredPtr)[index] = static_cast<int32>(100 + index);
  }

  // The selected array has another parent; only the selected Feature Attribute Matrix is compacted.
  auto* featureValuesPtr = UnitTest::CreateTestDataArray<T>(dataStructure, "Feature Values", {3}, {1}, earlierParentPtr->getId());
  auto* externalMarkerPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "External Marker", {3}, {1}, earlierParentPtr->getId());
  REQUIRE(featureValuesPtr != nullptr);
  REQUIRE(externalMarkerPtr != nullptr);
  auto* featuresPtr = AttributeMatrix::Create(dataStructure, "Features", {3}, imageGeomPtr->getId());
  REQUIRE(featuresPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(featureValuesPtr->getId(), featuresPtr->getId()));
  auto* markerPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Marker", {3}, {1}, featuresPtr->getId());
  REQUIRE(markerPtr != nullptr);
  for(usize index = 0; index < 3; index++)
  {
    (*markerPtr)[index] = static_cast<int32>(11 * index);
    (*externalMarkerPtr)[index] = static_cast<int32>(90 + index);
  }
  for(usize index = 0; index < featureValues.size(); ++index)
  {
    (*featureValuesPtr)[index] = featureValues[index];
  }
}

template void BuildInput<int32>(DataStructure&, const std::array<int32, 3>&);
template void BuildInput<bool>(DataStructure&, const std::array<bool, 3>&);

void CheckModifiedPaths(const IFilter::PreflightResult& preflight)
{
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  bool reportsValues = false;
  for(const auto& modification : preflight.outputActions.value().modifiedActions)
  {
    CHECK(modification.modifiedPath.getParent() != k_AliasCellPath);
    reportsValues = reportsValues || modification.modifiedPath == k_CellPath.createChildPath("Values");
  }
  CHECK(reportsValues);
}

void CheckCompaction(const DataStructure& dataStructure)
{
  CHECK(ReadInt32(dataStructure, k_CellPath.createChildPath("Ids")) == std::vector<int32>{1, 1, 1, 1});
  const auto values = ReadInt32(dataStructure, k_CellPath.createChildPath("Values"));
  REQUIRE(values.size() == 4);
  CHECK(values[0] == 10);
  CHECK((values[1] == 10 || values[1] == 12));
  CHECK(values[2] == 12);
  CHECK(values[3] == 13);

  const DataPath featurePath = k_FeatureValuesPath.getParent();
  const auto* features = dataStructure.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(features != nullptr);
  CHECK(features->getNumberOfTuples() == 2);
  CHECK(ReadInt32(dataStructure, featurePath.createChildPath("Marker")) == std::vector<int32>{0, 11});
  CHECK(ReadInt32(dataStructure, DataPath({"Earlier Parent", "External Marker"})) == std::vector<int32>{90, 91, 92});
}

void CheckIgnoredValues(const DataStructure& dataStructure)
{
  CHECK(ReadInt32(dataStructure, k_CellPath.createChildPath("Ignored")) == std::vector<int32>{100, 101, 102, 103});
}
} // namespace SharedCellPaths

std::string GetStaleArrayReport(const IFilter::PreflightResult& result, usize expectedCount)
{
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  const IFilter::PreflightValue* report = nullptr;
  usize reportCount = 0;
  for(const auto& value : result.outputValues)
  {
    if(value.name == "Stale Arrays")
    {
      ++reportCount;
      report = &value;
    }
  }
  REQUIRE(reportCount == expectedCount);
  return report == nullptr ? std::string{} : report->value;
}

std::vector<int32> ReadInt32(const DataStructure& dataStructure, const DataPath& path)
{
  CAPTURE(path.toString());
  const auto* array = dataStructure.getDataAs<Int32Array>(path);
  REQUIRE(array != nullptr);
  REQUIRE(array->getNumberOfComponents() == 1);
  std::vector<int32> values(array->getNumberOfTuples());
  const auto result = array->getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  return values;
}

bool HasWarningCode(const std::vector<Warning>& warnings, int32 code)
{
  return std::any_of(warnings.cbegin(), warnings.cend(), [code](const Warning& warning) { return warning.code == code; });
}
} // namespace nx::core::FeatureRemovalTest

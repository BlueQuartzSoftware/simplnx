#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/StoreCopyConfig.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/EmptyStringStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/DataStructure/StringStore.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include "DataStructObserver.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;

TEST_CASE("DataArrayUtilities::ValidateFeatureIds checks every component", "[simplnx][DataArrayUtilities][FeatureIdsValidation]")
{
  UnitTest::LoadPlugins();

  const usize components = GENERATE(usize{1}, usize{2});
  const int32 lastId = GENERATE(-1, 1, 2);
  const bool ignoreNegativeValues = GENERATE(false, true);
  const bool useFeatureArray = GENERATE(false, true);
  CAPTURE(components, lastId, ignoreNegativeValues, useFeatureArray);

  DataStructure dataStructure;
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "Features", ShapeType{2});
  REQUIRE(featureAmPtr != nullptr);
  const DataPath featureArrayPath({"Features", "Values"});
  auto featureStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, featureArrayPath, ShapeType{2}, ShapeType{1});
  REQUIRE(Int32Array::Create(dataStructure, "Values", featureStore, featureAmPtr->getId()) != nullptr);

  const DataPath idsPath({"FeatureIds"});
  auto idsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, ShapeType{2}, ShapeType{components});
  auto* featureIdsPtr = Int32Array::Create(dataStructure, "FeatureIds", idsStore);
  REQUIRE(featureIdsPtr != nullptr);
  std::vector<int32> ids(2 * components, 0);
  ids.back() = lastId;
  const auto copyResult = idsStore->copyFromBuffer(0, nonstd::span<const int32>(ids.data(), ids.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyResult);

  // Two feature tuples have valid indices 0 and 1. Index 2 is outside the array;
  // -1 is valid only when negative labels are explicitly ignored.
  const auto sourcePath = useFeatureArray ? featureArrayPath : DataPath({"Features"});
  const auto result = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(dataStructure, sourcePath, *featureIdsPtr, ignoreNegativeValues, {});
  if(lastId == 2 || (lastId == -1 && !ignoreNegativeValues))
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == (lastId == -1 ? -5355 : -5351));
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataArrayUtilities::ValidateFeatureIds accepts empty arrays", "[simplnx][DataArrayUtilities][FeatureIdsValidation]")
{
  UnitTest::LoadPlugins();

  const usize components = GENERATE(usize{1}, usize{2});
  DataStructure dataStructure;
  REQUIRE(AttributeMatrix::Create(dataStructure, "Features", ShapeType{0}) != nullptr);
  auto store = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"FeatureIds"}), ShapeType{0}, ShapeType{components});
  auto* featureIdsPtr = Int32Array::Create(dataStructure, "FeatureIds", store);
  REQUIRE(featureIdsPtr != nullptr);
  const auto result = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(dataStructure, DataPath({"Features"}), *featureIdsPtr, false, {});
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataArrayUtilities::ValidateCellTupleCount", "[simplnx][DataArrayUtilities]")
{
  const bool useRectGrid = GENERATE(false, true);
  const bool emptyGeometry = GENERATE(false, true);
  const usize tupleCount = GENERATE(usize{0}, usize{1}, usize{59}, usize{60}, usize{61});
  const int32 errorCode = GENERATE(-1234, -5678);
  const usize cellCount = emptyGeometry ? 0 : 60;
  CAPTURE(useRectGrid, emptyGeometry, tupleCount, errorCode);

  DataStructure dataStructure;
  auto* parentPtr = DataGroup::Create(dataStructure, "Geometry Group");
  REQUIRE(parentPtr != nullptr);
  const DataPath geometryPath({"Geometry Group", "Selected Geometry"});
  const DataPath dataPath({"Other Geometry", "Cell Data", "Values"});
  IGridGeometry* gridGeomPtr = nullptr;
  if(useRectGrid)
  {
    gridGeomPtr = RectGridGeom::Create(dataStructure, geometryPath.getTargetName(), parentPtr->getId());
  }
  else
  {
    gridGeomPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName(), parentPtr->getId());
  }
  REQUIRE(gridGeomPtr != nullptr);
  gridGeomPtr->setDimensions({emptyGeometry ? usize{0} : usize{4}, 3, 5});

  auto result = ValidateCellTupleCount(*gridGeomPtr, geometryPath, tupleCount, dataPath, errorCode);
  if(tupleCount == cellCount)
  {
    REQUIRE(result.valid());
  }
  else
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    const auto& error = result.errors().front();
    CHECK(error.code == errorCode);
    CHECK(error.message.find(geometryPath.toString()) != std::string::npos);
    CHECK(error.message.find(dataPath.toString()) != std::string::npos);
    CHECK(error.message.find(fmt::format("{} tuples", tupleCount)) != std::string::npos);
    CHECK(error.message.find(fmt::format("{} cells", cellCount)) != std::string::npos);
    CHECK(error.message.find("Select data with one tuple per geometry cell.") != std::string::npos);
  }
  CHECK(result.warnings().empty());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataArrayUtilities::ValidateCellTupleCount arrays", "[simplnx][DataArrayUtilities][TupleCount]")
{
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  const DataPath geometryPath({"Image"});
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{2}, 0), otherAMPtr->getId()) != nullptr);
  auto* stringsPtr = StringArray::Create(dataStructure, "Strings");
  REQUIRE(stringsPtr != nullptr);
  const auto resizeResult = stringsPtr->resizeTuples({tuples});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  REQUIRE(NeighborList<int32>::Create(dataStructure, "Neighbors", ShapeType{tuples}) != nullptr);
  const DataPath path = GENERATE(DataPath({"Other Data", "Values"}), DataPath({"Strings"}), DataPath({"Neighbors"}));
  REQUIRE(Int32Array::Create(dataStructure, "Matching", std::make_shared<DataStore<int32>>(ShapeType{12}, ShapeType{1}, 0)) != nullptr);
  const auto result = ValidateCellTupleCount(dataStructure, geometryPath, {DataPath({"Matching"}), path}, -98765);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == -98765);
    REQUIRE(result.errors()[0].message ==
            fmt::format("The selected data '{}' has {} tuples, but the selected grid geometry 'Image' has 12 cells. Select data with one tuple per geometry cell.", path.toString(), tuples));
  }
  const auto emptyResult = ValidateCellTupleCount(dataStructure, geometryPath, {}, -98765);
  SIMPLNX_RESULT_REQUIRE_VALID(emptyResult);
}

TEST_CASE("DataArrayUtilities::ValidateCellTupleCount arrays first mismatch", "[simplnx][DataArrayUtilities][TupleCount]")
{
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  REQUIRE(Int32Array::Create(dataStructure, "Matching", std::make_shared<DataStore<int32>>(ShapeType{12}, ShapeType{1}, 0)) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "Short", std::make_shared<DataStore<int32>>(ShapeType{11}, ShapeType{1}, 0)) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "Long", std::make_shared<DataStore<int32>>(ShapeType{13}, ShapeType{1}, 0)) != nullptr);
  const auto result = ValidateCellTupleCount(dataStructure, DataPath({"Image"}), {DataPath({"Matching"}), DataPath({"Short"}), DataPath({"Long"})}, -98765);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors()[0].code == -98765);
  REQUIRE(result.errors()[0].message == "The selected data 'Short' has 11 tuples, but the selected grid geometry 'Image' has 12 cells. Select data with one tuple per geometry cell.");
}

TEST_CASE("DataArrayUtilities::IsChildOfAttributeMatrix", "[simplnx][DataArrayUtilities]")
{
  DataStructure dataStructure;
  auto* firstParentPtr = DataGroup::Create(dataStructure, "First Parent");
  auto* secondParentPtr = DataGroup::Create(dataStructure, "Second Parent");
  REQUIRE(firstParentPtr != nullptr);
  REQUIRE(secondParentPtr != nullptr);
  auto* attributeMatrixPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1}, firstParentPtr->getId());
  REQUIRE(attributeMatrixPtr != nullptr);
  auto* childPtr = Int32Array::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{1}, ShapeType{1}, 0), attributeMatrixPtr->getId());
  REQUIRE(childPtr != nullptr);
  const DataPath childPath({"First Parent", "Cell Data", "Values"});

  SECTION("Direct child")
  {
    REQUIRE(IsChildOfAttributeMatrix(dataStructure, childPath, *attributeMatrixPtr));
  }

  SECTION("DataArray in a sibling Attribute Matrix")
  {
    auto* siblingPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{1}, firstParentPtr->getId());
    REQUIRE(siblingPtr != nullptr);
    auto* siblingChildPtr = Int32Array::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{1}, ShapeType{1}, 0), siblingPtr->getId());
    REQUIRE(siblingChildPtr != nullptr);
    REQUIRE_FALSE(IsChildOfAttributeMatrix(dataStructure, DataPath({"First Parent", "Other Data", "Values"}), *attributeMatrixPtr));
  }

  SECTION("Top-level DataArray")
  {
    auto* topLevelPtr = Int32Array::Create(dataStructure, "Top Level Values", std::make_shared<DataStore<int32>>(ShapeType{1}, ShapeType{1}, 0));
    REQUIRE(topLevelPtr != nullptr);
    REQUIRE_FALSE(IsChildOfAttributeMatrix(dataStructure, DataPath({"Top Level Values"}), *attributeMatrixPtr));
  }

  SECTION("Attribute Matrix reached through two parent paths")
  {
    REQUIRE(dataStructure.setAdditionalParent(attributeMatrixPtr->getId(), secondParentPtr->getId()));
    const DataPath alternateChildPath({"Second Parent", "Cell Data", "Values"});
    REQUIRE(dataStructure.getDataAs<Int32Array>(childPath) == childPtr);
    REQUIRE(dataStructure.getDataAs<Int32Array>(alternateChildPath) == childPtr);
    SECTION("First parent path")
    {
      REQUIRE(IsChildOfAttributeMatrix(dataStructure, childPath, *attributeMatrixPtr));
    }
    SECTION("Second parent path")
    {
      REQUIRE(IsChildOfAttributeMatrix(dataStructure, alternateChildPath, *attributeMatrixPtr));
    }
  }
}

TEST_CASE("DataArrayUtilities::Geometry attribute matrix validation", "[simplnx][DataArrayUtilities][StrictFeatureIds]")
{
  DataStructure dataStructure;
  const DataPath geometryPath({"Geometry"});
  IGridGeometry* gridGeomPtr = nullptr;
  SECTION("Image Geometry")
  {
    gridGeomPtr = ImageGeom::Create(dataStructure, "Geometry");
  }
  SECTION("Rectilinear Grid Geometry")
  {
    gridGeomPtr = RectGridGeom::Create(dataStructure, "Geometry");
  }
  REQUIRE(gridGeomPtr != nullptr);
  const ShapeType shape{1, 2, 2};
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", shape, gridGeomPtr->getId());
  auto* otherPtr = AttributeMatrix::Create(dataStructure, "Other", shape, gridGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  REQUIRE(otherPtr != nullptr);
  gridGeomPtr->setCellData(*cellsPtr);
  auto* groupPtr = DataGroup::Create(dataStructure, "Elsewhere");
  REQUIRE(groupPtr != nullptr);
  auto* nestedPtr = AttributeMatrix::Create(dataStructure, "Features", shape, groupPtr->getId());
  REQUIRE(nestedPtr != nullptr);
  const DataPath cellPath({"Geometry", "Cells"});
  const DataPath arrayPath = cellPath.createChildPath("Ids");
  const DataPath otherPath({"Geometry", "Other", "Ids"});
  REQUIRE(Int32Array::Create(dataStructure, "Ids", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 0), cellsPtr->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "Ids", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 0), otherPtr->getId()) != nullptr);

  const auto checkError = [](const Result<>& result, const std::vector<std::string>& messageParts) {
    CHECK(result.invalid());
    if(result.valid())
    {
      return;
    }
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -12345);
    for(const auto& part : messageParts)
    {
      CHECK(result.errors().front().message.find(part) != std::string::npos);
    }
  };

  CHECK(MapToGeometryCellDataPath(cellPath, DataPath({"Elsewhere", "Cells", "Ids"})) == arrayPath);
  CHECK(MapToGeometryCellDataPath(cellPath, otherPath) == arrayPath);
  CHECK(MapToGeometryCellDataPath(cellPath, arrayPath) == arrayPath);
  const std::vector<DataPath> mappedPaths = MapToGeometryCellDataPath(cellPath, std::vector<DataPath>{DataPath({"Elsewhere", "Cells", "Phases"}), otherPath});
  CHECK(mappedPaths == std::vector<DataPath>{cellPath.createChildPath("Phases"), arrayPath});
  CHECK(MapToGeometryCellDataPath(cellPath, std::vector<DataPath>{}).empty());

  CHECK(ValidateArrayInGeometryCellData(dataStructure, geometryPath, arrayPath, -12345, "Cell Feature Ids").valid());
  checkError(ValidateArrayInGeometryCellData(dataStructure, geometryPath, otherPath, -12345, "Cell Feature Ids"),
             {otherPath.toString(), cellPath.toString(), geometryPath.toString(), "**Cell Feature Ids**"});
  CHECK(ValidateArraysInGeometryCellData(dataStructure, geometryPath, {}, -12345, "Attribute Arrays to Ignore").valid());
  CHECK(ValidateArraysInGeometryCellData(dataStructure, geometryPath, {arrayPath}, -12345, "Attribute Arrays to Ignore").valid());
  checkError(ValidateArraysInGeometryCellData(dataStructure, geometryPath, {arrayPath, otherPath, DataPath({"Missing"})}, -12345, "Attribute Arrays to Ignore"),
             {otherPath.toString(), cellPath.toString(), "**Attribute Arrays to Ignore**"});

  const DataPath featurePath({"Geometry", "Other"});
  CHECK(ValidateFeatureAttributeMatrixInGeometry(dataStructure, geometryPath, featurePath, -12345, "Feature Ranking Array", otherPath).valid());
  checkError(ValidateFeatureAttributeMatrixInGeometry(dataStructure, geometryPath, cellPath, -12345, "Feature Ranking Array", arrayPath),
             {cellPath.toString(), arrayPath.toString(), geometryPath.toString(), "**Feature Ranking Array**"});
  const DataPath nestedPath({"Elsewhere", "Features"});
  checkError(ValidateFeatureAttributeMatrixInGeometry(dataStructure, geometryPath, nestedPath, -12345, "Feature Ranking Array", nestedPath.createChildPath("Rank")),
             {nestedPath.toString(), nestedPath.createChildPath("Rank").toString(), geometryPath.toString()});
  REQUIRE(DataGroup::Create(dataStructure, "Not an AM", gridGeomPtr->getId()) != nullptr);
  const DataPath groupPath({"Geometry", "Not an AM"});
  checkError(ValidateFeatureAttributeMatrixInGeometry(dataStructure, geometryPath, groupPath, -12345, "Feature Ranking Array", groupPath.createChildPath("Rank")),
             {groupPath.toString(), geometryPath.toString()});

  REQUIRE(dataStructure.setAdditionalParent(otherPtr->getId(), groupPtr->getId()));
  CHECK(ValidateFeatureAttributeMatrixInGeometry(dataStructure, geometryPath, DataPath({"Elsewhere", "Other"}), -12345, "Feature Ranking Array", DataPath({"Elsewhere", "Other", "Ids"})).valid());

  CHECK(ValidateFeatureAttributeMatrixInGeometry(dataStructure, geometryPath, featurePath, -12345, "Feature Attribute Matrix").valid());
  CHECK(ValidateFeatureAttributeMatrixInGeometry(dataStructure, geometryPath, DataPath({"Elsewhere", "Other"}), -12345, "Feature Attribute Matrix").valid());
  for(const auto& invalidPath : {cellPath, nestedPath, groupPath, DataPath({"Missing"})})
  {
    auto result = ValidateFeatureAttributeMatrixInGeometry(dataStructure, geometryPath, invalidPath, -12345, "Feature Attribute Matrix");
    checkError(result, {invalidPath.toString(), geometryPath.toString(), "**Feature Attribute Matrix**", "must not be its Cell Attribute Matrix"});
    REQUIRE(result.invalid());
    CHECK(result.errors().front().message.find("parent of") == std::string::npos);
  }

  auto* noCellsPtr = ImageGeom::Create(dataStructure, "No Cells");
  REQUIRE(noCellsPtr != nullptr);
  const DataPath noCellsPath({"No Cells"});
  checkError(ValidateArrayInGeometryCellData(dataStructure, noCellsPath, arrayPath, -12345, "Cell Feature Ids"), {noCellsPath.toString(), arrayPath.toString()});
  checkError(ValidateArraysInGeometryCellData(dataStructure, noCellsPath, {}, -12345, "Attribute Arrays to Ignore"), {noCellsPath.toString(), "**Attribute Arrays to Ignore**"});
  checkError(ValidateFeatureAttributeMatrixInGeometry(dataStructure, noCellsPath, featurePath, -12345, "Feature Ranking Array", otherPath),
             {noCellsPath.toString(), featurePath.toString(), otherPath.toString()});
  checkError(ValidateFeatureAttributeMatrixInGeometry(dataStructure, noCellsPath, featurePath, -12345, "Feature Attribute Matrix"),
             {noCellsPath.toString(), featurePath.toString(), "**Feature Attribute Matrix**", "requires a Cell Attribute Matrix"});
  checkError(ValidateFeatureAttributeMatrixInGeometry(dataStructure, DataPath({"Missing"}), featurePath, -12345, "Feature Attribute Matrix"),
             {"Missing", featurePath.toString(), "requires a Cell Attribute Matrix"});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
/**
 * @brief Enforces the required format-taking store interface.
 * @tparam Store Abstract or concrete store type under test.
 */
template <typename Store>
void CheckStoreCopyContract()
{
  constexpr bool k_AcceptsFormat = requires(const Store& store, const std::string& format) { store.deepCopy(format); };
  constexpr bool k_AcceptsNoFormat = requires(const Store& store) { store.deepCopy(); };
  static_assert(k_AcceptsFormat);
  static_assert(!k_AcceptsNoFormat);
  CHECK(k_AcceptsFormat);
  CHECK_FALSE(k_AcceptsNoFormat);
}

constexpr StringLiteral k_BuildDir = SIMPLNX_BUILD_DIR;

class FailingResizeDataStore final : public DataStore<int32>
{
public:
  using DataStore<int32>::DataStore;

  /**
   * @brief Returns an injected resize failure for AttributeMatrix contract testing.
   * @param tupleShape Requested tuple dimensions.
   * @return Error -6035 for every request.
   */
  [[nodiscard]] Result<> resizeTuples(const ShapeType& tupleShape) override
  {
    return MakeErrorResult(-6035, fmt::format("Test store resize to shape [{}] failed: injected failure", fmt::join(tupleShape, ", ")));
  }
};

class RecordingFormatResolver final : public IDataStoreFormatResolver
{
public:
  std::string resolveFormat(const DataStructure& /*dataStructure*/, const DataPath& arrayPath, DataType /*numericType*/, uint64 /*dataSizeBytes*/) const override
  {
    const std::scoped_lock lock(m_Mutex);
    m_RequestedPaths.push_back(arrayPath);
    return {};
  }

  std::vector<DataPath> requestedPaths() const
  {
    const std::scoped_lock lock(m_Mutex);
    return m_RequestedPaths;
  }

private:
  mutable std::mutex m_Mutex;
  mutable std::vector<DataPath> m_RequestedPaths;
};
} // namespace

TEST_CASE("Array")
{
  FloatVec3 vec0(3.0F, 4.0F, 5.0F);

  IntVec3 v0(1, 2, 3);
  IntVec3 v1(0, 0, 0);
  v1.setValues(1, 2, 3);
  REQUIRE(v1[0] == 1);
  REQUIRE(v1[1] == 2);
  REQUIRE(v1[2] == 3);

  auto v1Tuple = v1.toTuple();
  FloatVec3 v1F32 = v1.convertType<float>();

  FloatVec3 v2(-3.0F, -4.0F, -5.0F);
  FloatVec3 cross = vec0.cross(v2);
  REQUIRE(cross[0] == 0);
  REQUIRE(cross[1] == 0);
  REQUIRE(cross[2] == 0);

  float32 dot = v2.dot(vec0);
  REQUIRE(dot == -50.0F);

  float32 mag = vec0.magnitude();
  REQUIRE(mag == std::sqrt(50.0F));
}

TEST_CASE("DataArrayCreation")
{
  nx::core::DataStructure dataStructure;

  using DataStoreType = nx::core::DataStore<int32_t>;
  DataStoreType dataArray = DataStoreType(nx::core::ShapeType{0}, nx::core::ShapeType{2}, 0);
  size_t numTuples = dataArray.getNumberOfTuples();
  REQUIRE(numTuples == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("CopyUsingIndexList maps data arrays", "[simplnx][DataArray][CopyUsingIndexList]", bool, int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64)
{
  DataStructure dataStructure;
  auto* sourceArrayPtr = DataArray<TestType>::Create(dataStructure, "Source", std::make_shared<DataStore<TestType>>(ShapeType{3}, ShapeType{2}, TestType{0}));
  auto* destArrayPtr = DataArray<TestType>::Create(dataStructure, "Destination", std::make_shared<DataStore<TestType>>(ShapeType{4}, ShapeType{2}, TestType{1}));
  REQUIRE(sourceArrayPtr != nullptr);
  REQUIRE(destArrayPtr != nullptr);
  const std::array<TestType, 6> sourceValues = {TestType{1}, TestType{0}, TestType{0}, TestType{1}, TestType{1}, TestType{1}};
  for(usize valueIndex = 0; valueIndex < sourceValues.size(); ++valueIndex)
  {
    (*sourceArrayPtr)[valueIndex] = sourceValues[valueIndex];
  }
  const std::vector<int64> indexMap = {2, -1, 0, 2};
  const nonstd::span<const int64> indexSpan(indexMap);
  CopyFromArray::ParallelTaskResult taskResult;
  ParallelTaskAlgorithm runner;
  runner.setParallelizationEnabled(false);
  CopyFromArray::RunParallelCopyUsingIndexList(*destArrayPtr, runner, taskResult, *sourceArrayPtr, indexSpan);
  runner.wait();
  REQUIRE(taskResult.takeResult().valid());
  const std::array<TestType, 8> expectedValues = {TestType{1}, TestType{1}, TestType{0}, TestType{0}, TestType{1}, TestType{0}, TestType{1}, TestType{1}};
  for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
  {
    REQUIRE((*destArrayPtr)[valueIndex] == expectedValues[valueIndex]);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CopyUsingIndexList maps neighbor lists", "[simplnx][NeighborList][CopyUsingIndexList]")
{
  DataStructure dataStructure;
  auto* sourceListPtr = NeighborList<int32>::Create(dataStructure, "Source", ShapeType{3});
  auto* destListPtr = NeighborList<int32>::Create(dataStructure, "Destination", ShapeType{4});
  REQUIRE(sourceListPtr != nullptr);
  REQUIRE(destListPtr != nullptr);
  sourceListPtr->setList(0, std::vector<int32>{7, 8});
  sourceListPtr->setList(1, std::vector<int32>{9});
  sourceListPtr->setList(2, std::vector<int32>{10, 11, 12});
  for(int32 tupleIndex = 0; tupleIndex < 4; ++tupleIndex)
  {
    destListPtr->setList(tupleIndex, std::vector<int32>{99});
  }
  const std::vector<int64> indexMap = {2, -1, 0, 2};
  const nonstd::span<const int64> indexSpan(indexMap);
  CopyFromArray::ParallelTaskResult taskResult;
  ParallelTaskAlgorithm runner;
  runner.setParallelizationEnabled(false);
  CopyFromArray::RunParallelCopyUsingIndexList(*destListPtr, runner, taskResult, *sourceListPtr, indexSpan);
  runner.wait();
  REQUIRE(taskResult.takeResult().valid());
  REQUIRE(destListPtr->getList(0) == std::vector<int32>{10, 11, 12});
  REQUIRE(destListPtr->getList(1).empty());
  REQUIRE(destListPtr->getList(2) == std::vector<int32>{7, 8});
  REQUIRE(destListPtr->getList(3) == std::vector<int32>{10, 11, 12});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("nx::core::IDataStore getPlannedStoreType", "[simplnx][DataStore]")
{
  // A real in-memory DataStore reports InMemory for both its actual and planned store type.
  DataStore<int32> inMemoryStore(ShapeType{4}, ShapeType{1}, 0);
  REQUIRE(inMemoryStore.getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(inMemoryStore.getPlannedStoreType() == IDataStore::StoreType::InMemory);

  // An Empty placeholder with no data format is destined for in-memory storage.
  auto emptyInMemory = EmptyDataStore<int32>::Create(ShapeType{4}, ShapeType{1}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(emptyInMemory);
  REQUIRE(emptyInMemory.value()->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(emptyInMemory.value()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
}

TEMPLATE_TEST_CASE("DataArray deepCopy preserves resident initialization", "[simplnx][DataArray][DeepCopyInitialization]", int32, float32, bool)
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  const std::optional<TestType> initValue = GENERATE(std::optional<TestType>{TestType{1}}, std::optional<TestType>{});
  CAPTURE(initValue.has_value());

  auto* sourceAMPtr = AttributeMatrix::Create(dataStructure, "Source", {1});
  REQUIRE(sourceAMPtr != nullptr);
  auto sourceStore = std::make_shared<DataStore<TestType>>(ShapeType{1}, ShapeType{2}, initValue);
  sourceStore->setValue(0, TestType{0});
  sourceStore->setValue(1, TestType{1});
  auto* sourceArrayPtr = DataArray<TestType>::Create(dataStructure, "Values", sourceStore, sourceAMPtr->getId());
  REQUIRE(sourceArrayPtr != nullptr);

  auto copy = sourceAMPtr->deepCopy(DataPath({"Copy"}));
  REQUIRE(copy != nullptr);
  AttributeMatrix* copyAMPtr = nullptr;
  DataArray<TestType>* copyArrayPtr = nullptr;
  REQUIRE_NOTHROW(copyAMPtr = &dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Copy"})));
  REQUIRE_NOTHROW(copyArrayPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(DataPath({"Copy", "Values"})));
  REQUIRE(copyArrayPtr->getIDataStore() != sourceArrayPtr->getIDataStore());
  const auto* copyStorePtr = dynamic_cast<const DataStore<TestType>*>(copyArrayPtr->getIDataStore());
  REQUIRE(copyStorePtr != nullptr);
  CHECK(copyStorePtr->getInitValue() == initValue);

  const Result<> resizeResult = copyAMPtr->resizeTuples({4});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  CHECK(copyArrayPtr->getDataStoreRef().getValue(0) == TestType{0});
  CHECK(copyArrayPtr->getDataStoreRef().getValue(1) == TestType{1});
  const TestType expectedGrowthValue = initValue.value_or(GetMudflap<TestType>());
  for(usize valueIndex = 2; valueIndex < 8; ++valueIndex)
  {
    CHECK(copyArrayPtr->getDataStoreRef().getValue(valueIndex) == expectedGrowthValue);
  }

  CHECK(sourceArrayPtr->getNumberOfTuples() == 1);
  CHECK(sourceArrayPtr->getDataStoreRef().getValue(0) == TestType{0});
  CHECK(sourceArrayPtr->getDataStoreRef().getValue(1) == TestType{1});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataArray deepCopy preserves signed initialization and destination behavior", "[simplnx][DataArray][DeepCopyInitialization]")
{
  DataStructure dataStructure;
  auto resolver = std::make_shared<RecordingFormatResolver>();
  dataStructure.setFormatResolver(resolver);
  const DataPath sourcePath({"Source"});
  const DataPath copyPath({"Copy"});
  auto sourceStore = std::make_shared<DataStore<int32>>(ShapeType{1}, ShapeType{1}, std::optional<int32>{-1});
  sourceStore->setValue(0, 7);
  auto* sourceArrayPtr = DataArray<int32>::Create(dataStructure, sourcePath.getTargetName(), sourceStore);
  REQUIRE(sourceArrayPtr != nullptr);

  auto firstCopy = sourceArrayPtr->deepCopy(copyPath);
  REQUIRE(firstCopy != nullptr);
  DataArray<int32>* copyArrayPtr = nullptr;
  REQUIRE_NOTHROW(copyArrayPtr = &dataStructure.getDataRefAs<DataArray<int32>>(copyPath));
  REQUIRE(copyArrayPtr->getIDataStore() != sourceArrayPtr->getIDataStore());
  const std::vector<DataPath> expectedRequests = {copyPath};
  CHECK(resolver->requestedPaths() == expectedRequests);

  const Result<> resizeResult = copyArrayPtr->resizeTuples({4});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  const std::array<int32, 4> expectedValues = {7, -1, -1, -1};
  for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
  {
    CHECK(copyArrayPtr->getDataStoreRef().getValue(valueIndex) == expectedValues[valueIndex]);
  }
  CHECK(sourceArrayPtr->getNumberOfTuples() == 1);
  CHECK(sourceArrayPtr->getDataStoreRef().getValue(0) == 7);

  auto collision = sourceArrayPtr->deepCopy(copyPath);
  CHECK(collision == nullptr);
  CHECK(dataStructure.getDataAs<DataArray<int32>>(copyPath) == copyArrayPtr);
  CHECK(copyArrayPtr->getNumberOfTuples() == 4);
  for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
  {
    CHECK(copyArrayPtr->getDataStoreRef().getValue(valueIndex) == expectedValues[valueIndex]);
  }
  CHECK(resolver->requestedPaths() == expectedRequests);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataArray deepCopy preserves Empty store metadata", "[simplnx][DataArray][DeepCopyInitialization]")
{
  DataStructure dataStructure;
  auto sourceStoreResult = EmptyDataStore<int32>::Create(ShapeType{1}, ShapeType{2}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(sourceStoreResult);
  std::shared_ptr<EmptyDataStore<int32>> sourceStore(std::move(sourceStoreResult.value()));
  auto* sourceArrayPtr = DataArray<int32>::Create(dataStructure, "Source", sourceStore);
  REQUIRE(sourceArrayPtr != nullptr);

  auto copy = sourceArrayPtr->deepCopy(DataPath({"Copy"}));
  REQUIRE(copy != nullptr);
  DataArray<int32>* copyArrayPtr = nullptr;
  REQUIRE_NOTHROW(copyArrayPtr = &dataStructure.getDataRefAs<DataArray<int32>>(DataPath({"Copy"})));
  REQUIRE(copyArrayPtr->getIDataStore() != sourceArrayPtr->getIDataStore());

  const auto* sourceEmptyStorePtr = sourceArrayPtr->getIDataStoreAs<EmptyDataStore<int32>>();
  const auto* copyEmptyStorePtr = copyArrayPtr->getIDataStoreAs<EmptyDataStore<int32>>();
  REQUIRE(sourceEmptyStorePtr != nullptr);
  REQUIRE(copyEmptyStorePtr != nullptr);
  CHECK(copyEmptyStorePtr->getStoreType() == IDataStore::StoreType::Empty);
  CHECK(copyEmptyStorePtr->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  CHECK(copyEmptyStorePtr->getTupleShape() == ShapeType{1});
  CHECK(copyEmptyStorePtr->getComponentShape() == ShapeType{2});
  CHECK(copyEmptyStorePtr->getDataFormat().empty());
  CHECK(copyEmptyStorePtr->getTupleShape() == sourceEmptyStorePtr->getTupleShape());
  CHECK(copyEmptyStorePtr->getComponentShape() == sourceEmptyStorePtr->getComponentShape());
  CHECK(copyEmptyStorePtr->getDataFormat() == sourceEmptyStorePtr->getDataFormat());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("nx::core::DataArray Copy TupleTest", "[simplnx][DataArray]")
{
  UnitTest::LoadPlugins();

  const std::string kDataArrayName("DataArray");
  const DataPath kDataPath({kDataArrayName});
  const usize kNumTuples = 5;
  const usize kNumComponents = 3;

  DataStructure dataStructure;
  ShapeType tupleShape{kNumTuples};
  ShapeType componentShape{kNumComponents};
  Result<> result = ArrayCreationUtilities::CreateArray<int32>(dataStructure, tupleShape, componentShape, kDataPath, IDataAction::Mode::Execute);
  REQUIRE(result.valid() == true);

  auto& dataArray = dataStructure.getDataRefAs<DataArray<int32>>(kDataPath);

  for(usize i = 0; i < kNumTuples; i++)
  {
    dataArray.initializeTuple(i, static_cast<int32>(i));
  }

  for(usize tupleIndex = 0; tupleIndex < kNumTuples; tupleIndex++)
  {
    for(usize componentIndex = 0; componentIndex < kNumComponents; componentIndex++)
    {
      uint64 index = tupleIndex * 3 + componentIndex;
      REQUIRE(dataArray[index] == static_cast<int32>(tupleIndex));
    }
  }

  dataArray.copyTuple(4, 0);
  REQUIRE(dataArray[0] == 4);
  REQUIRE(dataArray[1] == 4);
  REQUIRE(dataArray[2] == 4);

  dataArray.copyTuple(1, 4);
  REQUIRE(dataArray[12] == 1);
  REQUIRE(dataArray[13] == 1);
  REQUIRE(dataArray[14] == 1);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataStore Test")
{
  ShapeType tupleShape{5};
  ShapeType componentShape{3};
  DataStore<int32> dataStore(tupleShape, componentShape, 5);

  REQUIRE(dataStore.getSize() == 15);

  for(uint64_t i = 0; i < dataStore.getSize(); i++)
  {
    REQUIRE(dataStore[i] == 5);
  }

  int32 newArrayValues[] = {6, 7, 8};
  dataStore.setTuple(0, newArrayValues);
  for(uint64 i = 0; i < 3; i++)
  {
    REQUIRE(dataStore[i] == newArrayValues[i]);
    REQUIRE(dataStore.getComponentValue(0, i) == newArrayValues[i]);
  }

  std::vector<int32> newValues{1, 2, 3};
  dataStore.setTuple(1, newValues);
  usize offset = dataStore.getNumberOfComponents();
  for(usize i = 0; i < newValues.size(); i++)
  {
    REQUIRE(dataStore[offset + i] == newValues[i]);
    REQUIRE(dataStore.getComponentValue(1, i) == newValues[i]);
  }

  dataStore.setComponent(2, 2, 99);
  REQUIRE(dataStore[8] == 99);
  REQUIRE(dataStore.getComponentValue(2, 2) == 99);
}

TEST_CASE("DataStore allocation contract", "[simplnx][DataStore]")
{
  SECTION("initialized growth preserves the prefix and initializes the tail")
  {
    DataStore<int32> dataStore(ShapeType{2}, ShapeType{2}, 7);
    const std::array<int32, 4> initialValues = {7, 7, 7, 7};
    for(usize valueIndex = 0; valueIndex < initialValues.size(); ++valueIndex)
    {
      REQUIRE(dataStore[valueIndex] == initialValues[valueIndex]);
    }

    const std::array<int32, 4> prefixValues = {10, 11, 20, 21};
    for(usize valueIndex = 0; valueIndex < prefixValues.size(); ++valueIndex)
    {
      dataStore[valueIndex] = prefixValues[valueIndex];
    }

    const Result<> resizeResult = dataStore.resizeTuples(ShapeType{4});
    REQUIRE(resizeResult.valid());
    REQUIRE(dataStore.getTupleShape() == ShapeType{4});
    REQUIRE(dataStore.getComponentShape() == ShapeType{2});
    REQUIRE(dataStore.getNumberOfTuples() == 4);
    REQUIRE(dataStore.getNumberOfComponents() == 2);
    REQUIRE(dataStore.getSize() == 8);

    const std::array<int32, 8> expectedValues = {10, 11, 20, 21, 7, 7, 7, 7};
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      REQUIRE(dataStore[valueIndex] == expectedValues[valueIndex]);
    }
  }

  SECTION("zero-sized initialized storage grows to initialized values")
  {
    DataStore<int32> dataStore(ShapeType{0}, ShapeType{2}, 7);
    REQUIRE(dataStore.getTupleShape() == ShapeType{0});
    REQUIRE(dataStore.getComponentShape() == ShapeType{2});
    REQUIRE(dataStore.getNumberOfTuples() == 0);
    REQUIRE(dataStore.getNumberOfComponents() == 2);
    REQUIRE(dataStore.getSize() == 0);

    const Result<> resizeResult = dataStore.resizeTuples(ShapeType{2});
    REQUIRE(resizeResult.valid());
    REQUIRE(dataStore.getTupleShape() == ShapeType{2});
    REQUIRE(dataStore.getComponentShape() == ShapeType{2});
    REQUIRE(dataStore.getNumberOfTuples() == 2);
    REQUIRE(dataStore.getNumberOfComponents() == 2);
    REQUIRE(dataStore.getSize() == 4);

    const std::array<int32, 4> expectedValues = {7, 7, 7, 7};
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      REQUIRE(dataStore[valueIndex] == expectedValues[valueIndex]);
    }
  }

  SECTION("copy construction preserves values and remembered initialization")
  {
    DataStore<int32> sourceDataStore(ShapeType{2}, ShapeType{2}, 7);
    const std::array<int32, 4> originalValues = {10, 11, 20, 21};
    for(usize valueIndex = 0; valueIndex < originalValues.size(); ++valueIndex)
    {
      sourceDataStore[valueIndex] = originalValues[valueIndex];
    }

    DataStore<int32> copiedDataStore(sourceDataStore);
    REQUIRE(copiedDataStore.getTupleShape() == ShapeType{2});
    REQUIRE(copiedDataStore.getComponentShape() == ShapeType{2});
    REQUIRE(copiedDataStore.getNumberOfTuples() == 2);
    REQUIRE(copiedDataStore.getNumberOfComponents() == 2);
    REQUIRE(copiedDataStore.getSize() == 4);
    REQUIRE(copiedDataStore.data() != sourceDataStore.data());
    for(usize valueIndex = 0; valueIndex < originalValues.size(); ++valueIndex)
    {
      REQUIRE(copiedDataStore[valueIndex] == originalValues[valueIndex]);
    }

    sourceDataStore[0] = 99;
    const std::array<int32, 4> changedSourceValues = {99, 11, 20, 21};
    for(usize valueIndex = 0; valueIndex < originalValues.size(); ++valueIndex)
    {
      REQUIRE(sourceDataStore[valueIndex] == changedSourceValues[valueIndex]);
      REQUIRE(copiedDataStore[valueIndex] == originalValues[valueIndex]);
    }

    const Result<> resizeResult = copiedDataStore.resizeTuples(ShapeType{3});
    REQUIRE(resizeResult.valid());
    REQUIRE(copiedDataStore.getTupleShape() == ShapeType{3});
    REQUIRE(copiedDataStore.getComponentShape() == ShapeType{2});
    REQUIRE(copiedDataStore.getNumberOfTuples() == 3);
    REQUIRE(copiedDataStore.getNumberOfComponents() == 2);
    REQUIRE(copiedDataStore.getSize() == 6);
    const std::array<int32, 6> expectedCopiedValues = {10, 11, 20, 21, 7, 7};
    for(usize valueIndex = 0; valueIndex < expectedCopiedValues.size(); ++valueIndex)
    {
      REQUIRE(copiedDataStore[valueIndex] == expectedCopiedValues[valueIndex]);
    }

    REQUIRE(sourceDataStore.getTupleShape() == ShapeType{2});
    REQUIRE(sourceDataStore.getComponentShape() == ShapeType{2});
    REQUIRE(sourceDataStore.getNumberOfTuples() == 2);
    REQUIRE(sourceDataStore.getNumberOfComponents() == 2);
    REQUIRE(sourceDataStore.getSize() == 4);
    for(usize valueIndex = 0; valueIndex < changedSourceValues.size(); ++valueIndex)
    {
      REQUIRE(sourceDataStore[valueIndex] == changedSourceValues[valueIndex]);
    }
  }

  SECTION("caller-owned storage grows with the integer mudflap value")
  {
    auto buffer = std::make_unique<int32[]>(4);
    const std::array<int32, 4> initialValues = {31, 32, 33, 34};
    for(usize valueIndex = 0; valueIndex < initialValues.size(); ++valueIndex)
    {
      buffer[valueIndex] = initialValues[valueIndex];
    }
    const int32* const bufferPtr = buffer.get();

    DataStore<int32> dataStore(std::move(buffer), ShapeType{2}, ShapeType{2});
    REQUIRE(buffer == nullptr);
    REQUIRE(dataStore.data() == bufferPtr);
    for(usize valueIndex = 0; valueIndex < initialValues.size(); ++valueIndex)
    {
      REQUIRE(dataStore[valueIndex] == initialValues[valueIndex]);
    }

    const Result<> resizeResult = dataStore.resizeTuples(ShapeType{3});
    REQUIRE(resizeResult.valid());
    REQUIRE(dataStore.getTupleShape() == ShapeType{3});
    REQUIRE(dataStore.getComponentShape() == ShapeType{2});
    REQUIRE(dataStore.getNumberOfTuples() == 3);
    REQUIRE(dataStore.getNumberOfComponents() == 2);
    REQUIRE(dataStore.getSize() == 6);
    const std::array<int32, 6> expectedValues = {31, 32, 33, 34, GetMudflap<int32>(), GetMudflap<int32>()};
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      REQUIRE(dataStore[valueIndex] == expectedValues[valueIndex]);
    }
  }

  SECTION("same-size reshape preserves the buffer and values")
  {
    DataStore<int32> dataStore(ShapeType{4}, ShapeType{1}, 7);
    const std::array<int32, 4> expectedValues = {10, 20, 30, 40};
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      dataStore[valueIndex] = expectedValues[valueIndex];
    }
    const int32* const originalDataPtr = dataStore.data();

    const Result<> resizeResult = dataStore.resizeTuples(ShapeType{2, 2});
    REQUIRE(resizeResult.valid());
    REQUIRE(dataStore.getTupleShape() == ShapeType{2, 2});
    REQUIRE(dataStore.getComponentShape() == ShapeType{1});
    REQUIRE(dataStore.getNumberOfTuples() == 4);
    REQUIRE(dataStore.getNumberOfComponents() == 1);
    REQUIRE(dataStore.getSize() == 4);
    REQUIRE(dataStore.data() == originalDataPtr);
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      REQUIRE(dataStore[valueIndex] == expectedValues[valueIndex]);
    }
  }

  SECTION("impossible array length preserves the original state")
  {
    DataStore<int32> dataStore(ShapeType{4}, ShapeType{1}, 7);
    const std::array<int32, 4> expectedValues = {10, 20, 30, 40};
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      dataStore[valueIndex] = expectedValues[valueIndex];
    }
    const int32* const originalDataPtr = dataStore.data();
    const ShapeType originalTupleShape = dataStore.getTupleShape();
    const ShapeType originalComponentShape = dataStore.getComponentShape();
    const usize originalTupleCount = dataStore.getNumberOfTuples();
    const usize originalComponentCount = dataStore.getNumberOfComponents();
    const usize originalSize = dataStore.getSize();
    // The element count fits usize, but its int32 byte count does not. The store rejects the byte count before requesting memory.
    const usize impossibleTuples = (std::numeric_limits<usize>::max)() / sizeof(int32) + 1;

    const Result<> resizeResult = dataStore.resizeTuples(ShapeType{impossibleTuples});
    REQUIRE(resizeResult.invalid());
    REQUIRE_FALSE(resizeResult.errors().empty());
    REQUIRE(resizeResult.errors()[0].code == -6035);
    REQUIRE(resizeResult.errors()[0].message.find("byte count") != std::string::npos);
    REQUIRE(dataStore.data() == originalDataPtr);
    REQUIRE(dataStore.getTupleShape() == originalTupleShape);
    REQUIRE(dataStore.getComponentShape() == originalComponentShape);
    REQUIRE(dataStore.getNumberOfTuples() == originalTupleCount);
    REQUIRE(dataStore.getNumberOfComponents() == originalComponentCount);
    REQUIRE(dataStore.getSize() == originalSize);
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      REQUIRE(dataStore[valueIndex] == expectedValues[valueIndex]);
    }
  }

  SECTION("overflowing tuple-shape product preserves the original state")
  {
    DataStore<int32> dataStore(ShapeType{4}, ShapeType{1}, 7);
    const std::array<int32, 4> expectedValues = {10, 20, 30, 40};
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      dataStore[valueIndex] = expectedValues[valueIndex];
    }
    const int32* const originalDataPtr = dataStore.data();
    const ShapeType originalTupleShape = dataStore.getTupleShape();
    const ShapeType originalComponentShape = dataStore.getComponentShape();
    const usize originalTupleCount = dataStore.getNumberOfTuples();
    const usize originalComponentCount = dataStore.getNumberOfComponents();
    const usize originalSize = dataStore.getSize();
    const Result<> resizeResult = dataStore.resizeTuples(ShapeType{(std::numeric_limits<usize>::max)(), 2});
    REQUIRE(resizeResult.invalid());
    REQUIRE_FALSE(resizeResult.errors().empty());
    REQUIRE(resizeResult.errors()[0].code == -6035);
    REQUIRE(resizeResult.errors()[0].message.find("tuple count") != std::string::npos);
    REQUIRE(dataStore.data() == originalDataPtr);
    REQUIRE(dataStore.getTupleShape() == originalTupleShape);
    REQUIRE(dataStore.getComponentShape() == originalComponentShape);
    REQUIRE(dataStore.getNumberOfTuples() == originalTupleCount);
    REQUIRE(dataStore.getNumberOfComponents() == originalComponentCount);
    REQUIRE(dataStore.getSize() == originalSize);
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      REQUIRE(dataStore[valueIndex] == expectedValues[valueIndex]);
    }
  }

  SECTION("overflowing element count preserves the original state")
  {
    DataStore<int32> dataStore(ShapeType{4}, ShapeType{2}, 7);
    const std::array<int32, 8> expectedValues = {10, 20, 30, 40, 50, 60, 70, 80};
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      dataStore[valueIndex] = expectedValues[valueIndex];
    }
    const int32* const originalDataPtr = dataStore.data();
    const ShapeType originalTupleShape = dataStore.getTupleShape();
    const ShapeType originalComponentShape = dataStore.getComponentShape();
    const usize originalTupleCount = dataStore.getNumberOfTuples();
    const usize originalComponentCount = dataStore.getNumberOfComponents();
    const usize originalSize = dataStore.getSize();
    // The tuple count fits usize, but multiplying by two components reaches the element-count overflow branch.
    const Result<> resizeResult = dataStore.resizeTuples(ShapeType{(std::numeric_limits<usize>::max)() / 2 + 1});
    REQUIRE(resizeResult.invalid());
    REQUIRE_FALSE(resizeResult.errors().empty());
    REQUIRE(resizeResult.errors()[0].code == -6035);
    REQUIRE(resizeResult.errors()[0].message.find("element count") != std::string::npos);
    REQUIRE(dataStore.data() == originalDataPtr);
    REQUIRE(dataStore.getTupleShape() == originalTupleShape);
    REQUIRE(dataStore.getComponentShape() == originalComponentShape);
    REQUIRE(dataStore.getNumberOfTuples() == originalTupleCount);
    REQUIRE(dataStore.getNumberOfComponents() == originalComponentCount);
    REQUIRE(dataStore.getSize() == originalSize);
    for(usize valueIndex = 0; valueIndex < expectedValues.size(); ++valueIndex)
    {
      REQUIRE(dataStore[valueIndex] == expectedValues[valueIndex]);
    }
  }

  SECTION("zero tuple dimension empties the store regardless of dimension order")
  {
    // A zero dimension makes the tuple count zero, so an extent that would overflow a partial product is not an error.
    const usize maxExtent = (std::numeric_limits<usize>::max)();
    const ShapeType zeroTupleShape = GENERATE_COPY(ShapeType{0}, ShapeType{maxExtent, 2, 0}, ShapeType{0, maxExtent, 2});
    INFO(fmt::format("Tuple shape: [{}]", fmt::join(zeroTupleShape, ", ")));

    DataStore<int32> dataStore(ShapeType{4}, ShapeType{1}, 7);
    const std::array<int32, 4> initialValues = {10, 20, 30, 40};
    for(usize valueIndex = 0; valueIndex < initialValues.size(); ++valueIndex)
    {
      dataStore[valueIndex] = initialValues[valueIndex];
    }

    const Result<> resizeResult = dataStore.resizeTuples(zeroTupleShape);
    SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
    REQUIRE(dataStore.getTupleShape() == zeroTupleShape);
    REQUIRE(dataStore.getComponentShape() == ShapeType{1});
    REQUIRE(dataStore.getNumberOfTuples() == 0);
    REQUIRE(dataStore.getNumberOfComponents() == 1);
    REQUIRE(dataStore.getSize() == 0);
  }
}

TEST_CASE("DataStore caller-owned extent buffers preserve values and validate before writing", "[simplnx][DataStore]")
{
  DataStore<int32> dataStore(ShapeType{2, 3, 4}, ShapeType{2}, 0);
  for(usize tupleIndex = 0; tupleIndex < dataStore.getNumberOfTuples(); ++tupleIndex)
  {
    dataStore[tupleIndex * 2] = static_cast<int32>(tupleIndex * 10);
    dataStore[tupleIndex * 2 + 1] = static_cast<int32>(tupleIndex * 10 + 1);
  }

  const Extent mainExtent({0, 0, 1}, {1, 2, 3}, {1, 2, 1});
  const std::vector<int32> expectedMain = {10, 11, 20, 21, 30, 31, 90, 91, 100, 101, 110, 111, 130, 131, 140, 141, 150, 151, 210, 211, 220, 221, 230, 231};
  std::vector<int32> mainValues(expectedMain.size(), -1);
  REQUIRE(dataStore.readExtentIntoBuffer(mainExtent, nonstd::span<int32>(mainValues.data(), mainValues.size())).valid());
  REQUIRE(mainValues == expectedMain);

  const Extent faceExtent({1, 1, 0}, {1, 2, 3});
  const std::vector<int32> expectedFace = {160, 161, 170, 171, 180, 181, 190, 191, 200, 201, 210, 211, 220, 221, 230, 231};
  std::vector<int32> multiMain(expectedMain.size(), -1);
  std::vector<int32> faceValues(expectedFace.size(), -1);
  std::array<Extent, 2> extents = {mainExtent, faceExtent};
  std::array<nonstd::span<int32>, 2> destinations = {
      nonstd::span<int32>(multiMain.data(), multiMain.size()),
      nonstd::span<int32>(faceValues.data(), faceValues.size()),
  };
  REQUIRE(dataStore.readExtentsIntoBuffers(nonstd::span<const Extent>(extents.data(), extents.size()), nonstd::span<nonstd::span<int32>>(destinations.data(), destinations.size())).valid());
  REQUIRE(multiMain == expectedMain);
  REQUIRE(faceValues == expectedFace);

  std::vector<int32> countMismatchValues(expectedMain.size(), -777);
  std::array<nonstd::span<int32>, 1> countMismatchDestination = {nonstd::span<int32>(countMismatchValues.data(), countMismatchValues.size())};
  Result<> countMismatchResult =
      dataStore.readExtentsIntoBuffers(nonstd::span<const Extent>(extents.data(), extents.size()), nonstd::span<nonstd::span<int32>>(countMismatchDestination.data(), countMismatchDestination.size()));
  REQUIRE(countMismatchResult.invalid());
  REQUIRE(countMismatchResult.errors()[0].code == -6034);
  REQUIRE(countMismatchValues == std::vector<int32>(expectedMain.size(), -777));

  std::vector<int32> shortMain(expectedMain.size() - 1, -777);
  std::vector<int32> untouchedFace(expectedFace.size(), -777);
  std::array<nonstd::span<int32>, 2> sizeMismatchDestinations = {
      nonstd::span<int32>(shortMain.data(), shortMain.size()),
      nonstd::span<int32>(untouchedFace.data(), untouchedFace.size()),
  };
  Result<> sizeMismatchResult =
      dataStore.readExtentsIntoBuffers(nonstd::span<const Extent>(extents.data(), extents.size()), nonstd::span<nonstd::span<int32>>(sizeMismatchDestinations.data(), sizeMismatchDestinations.size()));
  REQUIRE(sizeMismatchResult.invalid());
  REQUIRE(sizeMismatchResult.errors()[0].code == -6034);
  REQUIRE(shortMain == std::vector<int32>(expectedMain.size() - 1, -777));
  REQUIRE(untouchedFace == std::vector<int32>(expectedFace.size(), -777));
}

TEST_CASE("DataStore caller-owned extent buffers support two-dimensional stores", "[simplnx][DataStore]")
{
  DataStore<int32> dataStore(ShapeType{3, 4}, ShapeType{1}, 0);
  for(usize valueIndex = 0; valueIndex < dataStore.getSize(); ++valueIndex)
  {
    dataStore[valueIndex] = static_cast<int32>(valueIndex);
  }

  const Extent extent({0, 1}, {2, 3}, {2, 1});
  std::vector<int32> values(6, -1);
  REQUIRE(dataStore.readExtentIntoBuffer(extent, nonstd::span<int32>(values.data(), values.size())).valid());
  REQUIRE(values == std::vector<int32>{1, 2, 3, 9, 10, 11});
}

TEST_CASE("DataStore::writeExtent rejects unsupported rank instead of silently succeeding", "[DataStore]")
{
  DataStore<int32> store(ShapeType{10, 10}, ShapeType{1}, 0);
  std::vector<int32> data(4, 7);
  Extent extent(std::vector<uint64>{0, 0}, std::vector<uint64>{1, 1}, std::vector<uint64>{1, 1});
  Result<> result = store.writeExtent(extent, data);
  REQUIRE(result.invalid());
  REQUIRE(result.errors()[0].code == -6037);
}

TEST_CASE("AttributeMatrix::resizeTuples reports the failing child", "[AttributeMatrix]")
{
  DataStructure dataStructure;
  auto* attributeMatrixPtr = AttributeMatrix::Create(dataStructure, "AM", ShapeType{4});
  REQUIRE(attributeMatrixPtr != nullptr);
  REQUIRE(DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "first", ShapeType{4}, ShapeType{1}, attributeMatrixPtr->getId()) != nullptr);
  REQUIRE(DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "second", ShapeType{4}, ShapeType{1}, attributeMatrixPtr->getId()) != nullptr);

  const auto children = attributeMatrixPtr->findAllChildrenOfType<IArray>();
  REQUIRE(children.size() == 2);
  auto childIterator = children.begin();
  auto* goodPtr = dynamic_cast<DataArray<int32>*>(childIterator->get());
  REQUIRE(goodPtr != nullptr);
  ++childIterator;
  auto* badPtr = dynamic_cast<DataArray<int32>*>(childIterator->get());
  REQUIRE(badPtr != nullptr);
  auto badSetStoreResult = badPtr->setDataStore(std::make_shared<FailingResizeDataStore>(ShapeType{4}, ShapeType{1}, 0));
  SIMPLNX_RESULT_REQUIRE_VALID(badSetStoreResult);
  const std::string badName = badPtr->getName();

  Result<> result = attributeMatrixPtr->resizeTuples(ShapeType{8});

  REQUIRE(result.invalid());
  REQUIRE(result.errors()[0].message.find(badName) != std::string::npos);
  REQUIRE(goodPtr->getNumberOfTuples() == 8);
}

TEST_CASE("Copy DataStore", "DataArray")
{
  ShapeType tupleShape{5};
  ShapeType componentShape{3};
  DataStore<int32> dataStore(tupleShape, componentShape, 5);
  usize size = dataStore.getSize();
  for(usize i = 0; i < size; i++)
  {
    dataStore[i] = i;
  }

  DataStore<int32> dataStore2(tupleShape, componentShape, 5);
  dataStore2.copy(dataStore);

  for(usize i = 0; i < size; i++)
  {
    REQUIRE(dataStore[i] == dataStore2[i]);
  }
}

template <typename T>
static void TestDataArrayToFromString(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize k_Index1 = 5;
  constexpr usize k_Index2 = 6;
  constexpr usize k_Index3 = 7;
  constexpr usize k_Index4 = 8;
  constexpr usize k_Index5 = 9;
  constexpr usize k_Index6 = 10;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* dataArrayPtr = UnitTest::CreateTestDataArray<T>(datastructure, arrayName, tupleShape, compShape);
  constexpr T k_MinVal = std::numeric_limits<T>::min();
  constexpr T k_MaxVal = std::numeric_limits<T>::max();
  dataArrayPtr->setValue(k_Index2, k_MinVal);
  dataArrayPtr->setValue(k_Index3, k_MaxVal);
  const std::string minStr = dataArrayPtr->toString(k_Index2, 0);
  const std::string maxStr = dataArrayPtr->toString(k_Index3, 0);
  // std::cout << "DataArray<T>\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n";
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(dataArrayPtr->setValueFromString(k_Index5, 0, minStr));
  REQUIRE(dataArrayPtr->setValueFromString(k_Index6, 0, maxStr));
  // std::cout << "DataArray<T>\n\tmin : " << dataArray->getValue(index5) << " == " << minVal << "\n\tmax : " << dataArray->getValue(index6) << " == " << maxVal << "\n";
  REQUIRE(dataArrayPtr->getValue(k_Index5) == k_MinVal);
  REQUIRE(dataArrayPtr->getValue(k_Index6) == k_MaxVal);
}

template <>
void TestDataArrayToFromString<float32>(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize k_Index1 = 5;
  constexpr usize k_Index2 = 6;
  constexpr usize k_Index3 = 7;
  constexpr usize k_Index4 = 8;
  constexpr usize k_Index5 = 9;
  constexpr usize k_Index6 = 10;
  constexpr usize k_Index7 = 11;
  constexpr usize k_Index8 = 12;
  constexpr usize k_Index9 = 13;
  constexpr usize k_Index10 = 14;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* dataArrayPtr = UnitTest::CreateTestDataArray<float32>(datastructure, arrayName, tupleShape, compShape);
  constexpr float32 k_LowestVal = std::numeric_limits<float32>::lowest();
  constexpr float32 k_MinVal = std::numeric_limits<float32>::min();
  constexpr float32 k_MaxVal = std::numeric_limits<float32>::max();
  constexpr float32 k_InfVal = std::numeric_limits<float32>::infinity();
  const float32 nanVal = std::nanf("1");
  dataArrayPtr->setValue(k_Index1, k_LowestVal);
  dataArrayPtr->setValue(k_Index2, k_MinVal);
  dataArrayPtr->setValue(k_Index3, k_MaxVal);
  dataArrayPtr->setValue(k_Index4, k_InfVal);
  dataArrayPtr->setValue(k_Index5, nanVal);
  const std::string lowestStr = dataArrayPtr->toString(k_Index1, 0);
  const std::string minStr = dataArrayPtr->toString(k_Index2, 0);
  const std::string maxStr = dataArrayPtr->toString(k_Index3, 0);
  const std::string infStr = dataArrayPtr->toString(k_Index4, 0);
  const std::string nanStr = dataArrayPtr->toString(k_Index5, 0);
  // std::cout << "DataArray<float32>\n\tlowest = " << lowestStr << "\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n\tinf = " << infStr << "\n\tnan = " << nanStr << "\n";
  REQUIRE(lowestStr == "-3.4028235e+38");
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(infStr == "inf");
  REQUIRE(nanStr == "nan");
  REQUIRE(dataArrayPtr->toString(k_Index2, 0, "{:.8g}") == "1.1754944e-38");
  REQUIRE(dataArrayPtr->toString(k_Index3, 0, "{:.6f}") == "340282346638528859811704183484516925440.000000");
  REQUIRE(dataArrayPtr->toString(k_Index2, 0, "{:.4e}") == "1.1755e-38");
  REQUIRE(dataArrayPtr->setValueFromString(k_Index6, 0, lowestStr));
  REQUIRE(dataArrayPtr->setValueFromString(k_Index7, 0, minStr));
  REQUIRE(dataArrayPtr->setValueFromString(k_Index8, 0, maxStr));
  REQUIRE(dataArrayPtr->setValueFromString(k_Index9, 0, infStr));
  REQUIRE(dataArrayPtr->setValueFromString(k_Index10, 0, nanStr));
  // std::cout << "DataArray<float32>\n\tlowest : " << dataArray->getValue(index6) << " == " << lowestVal << "\n\tmin : " << dataArray->getValue(index7) << " == " << minVal
  //<< "\n\tmax : " << dataArray->getValue(index8) << " == " << maxVal << "\n\tinf : " << dataArray->getValue(index9) << " == " << infVal << "\n\tnan : " << dataArray->getValue(index10)
  //<< " == " << nanVal << "\n";
  REQUIRE(dataArrayPtr->getValue(k_Index6) == k_LowestVal);
  REQUIRE(dataArrayPtr->getValue(k_Index7) == k_MinVal);
  REQUIRE(dataArrayPtr->getValue(k_Index8) == k_MaxVal);
  REQUIRE(dataArrayPtr->getValue(k_Index9) == k_InfVal);
  REQUIRE(std::isnan(dataArrayPtr->getValue(k_Index10)));
}
template <>
void TestDataArrayToFromString<float64>(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize k_Index1 = 5;
  constexpr usize k_Index2 = 6;
  constexpr usize k_Index3 = 7;
  constexpr usize k_Index4 = 8;
  constexpr usize k_Index5 = 9;
  constexpr usize k_Index6 = 10;
  constexpr usize k_Index7 = 11;
  constexpr usize k_Index8 = 12;
  constexpr usize k_Index9 = 13;
  constexpr usize k_Index10 = 14;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* dataArrayPtr = UnitTest::CreateTestDataArray<float64>(datastructure, arrayName, tupleShape, compShape);
  constexpr float64 k_LowestVal = std::numeric_limits<float64>::lowest();
  constexpr float64 k_MinVal = std::numeric_limits<float64>::min();
  constexpr float64 k_MaxVal = std::numeric_limits<float64>::max();
  constexpr float64 k_InfVal = std::numeric_limits<float64>::infinity();
  const float64 nanVal = std::nan("1");
  dataArrayPtr->setValue(k_Index1, k_LowestVal);
  dataArrayPtr->setValue(k_Index2, k_MinVal);
  dataArrayPtr->setValue(k_Index3, k_MaxVal);
  dataArrayPtr->setValue(k_Index4, k_InfVal);
  dataArrayPtr->setValue(k_Index5, nanVal);
  const std::string lowestStr = dataArrayPtr->toString(k_Index1, 0);
  const std::string minStr = dataArrayPtr->toString(k_Index2, 0);
  const std::string maxStr = dataArrayPtr->toString(k_Index3, 0);
  const std::string infStr = dataArrayPtr->toString(k_Index4, 0);
  const std::string nanStr = dataArrayPtr->toString(k_Index5, 0);
  // std::cout << "DataArray<float64>\n\tlowest = " << lowestStr << "\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n\tinf = " << infStr << "\n\tnan = " << nanStr << "\n";
  REQUIRE(lowestStr == "-1.7976931348623157e+308");
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(infStr == "inf");
  REQUIRE(nanStr == "nan");
  REQUIRE(dataArrayPtr->toString(k_Index2, 0, "{:.8g}") == "2.2250739e-308");
  REQUIRE(dataArrayPtr->toString(k_Index2, 0, "{:.6f}") == "0.000000");
  REQUIRE(dataArrayPtr->toString(k_Index2, 0, "{:.4e}") == "2.2251e-308");
  REQUIRE(dataArrayPtr->setValueFromString(k_Index6, 0, lowestStr));
  REQUIRE(dataArrayPtr->setValueFromString(k_Index7, 0, minStr));
  REQUIRE(dataArrayPtr->setValueFromString(k_Index8, 0, maxStr));
  REQUIRE(dataArrayPtr->setValueFromString(k_Index9, 0, infStr));
  REQUIRE(dataArrayPtr->setValueFromString(k_Index10, 0, nanStr));
  // std::cout << "DataArray<float64>\n\tlowest : " << dataArray->getValue(index6) << " == " << lowestVal << "\n\tmin : " << dataArray->getValue(index7) << " == " << minVal
  //<< "\n\tmax : " << dataArray->getValue(index8) << " == " << maxVal << "\n\tinf : " << dataArray->getValue(index9) << " == " << infVal << "\n\tnan : " << dataArray->getValue(index10)
  //<< " == " << nanVal << "\n";
  REQUIRE(dataArrayPtr->getValue(k_Index6) == k_LowestVal);
  REQUIRE(dataArrayPtr->getValue(k_Index7) == k_MinVal);
  REQUIRE(dataArrayPtr->getValue(k_Index8) == k_MaxVal);
  REQUIRE(dataArrayPtr->getValue(k_Index9) == k_InfVal);
  REQUIRE(std::isnan(dataArrayPtr->getValue(k_Index10)));
}

template <typename T>
static void TestNeighborListArrayToFromString(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize k_Index1 = 5;
  constexpr usize k_Index2 = 6;
  constexpr usize k_Index3 = 7;
  constexpr usize k_Index4 = 8;
  constexpr usize k_Index5 = 9;
  constexpr usize k_Index6 = 10;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* neighborListArrayPtr = UnitTest::CreateTestNeighborList<T>(datastructure, arrayName, 10, {});
  constexpr T k_MinVal = std::numeric_limits<T>::min();
  constexpr T k_MaxVal = std::numeric_limits<T>::max();
  neighborListArrayPtr->addEntry(k_Index2, k_MinVal);
  neighborListArrayPtr->addEntry(k_Index3, k_MaxVal);
  neighborListArrayPtr->addEntry(k_Index5, 0);
  neighborListArrayPtr->addEntry(k_Index6, 0);
  const std::string minStr = neighborListArrayPtr->toString(k_Index2, 0);
  const std::string maxStr = neighborListArrayPtr->toString(k_Index3, 0);
  // std::cout << "NeighborListArray<T>\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n";
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(neighborListArrayPtr->setValueFromString(k_Index5, 0, minStr));
  REQUIRE(neighborListArrayPtr->setValueFromString(k_Index6, 0, maxStr));
  bool ok = false;
  // std::cout << "NeighborListArray<T>\n\tmin : " << neighborListArray->getValue(index5, 0, ok) << " == " << minVal << "\n\tmax : " << neighborListArray->getValue(index6, 0, ok) << " == " << maxVal
  //<< "\n";
  REQUIRE(neighborListArrayPtr->getValue(k_Index5, 0, ok) == k_MinVal);
  REQUIRE(ok);
  REQUIRE(neighborListArrayPtr->getValue(k_Index6, 0, ok) == k_MaxVal);
  REQUIRE(ok);
}

template <>
void TestNeighborListArrayToFromString<float32>(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize k_Index1 = 5;
  constexpr usize k_Index2 = 6;
  constexpr usize k_Index3 = 7;
  constexpr usize k_Index4 = 8;
  constexpr usize k_Index5 = 9;
  constexpr usize k_Index6 = 10;
  constexpr usize k_Index7 = 11;
  constexpr usize k_Index8 = 12;
  constexpr usize k_Index9 = 13;
  constexpr usize k_Index10 = 14;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* neighborListFloat32Ptr = UnitTest::CreateTestNeighborList<float32>(datastructure, arrayName, 10, {});
  constexpr float32 k_LowestVal = std::numeric_limits<float32>::lowest();
  constexpr float32 k_MinVal = std::numeric_limits<float32>::min();
  constexpr float32 k_MaxVal = std::numeric_limits<float32>::max();
  constexpr float32 k_InfVal = std::numeric_limits<float32>::infinity();
  const float32 nanVal = std::nanf("1");
  neighborListFloat32Ptr->addEntry(k_Index1, k_LowestVal);
  neighborListFloat32Ptr->addEntry(k_Index2, k_MinVal);
  neighborListFloat32Ptr->addEntry(k_Index3, k_MaxVal);
  neighborListFloat32Ptr->addEntry(k_Index4, k_InfVal);
  neighborListFloat32Ptr->addEntry(k_Index5, nanVal);
  neighborListFloat32Ptr->addEntry(k_Index6, 0);
  neighborListFloat32Ptr->addEntry(k_Index7, 0);
  neighborListFloat32Ptr->addEntry(k_Index8, 0);
  neighborListFloat32Ptr->addEntry(k_Index9, 0);
  neighborListFloat32Ptr->addEntry(k_Index10, 0);
  const std::string lowestStr = neighborListFloat32Ptr->toString(k_Index1, 0);
  const std::string minStr = neighborListFloat32Ptr->toString(k_Index2, 0);
  const std::string maxStr = neighborListFloat32Ptr->toString(k_Index3, 0);
  const std::string infStr = neighborListFloat32Ptr->toString(k_Index4, 0);
  const std::string nanStr = neighborListFloat32Ptr->toString(k_Index5, 0);
  // std::cout << "NeighborListArray<float32>\n\tlowest = " << lowestStr << "\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n\tinf = " << infStr << "\n\tnan = " << nanStr << "\n";
  REQUIRE(lowestStr == "-3.4028235e+38");
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(infStr == "inf");
  REQUIRE(nanStr == "nan");
  REQUIRE(neighborListFloat32Ptr->toString(k_Index2, 0, "{:.8g}") == "1.1754944e-38");
  REQUIRE(neighborListFloat32Ptr->toString(k_Index3, 0, "{:.6f}") == "340282346638528859811704183484516925440.000000");
  REQUIRE(neighborListFloat32Ptr->toString(k_Index2, 0, "{:.4e}") == "1.1755e-38");
  REQUIRE(neighborListFloat32Ptr->setValueFromString(k_Index6, 0, lowestStr));
  REQUIRE(neighborListFloat32Ptr->setValueFromString(k_Index7, 0, minStr));
  REQUIRE(neighborListFloat32Ptr->setValueFromString(k_Index8, 0, maxStr));
  REQUIRE(neighborListFloat32Ptr->setValueFromString(k_Index9, 0, infStr));
  REQUIRE(neighborListFloat32Ptr->setValueFromString(k_Index10, 0, nanStr));
  // std::cout << "NeighborListArray<float32>\n\tlowest : " << neighborListFloat32->getValue(index6) << " == " << lowestVal << "\n\tmin : " << neighborListFloat32->getValue(index7) << " == " << minVal
  //<< "\n\tmax : " << neighborListFloat32->getValue(index8) << " == " << maxVal << "\n\tinf : " << neighborListFloat32->getValue(index9) << " == " << infVal << "\n\tnan : " <<
  // neighborListFloat32->getValue(index10) << " == " << nanVal << "\n";
  bool ok = false;
  REQUIRE(neighborListFloat32Ptr->getValue(k_Index6, 0, ok) == k_LowestVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat32Ptr->getValue(k_Index7, 0, ok) == k_MinVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat32Ptr->getValue(k_Index8, 0, ok) == k_MaxVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat32Ptr->getValue(k_Index9, 0, ok) == k_InfVal);
  REQUIRE(ok);
  REQUIRE(std::isnan(neighborListFloat32Ptr->getValue(k_Index10, 0, ok)));
  REQUIRE(ok);
}
template <>
void TestNeighborListArrayToFromString<float64>(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize k_Index1 = 5;
  constexpr usize k_Index2 = 6;
  constexpr usize k_Index3 = 7;
  constexpr usize k_Index4 = 8;
  constexpr usize k_Index5 = 9;
  constexpr usize k_Index6 = 10;
  constexpr usize k_Index7 = 11;
  constexpr usize k_Index8 = 12;
  constexpr usize k_Index9 = 13;
  constexpr usize k_Index10 = 14;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* neighborListFloat64Ptr = UnitTest::CreateTestNeighborList<float64>(datastructure, arrayName, 10, {});
  constexpr float64 k_LowestVal = std::numeric_limits<float64>::lowest();
  constexpr float64 k_MinVal = std::numeric_limits<float64>::min();
  constexpr float64 k_MaxVal = std::numeric_limits<float64>::max();
  constexpr float64 k_InfVal = std::numeric_limits<float64>::infinity();
  const float64 nanVal = std::nan("1");
  neighborListFloat64Ptr->addEntry(k_Index1, k_LowestVal);
  neighborListFloat64Ptr->addEntry(k_Index2, k_MinVal);
  neighborListFloat64Ptr->addEntry(k_Index3, k_MaxVal);
  neighborListFloat64Ptr->addEntry(k_Index4, k_InfVal);
  neighborListFloat64Ptr->addEntry(k_Index5, nanVal);
  neighborListFloat64Ptr->addEntry(k_Index6, 0);
  neighborListFloat64Ptr->addEntry(k_Index7, 0);
  neighborListFloat64Ptr->addEntry(k_Index8, 0);
  neighborListFloat64Ptr->addEntry(k_Index9, 0);
  neighborListFloat64Ptr->addEntry(k_Index10, 0);
  const std::string lowestStr = neighborListFloat64Ptr->toString(k_Index1, 0);
  const std::string minStr = neighborListFloat64Ptr->toString(k_Index2, 0);
  const std::string maxStr = neighborListFloat64Ptr->toString(k_Index3, 0);
  const std::string infStr = neighborListFloat64Ptr->toString(k_Index4, 0);
  const std::string nanStr = neighborListFloat64Ptr->toString(k_Index5, 0);
  // std::cout << "NeighborListArray<float64>\n\tlowest = " << lowestStr << "\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n\tinf = " << infStr << "\n\tnan = " << nanStr << "\n";
  REQUIRE(lowestStr == "-1.7976931348623157e+308");
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(infStr == "inf");
  REQUIRE(nanStr == "nan");
  REQUIRE(neighborListFloat64Ptr->toString(k_Index2, 0, "{:.8g}") == "2.2250739e-308");
  REQUIRE(neighborListFloat64Ptr->toString(k_Index2, 0, "{:.6f}") == "0.000000");
  REQUIRE(neighborListFloat64Ptr->toString(k_Index2, 0, "{:.4e}") == "2.2251e-308");
  REQUIRE(neighborListFloat64Ptr->setValueFromString(k_Index6, 0, lowestStr));
  REQUIRE(neighborListFloat64Ptr->setValueFromString(k_Index7, 0, minStr));
  REQUIRE(neighborListFloat64Ptr->setValueFromString(k_Index8, 0, maxStr));
  REQUIRE(neighborListFloat64Ptr->setValueFromString(k_Index9, 0, infStr));
  REQUIRE(neighborListFloat64Ptr->setValueFromString(k_Index10, 0, nanStr));
  // std::cout << "NeighborListArray<float64>\n\tlowest : " << neighborListFloat64->getValue(index6) << " == " << lowestVal << "\n\tmin : " << neighborListFloat64->getValue(index7) << " == " << minVal
  //<< "\n\tmax : " << neighborListFloat64->getValue(index8) << " == " << maxVal << "\n\tinf : " << neighborListFloat64->getValue(index9) << " == " << infVal << "\n\tnan : " <<
  // neighborListFloat64->getValue(index10) << " == " << nanVal << "\n";
  bool ok = false;
  REQUIRE(neighborListFloat64Ptr->getValue(k_Index6, 0, ok) == k_LowestVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat64Ptr->getValue(k_Index7, 0, ok) == k_MinVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat64Ptr->getValue(k_Index8, 0, ok) == k_MaxVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat64Ptr->getValue(k_Index9, 0, ok) == k_InfVal);
  REQUIRE(ok);
  REQUIRE(std::isnan(neighborListFloat64Ptr->getValue(k_Index10, 0, ok)));
  REQUIRE(ok);
}

TEST_CASE("IArray ToFromString")
{
  DataStructure datastructure;

  SECTION("DataArray<int8>")
  {
    TestDataArrayToFromString<int8>(datastructure, "Int8Array", "-128", "127");
  }
  SECTION("DataArray<uint8>")
  {
    TestDataArrayToFromString<uint8>(datastructure, "UInt8Array", "0", "255");
  }
  SECTION("DataArray<int16>")
  {
    TestDataArrayToFromString<int16>(datastructure, "Int16Array", "-32768", "32767");
  }
  SECTION("DataArray<uint16>")
  {
    TestDataArrayToFromString<uint16>(datastructure, "UInt16Array", "0", "65535");
  }
  SECTION("DataArray<int32>")
  {
    TestDataArrayToFromString<int32>(datastructure, "Int32Array", "-2147483648", "2147483647");
  }
  SECTION("DataArray<uint32>")
  {
    TestDataArrayToFromString<uint32>(datastructure, "UInt32Array", "0", "4294967295");
  }
  SECTION("DataArray<int64>")
  {
    TestDataArrayToFromString<int64>(datastructure, "Int64Array", "-9223372036854775808", "9223372036854775807");
  }
  SECTION("DataArray<uint64>")
  {
    TestDataArrayToFromString<uint64>(datastructure, "UInt64Array", "0", "18446744073709551615");
  }
  SECTION("DataArray<float32>")
  {
    TestDataArrayToFromString<float32>(datastructure, "Float32Array", "1.1754944e-38", "3.4028235e+38");
  }
  SECTION("DataArray<float64>")
  {
    TestDataArrayToFromString<float64>(datastructure, "Float64Array", "2.2250738585072014e-308", "1.7976931348623157e+308");
  }
  SECTION("DataArray<bool>")
  {
    TestDataArrayToFromString<bool>(datastructure, "BoolArray", "false", "true");
  }

  SECTION("NeighborList<int8>")
  {
    TestNeighborListArrayToFromString<int8>(datastructure, "Int8NeighborListArray", "-128", "127");
  }
  SECTION("NeighborList<uint8>")
  {
    TestNeighborListArrayToFromString<uint8>(datastructure, "UInt8NeighborListArray", "0", "255");
  }
  SECTION("NeighborList<int16>")
  {
    TestNeighborListArrayToFromString<int16>(datastructure, "Int16NeighborListArray", "-32768", "32767");
  }
  SECTION("NeighborList<uint16>")
  {
    TestNeighborListArrayToFromString<uint16>(datastructure, "UInt16NeighborListArray", "0", "65535");
  }
  SECTION("NeighborList<int32>")
  {
    TestNeighborListArrayToFromString<int32>(datastructure, "Int32NeighborListArray", "-2147483648", "2147483647");
  }
  SECTION("NeighborList<uint32>")
  {
    TestNeighborListArrayToFromString<uint32>(datastructure, "UInt32NeighborListArray", "0", "4294967295");
  }
  SECTION("NeighborList<int64>")
  {
    TestNeighborListArrayToFromString<int64>(datastructure, "Int64NeighborListArray", "-9223372036854775808", "9223372036854775807");
  }
  SECTION("NeighborList<uint64>")
  {
    TestNeighborListArrayToFromString<uint64>(datastructure, "UInt64NeighborListArray", "0", "18446744073709551615");
  }
  SECTION("NeighborList<float32>")
  {
    TestNeighborListArrayToFromString<float32>(datastructure, "Float32NeighborListArray", "1.1754944e-38", "3.4028235e+38");
  }
  SECTION("NeighborList<float64>")
  {
    TestNeighborListArrayToFromString<float64>(datastructure, "Float64NeighborListArray", "2.2250738585072014e-308", "1.7976931348623157e+308");
  }

  SECTION("StringArray")
  {
    constexpr usize k_Index1 = 5;
    constexpr usize k_Index2 = 6;
    constexpr usize k_Index3 = 7;
    constexpr usize k_Index4 = 8;
    constexpr usize k_Index5 = 9;
    constexpr usize k_Index6 = 10;
    const ShapeType tupleShape = {5, 4, 3};
    std::vector<std::string> values(60, "This is a string");
    StringArray* stringArrayPtr = StringArray::CreateWithValues(datastructure, "StringArray", tupleShape, values, {});
    REQUIRE(stringArrayPtr->setValueFromString(k_Index1, 0, "AllLetters"));
    REQUIRE(stringArrayPtr->setValueFromString(k_Index2, 0, "MixedLetters&Numbers"));
    REQUIRE(stringArrayPtr->setValueFromString(k_Index3, 0, "234523"));
    REQUIRE(stringArrayPtr->setValueFromString(k_Index4, 0, "45.78"));
    REQUIRE(stringArrayPtr->setValueFromString(k_Index5, 0, "inf"));
    REQUIRE(stringArrayPtr->setValueFromString(k_Index6, 0, "nan"));
    REQUIRE(stringArrayPtr->toString(k_Index1, 0) == "AllLetters");
    REQUIRE(stringArrayPtr->toString(k_Index2, 0) == "MixedLetters&Numbers");
    REQUIRE(stringArrayPtr->toString(k_Index3, 0) == "234523");
    REQUIRE(stringArrayPtr->toString(k_Index4, 0) == "45.78");
    REQUIRE(stringArrayPtr->toString(k_Index5, 0) == "inf");
    REQUIRE(stringArrayPtr->toString(k_Index6, 0) == "nan");
  }
}

TEMPLATE_TEST_CASE("Numeric and list stores require an explicit copy destination format", "[simplnx][DataStore][DeepCopy][StoreCopyContract]", bool, int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64, float32, float64)
{
  CheckStoreCopyContract<IDataStore>();
  CheckStoreCopyContract<AbstractDataStore<TestType>>();
  CheckStoreCopyContract<DataStore<TestType>>();
  CheckStoreCopyContract<EmptyDataStore<TestType>>();
  CheckStoreCopyContract<AbstractListStore<TestType>>();
  CheckStoreCopyContract<ListStore<TestType>>();
  CheckStoreCopyContract<EmptyListStore<TestType>>();
}

TEMPLATE_TEST_CASE("String stores require an explicit copy destination format", "[simplnx][DataStore][DeepCopy][StoreCopyContract]", AbstractStringStore, StringStore, EmptyStringStore)
{
  constexpr bool k_AcceptsFormat = requires(const TestType& store, const std::string& format) { store.deepCopy(format); };
  constexpr bool k_AcceptsNoFormat = requires(const TestType& store) { store.deepCopy(); };
  CHECK(k_AcceptsFormat);
  CHECK_FALSE(k_AcceptsNoFormat);
}

namespace
{
/**
 * @struct StoreCopyProbe
 * @brief Records transfer sizes and destination cleanup for deterministic copy failures.
 */
struct StoreCopyProbe
{
  std::vector<usize> read_sizes;
  std::vector<usize> write_sizes;
  usize fail_read_call = 0;
  usize fail_write_call = 0;
  usize destroyed_destinations = 0;
};

/**
 * @class ProbedCopyStore
 * @brief Supplies a synthetic named backend and observes bulk copy operations.
 */
class ProbedCopyStore : public DataStore<int32>
{
public:
  /**
   * @brief Creates resident test values behind an optional synthetic backend identity.
   * @param tuples Tuple dimensions.
   * @param components Component dimensions.
   * @param probe Shared sequential test observations and failure settings.
   * @param format Synthetic backend identifier, or empty for memory.
   * @param destination True when destruction must update the cleanup witness.
   */
  ProbedCopyStore(const ShapeType& tuples, const ShapeType& components, std::shared_ptr<StoreCopyProbe> probe, std::string format = "", bool destination = false)
  : DataStore<int32>(tuples, components, 0)
  , m_Probe(std::move(probe))
  , m_Format(std::move(format))
  , m_Destination(destination)
  {
  }

  /**
   * @brief Records destruction of a factory-created destination.
   */
  ~ProbedCopyStore() override
  {
    if(m_Destination)
    {
      ++m_Probe->destroyed_destinations;
    }
  }

  std::string getDataFormat() const override
  {
    return m_Format;
  }

  StoreType getStoreType() const override
  {
    return m_Format.empty() ? StoreType::InMemory : StoreType::OutOfCore;
  }

  /**
   * @brief Records a read and optionally injects a late failure.
   * @param startIndex First source value.
   * @param buffer Caller-owned destination values.
   * @return DataStore result, or injected error -9871.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<int32> buffer) const override
  {
    m_Probe->read_sizes.push_back(buffer.size());
    if(m_Probe->read_sizes.size() == m_Probe->fail_read_call)
    {
      return MakeErrorResult(-9871, "injected late source read failure");
    }
    return DataStore<int32>::copyIntoBuffer(startIndex, buffer);
  }

  /**
   * @brief Records a write and optionally injects a late failure.
   * @param startIndex First destination value.
   * @param buffer Caller-owned source values.
   * @return DataStore result, or injected error -9872.
   */
  Result<> copyFromBuffer(usize startIndex, nonstd::span<const int32> buffer) override
  {
    m_Probe->write_sizes.push_back(buffer.size());
    if(m_Probe->write_sizes.size() == m_Probe->fail_write_call)
    {
      return MakeErrorResult(-9872, "injected late destination write failure");
    }
    return DataStore<int32>::copyFromBuffer(startIndex, buffer);
  }

private:
  std::shared_ptr<StoreCopyProbe> m_Probe;
  std::string m_Format;
  bool m_Destination = false;
};

/**
 * @class CopyTestManager
 * @brief Registers synthetic numeric factories without replacing built-in factories.
 */
class CopyTestManager : public IDataIOManager
{
public:
  /**
   * @brief Registers one test factory with a distinct format name.
   * @param format Test-only format identifier.
   * @param factory Callback that creates or rejects a copy destination.
   */
  CopyTestManager(std::string format, DataStoreCreateFnc factory)
  : m_Format(std::move(format))
  {
    addDataStoreCreationFnc(m_Format, std::move(factory));
  }

  std::string formatName() const override
  {
    return m_Format;
  }

private:
  std::string m_Format;
};

/**
 * @class CountingMemoryManager
 * @brief Counts fallback memory allocations and delegates them to the saved core manager.
 */
class CountingMemoryManager : public IDataIOManager
{
public:
  /**
   * @brief Registers both supported memory spellings.
   * @param delegateManager Saved core manager that performs the allocation.
   * @param callCounter Shared allocation counter.
   */
  CountingMemoryManager(std::shared_ptr<IDataIOManager> delegateManager, std::shared_ptr<usize> callCounter)
  {
    auto factory = [savedManager = std::move(delegateManager), sharedCounter = std::move(callCounter)](DataType dataType, const ShapeType& tupleShape, const ShapeType& componentShape,
                                                                                                       const std::optional<ShapeType>& chunkShape, DataStoreInitializationMode initializationMode) {
      ++(*sharedCounter);
      return savedManager->dataStoreCreationFnc(Preferences::k_InMemoryFormat.str())(dataType, tupleShape, componentShape, chunkShape, initializationMode);
    };
    addDataStoreCreationFnc("", factory);
    addDataStoreCreationFnc(Preferences::k_InMemoryFormat.str(), std::move(factory));
  }

  std::string formatName() const override
  {
    return Preferences::k_InMemoryFormat.str();
  }
};

/**
 * @class CanonicalMemoryStore
 * @brief Provides resident values with the canonical memory format identity.
 */
class CanonicalMemoryStore : public DataStore<int32>
{
public:
  using DataStore<int32>::DataStore;

  std::string getDataFormat() const override
  {
    return Preferences::k_InMemoryFormat.str();
  }
};

/**
 * @class RecordingCopyStore
 * @brief Records the resolved format passed by an array-level copy.
 */
class RecordingCopyStore : public DataStore<int32>
{
public:
  using DataStore<int32>::DataStore;
  mutable std::vector<std::string> formats;

  /**
   * @brief Records destination policy before the normal store copy.
   * @param destinationFormat Resolved format supplied by the owning array.
   * @return Independent store from the normal deepCopy implementation.
   */
  std::unique_ptr<IDataStore> deepCopy(const std::string& destinationFormat) const override
  {
    formats.push_back(destinationFormat);
    return DataStore<int32>::deepCopy(destinationFormat);
  }
};

/**
 * @class RecordingCopyListStore
 * @brief Records the resolved format passed by NeighborList operations.
 */
class RecordingCopyListStore : public ListStore<int32>
{
public:
  using ListStore<int32>::ListStore;
  mutable std::vector<std::string> formats;

  /**
   * @brief Records destination policy before the normal store copy.
   * @param destinationFormat Resolved format supplied by the owning array.
   * @return Independent store from the normal deepCopy implementation.
   */
  std::unique_ptr<AbstractListStore<int32>> deepCopy(const std::string& destinationFormat) const override
  {
    formats.push_back(destinationFormat);
    return ListStore<int32>::deepCopy(destinationFormat);
  }
};

/**
 * @class CopyPathResolver
 * @brief Records sequential test requests and selects in-memory storage.
 *
 * Tests use this resolver only from the calling thread.
 */
class CopyPathResolver : public IDataStoreFormatResolver
{
public:
  mutable std::vector<DataPath> paths;
  mutable std::vector<uint64> bytes;
  std::string selected_format;
  std::string failure_message;
  bool throw_bad_allocation = false;

  /**
   * @brief Records destination context without reading source values.
   * @param dataStructure Unused destination owner.
   * @param path Destination array path.
   * @param numericType Unused value type.
   * @param sizeBytes Logical numeric bytes or the list lower-bound estimate.
   * @return Configured format, which defaults to memory.
   * @throws std::runtime_error If failureMessage is not empty.
   * @throws std::bad_alloc If throwBadAllocation is true.
   */
  std::string resolveFormat([[maybe_unused]] const DataStructure& dataStructure, const DataPath& path, [[maybe_unused]] DataType numericType, uint64 sizeBytes) const override
  {
    paths.push_back(path);
    bytes.push_back(sizeBytes);
    if(throw_bad_allocation)
    {
      throw std::bad_alloc();
    }
    if(!failure_message.empty())
    {
      throw std::runtime_error(failure_message);
    }
    return selected_format;
  }
};
} // namespace

TEMPLATE_TEST_CASE("Numeric store copies honor explicit memory and preserve independent values", "[simplnx][DataStore][DeepCopy]", bool, int8, uint8, int16, uint16, int32, uint32, int64, uint64,
                   float32, float64)
{
  for(const auto& format : std::vector<std::string>{"", Preferences::k_InMemoryFormat.str()})
  {
    DataStore<TestType> source(ShapeType{2, 3}, ShapeType{2}, TestType{1});
    auto destinationBase = source.deepCopy(format);
    auto* destinationPtr = dynamic_cast<AbstractDataStore<TestType>*>(destinationBase.get());
    REQUIRE(destinationPtr != nullptr);
    REQUIRE(destinationPtr->getStoreType() == IDataStore::StoreType::InMemory);
    REQUIRE(destinationPtr->getTupleShape() == ShapeType{2, 3});
    REQUIRE(destinationPtr->getComponentShape() == ShapeType{2});
    for(usize index = 0; index < 12; ++index)
    {
      REQUIRE(destinationPtr->getValue(index) == TestType{1});
    }
    destinationPtr->setValue(0, TestType{0});
    REQUIRE(source.getValue(0) == TestType{1});
    source.setValue(1, TestType{0});
    REQUIRE(destinationPtr->getValue(1) == TestType{1});

    auto placeholderResult = EmptyDataStore<TestType>::Create({2, 3}, {2}, "");
    SIMPLNX_RESULT_REQUIRE_VALID(placeholderResult);
    auto copiedPlaceholder = placeholderResult.value()->deepCopy(format);
    REQUIRE(copiedPlaceholder->getStoreType() == IDataStore::StoreType::Empty);
    REQUIRE(copiedPlaceholder->getPlannedStoreType() == IDataStore::StoreType::InMemory);
    REQUIRE(copiedPlaceholder->getDataFormat().empty());
    REQUIRE(copiedPlaceholder->getTupleShape() == ShapeType{2, 3});
    REQUIRE(copiedPlaceholder->getComponentShape() == ShapeType{2});
  }
}

TEST_CASE("Unknown store-copy formats obey the compiled build policy", "[simplnx][DataStore][DeepCopy]")
{
  const std::string unknown = "unavailable-store-copy-test-format";
  DataStore<int32> numeric({3}, {1}, 7);
  auto placeholderResult = EmptyDataStore<int32>::Create({3}, {1}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(placeholderResult);
  ListStore<int32> lists(ShapeType{3});
  lists.setList(0, std::vector<int32>{4, 5});
  StringStore strings({"first", "second"}, {2});
#if SIMPLNX_STORE_COPY_STRICT_FORMAT
  REQUIRE_THROWS_WITH(numeric.deepCopy(unknown), Catch::Contains(unknown));
  REQUIRE_THROWS_WITH(placeholderResult.value()->deepCopy(unknown), Catch::Contains(unknown));
  REQUIRE_THROWS_WITH(lists.deepCopy(unknown), Catch::Contains(unknown));
  REQUIRE_THROWS_WITH(strings.deepCopy(unknown), Catch::Contains(unknown));
#else
  auto numericCopy = numeric.deepCopy(unknown);
  REQUIRE(numericCopy->getStoreType() == IDataStore::StoreType::InMemory);
  auto placeholderCopy = placeholderResult.value()->deepCopy(unknown);
  REQUIRE(placeholderCopy->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(placeholderCopy->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(placeholderCopy->getDataFormat().empty());
  REQUIRE(lists.deepCopy(unknown)->getList(0) == std::vector<int32>{4, 5});
  REQUIRE(strings.deepCopy(unknown)->getValue(1) == "second");
#endif
}

TEST_CASE("Numeric store transfer bounds and late failures retain ownership", "[simplnx][DataStore][DeepCopy]")
{
  constexpr usize k_PageValues = 1024 * 1024 / sizeof(int32);
  const std::string format = "store-copy-probed-backend";
  auto probe = std::make_shared<StoreCopyProbe>();
  auto manager = std::make_shared<CopyTestManager>(format, [probe, format](DataType, const ShapeType& tuples, const ShapeType& components, const std::optional<ShapeType>&,
                                                                           DataStoreInitializationMode) { return std::make_unique<ProbedCopyStore>(tuples, components, probe, format, true); });
  auto managerRegistrationResult = Application::GetOrCreateInstance()->getIOCollection().addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(managerRegistrationResult);
  ProbedCopyStore source({2 * k_PageValues + 7}, {1}, probe);
  source.setValue(0, 19);
  source.setValue(2 * k_PageValues + 6, 23);

  SECTION("complete transfer includes a final partial page")
  {
    auto base = source.deepCopy(format);
    auto* copyPtr = dynamic_cast<AbstractDataStore<int32>*>(base.get());
    REQUIRE(copyPtr != nullptr);
    REQUIRE(copyPtr->getValue(0) == 19);
    REQUIRE(copyPtr->getValue(2 * k_PageValues + 6) == 23);
    REQUIRE(probe->read_sizes == std::vector<usize>{k_PageValues, k_PageValues, 7});
    REQUIRE(probe->write_sizes == probe->read_sizes);
    copyPtr->setValue(0, 3);
    REQUIRE(source.getValue(0) == 19);
    base.reset();
    REQUIRE(probe->destroyed_destinations == 1);
  }
  SECTION("late read failure preserves every diagnostic and destroys the destination")
  {
    probe->fail_read_call = 2;
    REQUIRE_THROWS_WITH(source.deepCopy(format), Catch::Contains(format) && Catch::Contains("-9871") && Catch::Contains("injected late source read failure"));
    REQUIRE(probe->write_sizes.size() == 1);
    REQUIRE(probe->destroyed_destinations == 1);
    REQUIRE(source.getValue(0) == 19);
  }
  SECTION("late write failure preserves every diagnostic and destroys the destination")
  {
    probe->fail_write_call = 2;
    REQUIRE_THROWS_WITH(source.deepCopy(format), Catch::Contains(format) && Catch::Contains("-9872") && Catch::Contains("injected late destination write failure"));
    REQUIRE(probe->read_sizes.size() == 2);
    REQUIRE(probe->destroyed_destinations == 1);
    REQUIRE(source.getValue(2 * k_PageValues + 6) == 23);
  }
}

TEST_CASE("Store-copy factories cannot return incompatible stores or hide failures", "[simplnx][DataStore][DeepCopy]")
{
  const std::string format = "store-copy-invalid-factory";
  const auto failure = GENERATE(0, 1, 2, 3, 4, 5);
  auto manager = std::make_shared<CopyTestManager>(
      format, [failure](DataType, const ShapeType& tuples, const ShapeType& components, const std::optional<ShapeType>&, DataStoreInitializationMode) -> std::unique_ptr<IDataStore> {
        switch(failure)
        {
        case 0:
          return nullptr;
        case 1:
          return std::make_unique<DataStore<float32>>(tuples, components, 0.0F);
        case 2:
          return std::make_unique<DataStore<int32>>(ShapeType{99}, components, 0);
        case 3:
          throw std::runtime_error("injected factory failure -9873");
        case 4:
          throw std::bad_alloc();
        default:
          return std::make_unique<DataStore<int32>>(tuples, components, 0);
        }
      });
  auto managerRegistrationResult = Application::GetOrCreateInstance()->getIOCollection().addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(managerRegistrationResult);
  DataStore<int32> source({3}, {2}, 17);
  if(failure == 4)
  {
    REQUIRE_THROWS_AS(source.deepCopy(format), std::bad_alloc);
  }
  else
  {
    REQUIRE_THROWS_WITH(source.deepCopy(format), Catch::Contains(format));
  }
  REQUIRE(source.getValue(0) == 17);
  DataStructure ds;
  auto resolver = std::make_shared<CopyPathResolver>();
  resolver->selected_format = format;
  ds.setFormatResolver(resolver);
  auto* arrayPtr = DataArray<int32>::Create(ds, "FailureSource", std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{2}, 17));
  REQUIRE(arrayPtr != nullptr);
  if(failure == 4)
  {
    REQUIRE_THROWS_AS(arrayPtr->deepCopy(DataPath({"FailureCopy"})), std::bad_alloc);
  }
  else
  {
    REQUIRE(arrayPtr->deepCopy(DataPath({"FailureCopy"})) == nullptr);
  }
  REQUIRE_FALSE(ds.containsData(DataPath({"FailureCopy"})));
}

TEST_CASE("StorageFormatPlan: array creation does not retry a failed selected factory", "[simplnx][DataArray][StorageFormatPlan]")
{
  const std::string format = "array-creation-invalid-factory";
  const auto failure = GENERATE(0, 1, 2, 3, 4, 5);
  auto selectedFactoryCalls = std::make_shared<usize>(0);
  auto fallbackMemoryFactoryCalls = std::make_shared<usize>(0);

  auto& collection = Application::GetOrCreateInstance()->getIOCollection();
  auto memoryEntry = std::find_if(collection.begin(), collection.end(), [](const auto& entry) { return entry.first == Preferences::k_InMemoryFormat.str(); });
  REQUIRE(memoryEntry != collection.end());
  const auto savedMemoryManager = memoryEntry->second;
  auto restoreMemoryManager = MakeScopeGuard([memoryEntry, savedMemoryManager]() noexcept { memoryEntry->second = savedMemoryManager; });
  memoryEntry->second = std::make_shared<CountingMemoryManager>(savedMemoryManager, fallbackMemoryFactoryCalls);

  auto manager = std::make_shared<CopyTestManager>(format,
                                                   [failure, selectedFactoryCalls, format](DataType, const ShapeType& tuples, const ShapeType& components, const std::optional<ShapeType>&,
                                                                                           DataStoreInitializationMode) -> std::unique_ptr<IDataStore> {
                                                     ++(*selectedFactoryCalls);
                                                     switch(failure)
                                                     {
                                                     case 0:
                                                       throw std::runtime_error("injected array creation factory failure -9880");
                                                     case 1:
                                                       return nullptr;
                                                     case 2:
                                                       return std::make_unique<DataStore<float32>>(tuples, components, 0.0F);
                                                     case 3:
                                                       return std::make_unique<DataStore<int32>>(ShapeType{99}, components, 0);
                                                     case 4:
                                                       return std::make_unique<DataStore<int32>>(tuples, components, 0);
                                                     case 5:
                                                       throw std::bad_alloc();
                                                     default:
                                                       return std::make_unique<ProbedCopyStore>(tuples, components, std::make_shared<StoreCopyProbe>(), format, true);
                                                     }
                                                   });
  auto managerRegistrationResult = collection.addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(managerRegistrationResult);

  DataStructure ds;
  auto resolver = std::make_shared<CopyPathResolver>();
  resolver->selected_format = format;
  ds.setFormatResolver(resolver);
  DataStructObserver observer(ds);
  const DataPath outputPath({"RejectedOutput"});
  const usize initialAddCount = observer.getDataAddedCount();

  if(failure == 5)
  {
    REQUIRE_THROWS_AS(ArrayCreationUtilities::CreateArray<int32>(ds, ShapeType{3}, ShapeType{2}, outputPath, IDataAction::Mode::Execute, format), std::bad_alloc);
  }
  else
  {
    auto result = ArrayCreationUtilities::CreateArray<int32>(ds, ShapeType{3}, ShapeType{2}, outputPath, IDataAction::Mode::Execute, format);
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    REQUIRE(result.errors().front().code == -265);
    REQUIRE(result.errors().front().message.find(format) != std::string::npos);
    REQUIRE(result.errors().front().message.find(outputPath.toString()) != std::string::npos);
    if(failure == 0)
    {
      REQUIRE(result.errors().front().message.find("-9880") != std::string::npos);
    }
  }

  REQUIRE_FALSE(ds.containsData(outputPath));
  REQUIRE(observer.getDataAddedCount() == initialAddCount);
  REQUIRE(*selectedFactoryCalls == 1);
  REQUIRE(*fallbackMemoryFactoryCalls == 0);
}

TEST_CASE("StorageFormatPlan: planned arrays resolve root parent and first parent paths", "[simplnx][DataArray][StorageFormatPlan]")
{
  SECTION("root create")
  {
    DataStructure ds;
    auto resolver = std::make_shared<CopyPathResolver>();
    ds.setFormatResolver(resolver);
    DataStructObserver observer(ds);
    const DataPath expectedPath({"RootValues"});

    auto result = DataArray<int32>::CreatePlanned(ds, expectedPath.getTargetName(), ShapeType{2}, ShapeType{3}, "");

    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value() == ds.getDataAs<DataArray<int32>>(expectedPath));
    REQUIRE(resolver->paths == std::vector<DataPath>{expectedPath});
    REQUIRE(resolver->bytes == std::vector<uint64>{6 * sizeof(int32)});
    REQUIRE(observer.getDataAddedCount() == 1);
  }

  SECTION("explicit memory bypasses destination policy")
  {
    DataStructure ds;
    auto resolver = std::make_shared<CopyPathResolver>();
    resolver->failure_message = "explicit planned creation must not resolve policy";
    ds.setFormatResolver(resolver);
    DataStructObserver observer(ds);
    const DataPath expectedPath({"ExplicitMemory"});

    auto result = DataArray<int32>::CreatePlanned(ds, expectedPath.getTargetName(), ShapeType{2}, ShapeType{3}, Preferences::k_InMemoryFormat.str());

    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value() == ds.getDataAs<DataArray<int32>>(expectedPath));
    REQUIRE(resolver->paths.empty());
    REQUIRE(observer.getDataAddedCount() == 1);
  }

  SECTION("parented import preserves the supplied identifier")
  {
    DataStructure ds;
    auto* parentPtr = DataGroup::Create(ds, "Parent");
    REQUIRE(parentPtr != nullptr);
    auto resolver = std::make_shared<CopyPathResolver>();
    ds.setFormatResolver(resolver);
    DataStructObserver observer(ds);
    const DataPath expectedPath({"Parent", "ImportedValues"});
    constexpr DataObject::IdType k_ImportId = 701;

    auto result = DataArray<int32>::ImportPlanned(ds, expectedPath.getTargetName(), k_ImportId, ShapeType{4}, ShapeType{1}, "", parentPtr->getId());

    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value() == ds.getDataAs<DataArray<int32>>(expectedPath));
    REQUIRE(result.value()->getId() == k_ImportId);
    REQUIRE(resolver->paths == std::vector<DataPath>{expectedPath});
    REQUIRE(resolver->bytes == std::vector<uint64>{4 * sizeof(int32)});
    REQUIRE(observer.getDataAddedCount() == 1);
  }

  SECTION("multiply parented object uses its first path")
  {
    DataStructure ds;
    auto* firstParentPtr = DataGroup::Create(ds, "First");
    auto* secondParentPtr = DataGroup::Create(ds, "Second");
    REQUIRE(firstParentPtr != nullptr);
    REQUIRE(secondParentPtr != nullptr);
    auto* sharedParentPtr = DataGroup::Create(ds, "Shared", firstParentPtr->getId());
    REQUIRE(sharedParentPtr != nullptr);
    REQUIRE(ds.setAdditionalParent(sharedParentPtr->getId(), secondParentPtr->getId()));
    const DataPath expectedPath = sharedParentPtr->getDataPaths().front().createChildPath("ChildValues");
    auto resolver = std::make_shared<CopyPathResolver>();
    ds.setFormatResolver(resolver);
    DataStructObserver observer(ds);

    auto result = DataArray<int32>::CreatePlanned(ds, expectedPath.getTargetName(), ShapeType{5}, ShapeType{1}, "", sharedParentPtr->getId());

    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(resolver->paths == std::vector<DataPath>{expectedPath});
    REQUIRE(resolver->bytes == std::vector<uint64>{5 * sizeof(int32)});
    REQUIRE(observer.getDataAddedCount() == 1);
  }
}

TEST_CASE("StorageFormatPlan: planned array failures publish no object or notification", "[simplnx][DataArray][StorageFormatPlan]")
{
  SECTION("missing parent fails before policy resolution")
  {
    DataStructure ds;
    auto resolver = std::make_shared<CopyPathResolver>();
    ds.setFormatResolver(resolver);
    DataStructObserver observer(ds);

    auto result = DataArray<int32>::CreatePlanned(ds, "RejectedMissingParent", ShapeType{2}, ShapeType{1}, "", DataObject::IdType{99999});

    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    REQUIRE(result.errors().front().code == -10610);
    REQUIRE(resolver->paths.empty());
    REQUIRE_FALSE(ds.containsData(DataPath({"RejectedMissingParent"})));
    REQUIRE(observer.getDataAddedCount() == 0);
  }

  SECTION("resolver failure retains path and cause")
  {
    DataStructure ds;
    auto resolver = std::make_shared<CopyPathResolver>();
    resolver->failure_message = "injected planned-array resolver failure -9881";
    ds.setFormatResolver(resolver);
    DataStructObserver observer(ds);
    const DataPath rejectedPath({"RejectedResolution"});

    auto result = DataArray<int32>::CreatePlanned(ds, rejectedPath.getTargetName(), ShapeType{2}, ShapeType{1}, "");

    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    REQUIRE(result.errors().front().code == -10601);
    REQUIRE(result.errors().front().message.find(rejectedPath.toString()) != std::string::npos);
    REQUIRE(result.errors().front().message.find("-9881") != std::string::npos);
    REQUIRE(resolver->paths == std::vector<DataPath>{rejectedPath});
    REQUIRE_FALSE(ds.containsData(rejectedPath));
    REQUIRE(observer.getDataAddedCount() == 0);
  }

  SECTION("resolver allocation failure propagates")
  {
    DataStructure ds;
    auto resolver = std::make_shared<CopyPathResolver>();
    resolver->throw_bad_allocation = true;
    ds.setFormatResolver(resolver);
    DataStructObserver observer(ds);
    const DataPath rejectedPath({"RejectedAllocation"});

    REQUIRE_THROWS_AS(DataArray<int32>::CreatePlanned(ds, rejectedPath.getTargetName(), ShapeType{2}, ShapeType{1}, ""), std::bad_alloc);
    REQUIRE(resolver->paths == std::vector<DataPath>{rejectedPath});
    REQUIRE_FALSE(ds.containsData(rejectedPath));
    REQUIRE(observer.getDataAddedCount() == 0);
  }

  SECTION("insertion failure occurs after one successful plan")
  {
    DataStructure ds;
    auto* nonGroupParentPtr = DataArray<int32>::Create(ds, "NonGroupParent", std::make_shared<DataStore<int32>>(ShapeType{1}, ShapeType{1}, 0));
    REQUIRE(nonGroupParentPtr != nullptr);
    auto resolver = std::make_shared<CopyPathResolver>();
    ds.setFormatResolver(resolver);
    DataStructObserver observer(ds);
    const DataPath rejectedPath({"NonGroupParent", "RejectedChild"});

    auto result = DataArray<int32>::CreatePlanned(ds, rejectedPath.getTargetName(), ShapeType{2}, ShapeType{1}, "", nonGroupParentPtr->getId());

    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    REQUIRE(result.errors().front().code == -10612);
    REQUIRE(result.errors().front().message.find(rejectedPath.toString()) != std::string::npos);
    REQUIRE(resolver->paths == std::vector<DataPath>{rejectedPath});
    REQUIRE_FALSE(ds.containsData(rejectedPath));
    REQUIRE(observer.getDataAddedCount() == 0);
  }
}

TEST_CASE("StorageFormatPlan: planned factories reject invalid names before policy resolution", "[simplnx][DataArray][StorageFormatPlan]")
{
  const std::string invalidName = GENERATE(std::string{}, std::string{"Bad/Name"});
  const bool parented = GENERATE(false, true);
  const bool importing = GENERATE(false, true);
  CAPTURE(invalidName, parented, importing);

  DataStructure ds;
  std::optional<DataObject::IdType> parentId;
  if(parented)
  {
    auto* parentPtr = DataGroup::Create(ds, "Parent");
    REQUIRE(parentPtr != nullptr);
    parentId = parentPtr->getId();
  }
  auto resolver = std::make_shared<CopyPathResolver>();
  ds.setFormatResolver(resolver);
  DataStructObserver observer(ds);
  const usize initialObjectCount = ds.getAllDataObjectIds().size();
  constexpr DataObject::IdType k_ImportId = 901;

  Result<DataArray<int32>*> result = importing ? DataArray<int32>::ImportPlanned(ds, invalidName, k_ImportId, ShapeType{2}, ShapeType{1}, "", parentId) :
                                                 DataArray<int32>::CreatePlanned(ds, invalidName, ShapeType{2}, ShapeType{1}, "", parentId);

  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  REQUIRE(result.errors().front().code == -10610);
  REQUIRE(result.errors().front().message.find(invalidName) != std::string::npos);
  REQUIRE(resolver->paths.empty());
  REQUIRE(ds.getAllDataObjectIds().size() == initialObjectCount);
  REQUIRE(ds.getData(k_ImportId) == nullptr);
  REQUIRE(observer.getDataAddedCount() == 0);
}

TEST_CASE("StorageFormatPlan: planned helpers preserve scalar and zero-dimension shapes", "[simplnx][DataArray][StorageFormatPlan]")
{
  DataStructure ds;
  auto resolver = std::make_shared<CopyPathResolver>();
  ds.setFormatResolver(resolver);
  const DataPath emptyTuplePath({"EmptyTuples"});
  const DataPath emptyComponentPath({"EmptyComponents"});
  const DataPath scalarPath({"Scalar"});
  const DataPath zeroDimensionPath({"ZeroDimension"});

  auto emptyTupleResult = DataStoreUtilities::CreatePlannedDataStore<uint64>(ds, emptyTuplePath, ShapeType{}, ShapeType{3}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(emptyTupleResult);
  REQUIRE(emptyTupleResult.value()->getTupleShape().empty());
  REQUIRE(emptyTupleResult.value()->getComponentShape() == ShapeType{3});
  REQUIRE(emptyTupleResult.value()->getNumberOfTuples() == 1);
  REQUIRE(emptyTupleResult.value()->getNumberOfComponents() == 3);
  REQUIRE(emptyTupleResult.value()->getSize() == 3);

  auto emptyComponentResult = DataArray<uint64>::CreatePlanned(ds, emptyComponentPath.getTargetName(), ShapeType{3}, ShapeType{}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(emptyComponentResult);
  REQUIRE(emptyComponentResult.value()->getTupleShape() == ShapeType{3});
  REQUIRE(emptyComponentResult.value()->getComponentShape().empty());
  REQUIRE(emptyComponentResult.value()->getNumberOfTuples() == 3);
  REQUIRE(emptyComponentResult.value()->getNumberOfComponents() == 1);
  REQUIRE(emptyComponentResult.value()->getSize() == 3);

  auto scalarResult = DataStoreUtilities::CreatePlannedDataStore<uint64>(ds, scalarPath, ShapeType{}, ShapeType{}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(scalarResult);
  REQUIRE(scalarResult.value()->getTupleShape().empty());
  REQUIRE(scalarResult.value()->getComponentShape().empty());
  REQUIRE(scalarResult.value()->getNumberOfTuples() == 1);
  REQUIRE(scalarResult.value()->getNumberOfComponents() == 1);
  REQUIRE(scalarResult.value()->getSize() == 1);

  auto zeroDimensionResult = DataArray<uint64>::CreatePlanned(ds, zeroDimensionPath.getTargetName(), ShapeType{3, 0}, ShapeType{2}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(zeroDimensionResult);
  REQUIRE(zeroDimensionResult.value()->getTupleShape() == ShapeType{3, 0});
  REQUIRE(zeroDimensionResult.value()->getComponentShape() == ShapeType{2});
  REQUIRE(zeroDimensionResult.value()->getNumberOfTuples() == 0);
  REQUIRE(zeroDimensionResult.value()->getNumberOfComponents() == 2);
  REQUIRE(zeroDimensionResult.value()->getSize() == 0);

  REQUIRE(resolver->paths == std::vector<DataPath>{emptyTuplePath, emptyComponentPath, scalarPath, zeroDimensionPath});
  REQUIRE(resolver->bytes == std::vector<uint64>{3 * sizeof(uint64), 3 * sizeof(uint64), sizeof(uint64), 0});

  const usize maximum = std::numeric_limits<usize>::max();
  auto overflowResult = DataStoreUtilities::CreatePlannedDataStore<uint8>(ds, DataPath({"Overflow"}), ShapeType{}, ShapeType{maximum, 2}, "");
  REQUIRE(overflowResult.invalid());
  REQUIRE_FALSE(overflowResult.errors().empty());
  REQUIRE(overflowResult.errors().front().code == -10614);
  auto arrayOverflowResult = DataArray<uint8>::CreatePlanned(ds, "ArrayOverflow", ShapeType{}, ShapeType{maximum, 2}, "");
  REQUIRE(arrayOverflowResult.invalid());
  REQUIRE_FALSE(arrayOverflowResult.errors().empty());
  REQUIRE(arrayOverflowResult.errors().front().code == -10611);
  REQUIRE(resolver->paths == std::vector<DataPath>{emptyTuplePath, emptyComponentPath, scalarPath, zeroDimensionPath});
}

TEST_CASE("StorageFormatPlan: value stores retain populated scalar shape semantics", "[simplnx][DataArray][StorageFormatPlan]")
{
  DataStructure ds;
  auto resolver = std::make_shared<CopyPathResolver>();
  ds.setFormatResolver(resolver);
  const DataPath valuePath({"ScalarValues"});

  auto store = DataStoreUtilities::CreateDataStore<uint64>(ds, valuePath, ShapeType{3}, ShapeType{});

  REQUIRE(store != nullptr);
  REQUIRE(store->getNumberOfTuples() == 3);
  REQUIRE(store->getNumberOfComponents() == 1);
  REQUIRE(store->getSize() == 3);
  REQUIRE(resolver->paths == std::vector<DataPath>{valuePath});
  REQUIRE(resolver->bytes == std::vector<uint64>{3 * sizeof(uint64)});
}

TEST_CASE("StorageFormatPlan: scalar-shape metadata copy retains identity-product semantics", "[simplnx][DataArray][StorageFormatPlan]")
{
  DataStructure ds;
  auto resolver = std::make_shared<CopyPathResolver>();
  ds.setFormatResolver(resolver);
  auto placeholderResult = EmptyDataStore<uint64>::Create(ShapeType{3}, ShapeType{}, "");
  SIMPLNX_RESULT_REQUIRE_VALID(placeholderResult);
  std::shared_ptr<AbstractDataStore<uint64>> placeholder(std::move(placeholderResult.value()));
  auto* sourcePtr = DataArray<uint64>::Create(ds, "MetadataSource", std::move(placeholder));
  REQUIRE(sourcePtr != nullptr);
  const DataPath copyPath({"MetadataCopy"});

  auto copiedObject = sourcePtr->deepCopy(copyPath);

  REQUIRE(copiedObject != nullptr);
  REQUIRE(resolver->paths == std::vector<DataPath>{copyPath});
  REQUIRE(resolver->bytes == std::vector<uint64>{3 * sizeof(uint64)});
  const auto* copiedArrayPtr = ds.getDataAs<DataArray<uint64>>(copyPath);
  REQUIRE(copiedArrayPtr != nullptr);
  REQUIRE(copiedArrayPtr->getTupleShape() == ShapeType{3});
  REQUIRE(copiedArrayPtr->getComponentShape().empty());
  REQUIRE(copiedArrayPtr->getNumberOfTuples() == 3);
  REQUIRE(copiedArrayPtr->getNumberOfComponents() == 1);
  REQUIRE(copiedArrayPtr->getSize() == 3);
}

TEST_CASE("StorageFormatPlan: ArrayCreation preflight records plans without value factories", "[simplnx][DataArray][StorageFormatPlan]")
{
  const std::string format = "array-preflight-planned-ooc";
  auto selectedFactoryCalls = std::make_shared<usize>(0);
  auto fallbackMemoryFactoryCalls = std::make_shared<usize>(0);
  auto& collection = Application::GetOrCreateInstance()->getIOCollection();

  auto memoryEntry = std::find_if(collection.begin(), collection.end(), [](const auto& entry) { return entry.first == Preferences::k_InMemoryFormat.str(); });
  REQUIRE(memoryEntry != collection.end());
  const auto savedMemoryManager = memoryEntry->second;
  auto restoreMemoryManager = MakeScopeGuard([memoryEntry, savedMemoryManager]() noexcept { memoryEntry->second = savedMemoryManager; });
  memoryEntry->second = std::make_shared<CountingMemoryManager>(savedMemoryManager, fallbackMemoryFactoryCalls);

  auto manager = std::make_shared<CopyTestManager>(
      format,
      [selectedFactoryCalls, format](DataType, const ShapeType& tuples, const ShapeType& components, const std::optional<ShapeType>&, DataStoreInitializationMode) -> std::unique_ptr<IDataStore> {
        ++(*selectedFactoryCalls);
        return std::make_unique<ProbedCopyStore>(tuples, components, std::make_shared<StoreCopyProbe>(), format, true);
      });
  auto managerRegistrationResult = collection.addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(managerRegistrationResult);

  DataStructure ds;
  const DataPath memoryPath({"MemoryPlan"});
  const DataPath oocPath({"OocPlan"});
  auto memoryResult = ArrayCreationUtilities::CreateArray<int32>(ds, ShapeType{3}, ShapeType{2}, memoryPath, IDataAction::Mode::Preflight, Preferences::k_InMemoryFormat.str());
  SIMPLNX_RESULT_REQUIRE_VALID(memoryResult);
  auto oocResult = ArrayCreationUtilities::CreateArray<int32>(ds, ShapeType{3}, ShapeType{2}, oocPath, IDataAction::Mode::Preflight, format);
  SIMPLNX_RESULT_REQUIRE_VALID(oocResult);

  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<int32>>(memoryPath));
  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<int32>>(oocPath));
  const auto& memoryArray = ds.getDataRefAs<DataArray<int32>>(memoryPath);
  const auto& oocArray = ds.getDataRefAs<DataArray<int32>>(oocPath);
  REQUIRE(memoryArray.getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(memoryArray.getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(oocArray.getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(oocArray.getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(dynamic_cast<const EmptyDataStore<int32>&>(oocArray.getDataStoreRef()).getDataFormat() == format);
  REQUIRE(*selectedFactoryCalls == 0);
  REQUIRE(*fallbackMemoryFactoryCalls == 0);
}

TEST_CASE("StorageFormatPlan: value helper preserves nullable factory failures", "[simplnx][DataArray][StorageFormatPlan]")
{
  const std::string format = "value-helper-invalid-factory";
  const auto failure = GENERATE(0, 1, 2);
  CAPTURE(failure);
  auto selectedFactoryCalls = std::make_shared<usize>(0);
  auto fallbackMemoryFactoryCalls = std::make_shared<usize>(0);

  auto& collection = Application::GetOrCreateInstance()->getIOCollection();
  auto memoryEntry = std::find_if(collection.begin(), collection.end(), [](const auto& entry) { return entry.first == Preferences::k_InMemoryFormat.str(); });
  REQUIRE(memoryEntry != collection.end());
  const auto savedMemoryManager = memoryEntry->second;
  auto restoreMemoryManager = MakeScopeGuard([memoryEntry, savedMemoryManager]() noexcept { memoryEntry->second = savedMemoryManager; });
  memoryEntry->second = std::make_shared<CountingMemoryManager>(savedMemoryManager, fallbackMemoryFactoryCalls);

  auto manager = std::make_shared<CopyTestManager>(
      format,
      [failure, selectedFactoryCalls](DataType, const ShapeType& tuples, const ShapeType& components, const std::optional<ShapeType>&, DataStoreInitializationMode) -> std::unique_ptr<IDataStore> {
        ++(*selectedFactoryCalls);
        if(failure == 0)
        {
          return nullptr;
        }
        if(failure == 1)
        {
          return std::make_unique<DataStore<float32>>(tuples, components, 0.0F);
        }
        return std::make_unique<DataStore<int32>>(tuples, components, 0);
      });
  auto managerRegistrationResult = collection.addIOManager(manager);
  SIMPLNX_RESULT_REQUIRE_VALID(managerRegistrationResult);

  DataStructure ds;
  auto resolver = std::make_shared<CopyPathResolver>();
  resolver->selected_format = format;
  ds.setFormatResolver(resolver);
  const DataPath path({"RejectedValueStore"});

  auto store = DataStoreUtilities::CreateDataStore<int32>(ds, path, ShapeType{2}, ShapeType{1});

  REQUIRE(store == nullptr);
  REQUIRE(*selectedFactoryCalls == 1);
  REQUIRE(*fallbackMemoryFactoryCalls == 0);
}

TEST_CASE("StorageFormatPlan: canonical memory factory identity is accepted", "[simplnx][DataArray][StorageFormatPlan]")
{
  auto factoryCalls = std::make_shared<usize>(0);
  auto memoryManager = std::make_shared<CopyTestManager>(
      Preferences::k_InMemoryFormat.str(),
      [factoryCalls](DataType, const ShapeType& tuples, const ShapeType& components, const std::optional<ShapeType>&, DataStoreInitializationMode) -> std::unique_ptr<IDataStore> {
        ++(*factoryCalls);
        return std::make_unique<CanonicalMemoryStore>(tuples, components, 0);
      });
  auto& collection = Application::GetOrCreateInstance()->getIOCollection();
  auto memoryEntry = std::find_if(collection.begin(), collection.end(), [](const auto& entry) { return entry.first == Preferences::k_InMemoryFormat.str(); });
  REQUIRE(memoryEntry != collection.end());
  const auto savedMemoryManager = memoryEntry->second;
  auto restoreMemoryManager = MakeScopeGuard([memoryEntry, savedMemoryManager]() noexcept { memoryEntry->second = savedMemoryManager; });
  memoryEntry->second = memoryManager;

  DataStructure ds;
  const DataPath actionPath({"CanonicalActionMemory"});
  const DataPath helperPath({"CanonicalHelperMemory"});

  auto actionResult = ArrayCreationUtilities::CreateArray<int32>(ds, ShapeType{2}, ShapeType{1}, actionPath, IDataAction::Mode::Execute, Preferences::k_InMemoryFormat.str());
  SIMPLNX_RESULT_REQUIRE_VALID(actionResult);
  auto helperStore = DataStoreUtilities::CreateDataStore<int32>(ds, helperPath, ShapeType{2}, ShapeType{1});

  REQUIRE(helperStore != nullptr);
  REQUIRE(helperStore->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(helperStore->getDataFormat() == Preferences::k_InMemoryFormat.str());
  REQUIRE(*factoryCalls == 2);
}

TEST_CASE("StorageFormatPlan: null and prepared stores preserve array state", "[simplnx][DataArray][StorageFormatPlan]")
{
  DataStructure ds;
  auto resolver = std::make_shared<CopyPathResolver>();
  resolver->failure_message = "prepared stores must not resolve policy";
  ds.setFormatResolver(resolver);
  DataStructObserver observer(ds);

  REQUIRE(DataArray<int32>::Create(ds, "NullCreate", nullptr) == nullptr);
  REQUIRE(DataArray<int32>::Import(ds, "NullImport", 801, nullptr) == nullptr);
  REQUIRE_FALSE(ds.containsData(DataPath({"NullCreate"})));
  REQUIRE_FALSE(ds.containsData(DataPath({"NullImport"})));
  REQUIRE(observer.getDataAddedCount() == 0);

  auto createStore = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 7);
  auto* createdPtr = DataArray<int32>::Create(ds, "PreparedCreate", createStore);
  REQUIRE(createdPtr != nullptr);
  REQUIRE(createdPtr->getIDataStore() == createStore.get());

  auto importStore = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 8);
  auto* importedPtr = DataArray<int32>::Import(ds, "PreparedImport", 802, importStore);
  REQUIRE(importedPtr != nullptr);
  REQUIRE(importedPtr->getId() == 802);
  REQUIRE(importedPtr->getIDataStore() == importStore.get());

  const auto* originalPtr = createdPtr->getIDataStore();
  auto nullSetterResult = createdPtr->setDataStore(nullptr);
  REQUIRE(nullSetterResult.invalid());
  REQUIRE_FALSE(nullSetterResult.errors().empty());
  REQUIRE(nullSetterResult.errors().front().code == -10613);
  REQUIRE(createdPtr->getIDataStore() == originalPtr);

  auto replacement = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 9);
  auto replacementResult = createdPtr->setDataStore(replacement);
  SIMPLNX_RESULT_REQUIRE_VALID(replacementResult);
  REQUIRE(createdPtr->getIDataStore() == replacement.get());
  REQUIRE(resolver->paths.empty());
  REQUIRE(observer.getDataAddedCount() == 2);
}

TEST_CASE("Store copies preserve empty shapes and reject overflow before allocation", "[simplnx][DataStore][DeepCopy]")
{
  auto probe = std::make_shared<StoreCopyProbe>();
  ProbedCopyStore noTuples({0, 7}, {3}, probe);
  auto emptyCopy = noTuples.deepCopy("");
  REQUIRE(emptyCopy->getTupleShape() == ShapeType{0, 7});
  REQUIRE(probe->read_sizes.empty());
  REQUIRE_THROWS_AS(CalculateStoreCopyBytes({std::numeric_limits<usize>::max(), 2}, {1}, 1), std::runtime_error);
  REQUIRE_THROWS_AS(CalculateStoreCopyBytes({std::numeric_limits<usize>::max()}, {2}, 1), std::runtime_error);
  REQUIRE_THROWS_AS(CalculateStoreCopyBytes({std::numeric_limits<usize>::max()}, {1}, 2), std::runtime_error);
  REQUIRE(CalculateStoreCopyBytes({std::numeric_limits<usize>::max(), 0}, {2}, sizeof(int32)) == 0);
}

TEST_CASE("List and string store copies own independent values and preserve placeholders", "[simplnx][DataStore][DeepCopy]")
{
  for(const auto& format : std::vector<std::string>{"", Preferences::k_InMemoryFormat.str()})
  {
    ListStore<int32> lists(ShapeType{2, 2});
    lists.setList(0, std::vector<int32>{4, 8});
    lists.setList(3, std::vector<int32>{12});
    auto listCopy = lists.deepCopy(format);
    REQUIRE(listCopy->getTupleShape() == ShapeType{2, 2});
    REQUIRE(listCopy->getList(0) == std::vector<int32>{4, 8});
    listCopy->setList(0, std::vector<int32>{99});
    REQUIRE(lists.getList(0) == std::vector<int32>{4, 8});
    lists.setList(3, std::vector<int32>{88});
    REQUIRE(listCopy->getList(3) == std::vector<int32>{12});
    EmptyListStore<int32> emptyLists({2, 2});
    auto emptyListCopy = emptyLists.deepCopy(format);
    REQUIRE(dynamic_cast<EmptyListStore<int32>*>(emptyListCopy.get()) != nullptr);
    REQUIRE(emptyListCopy->getTupleShape() == ShapeType{2, 2});

    StringStore strings({"alpha", "beta", "gamma", "delta"}, {2, 2});
    auto stringCopy = strings.deepCopy(format);
    REQUIRE(stringCopy->getTupleShape() == ShapeType{2, 2});
    stringCopy->setValue(0, "changed");
    REQUIRE(strings.getValue(0) == "alpha");
    strings.setValue(1, "source changed");
    REQUIRE(stringCopy->getValue(1) == "beta");
    EmptyStringStore emptyStrings({2, 2});
    auto emptyStringCopy = emptyStrings.deepCopy(format);
    REQUIRE(emptyStringCopy->isPlaceholder());
    REQUIRE_THROWS_AS(emptyStringCopy->getValue(0), std::runtime_error);
  }
}

TEST_CASE("Array and NeighborList copies pass resolved memory formats and destination paths", "[simplnx][DataStore][DeepCopy]")
{
  DataStructure ds;
  auto resolver = std::make_shared<CopyPathResolver>();
  ds.setFormatResolver(resolver);
  auto store = std::make_shared<RecordingCopyStore>(ShapeType{3}, ShapeType{2}, 7);
  auto* arrayPtr = DataArray<int32>::Create(ds, "Numeric", store);
  REQUIRE(arrayPtr != nullptr);
  REQUIRE(arrayPtr->deepCopy(DataPath({"NumericCopy"})) != nullptr);
  REQUIRE(store->formats == std::vector<std::string>{""});
  REQUIRE(resolver->paths.back() == DataPath({"NumericCopy"}));
  REQUIRE(resolver->bytes.back() == 6 * sizeof(int32));

  auto* firstParentPtr = DataGroup::Create(ds, "First");
  auto* secondParentPtr = DataGroup::Create(ds, "Second");
  REQUIRE(firstParentPtr != nullptr);
  REQUIRE(secondParentPtr != nullptr);
  auto listStore = std::make_shared<RecordingCopyListStore>(ShapeType{3});
  listStore->setList(0, std::vector<int32>{2, 4, 6});
  auto* sourcePtr = NeighborList<int32>::Create(ds, "Source", listStore);
  auto* destinationPtr = NeighborList<int32>::Create(ds, "Destination", ShapeType{3}, secondParentPtr->getId());
  REQUIRE(sourcePtr != nullptr);
  REQUIRE(destinationPtr != nullptr);
  REQUIRE(ds.setAdditionalParent(destinationPtr->getId(), firstParentPtr->getId()));
  const auto expectedPath = destinationPtr->getDataPaths().front();
  *destinationPtr = *sourcePtr;
  REQUIRE(listStore->formats == std::vector<std::string>{""});
  REQUIRE(resolver->paths.back() == expectedPath);
  REQUIRE(resolver->bytes.back() == 3 * sizeof(int32));
  REQUIRE(sourcePtr->deepCopy(DataPath({"ListCopy"})) != nullptr);
  REQUIRE(listStore->formats == std::vector<std::string>{"", ""});
  REQUIRE(resolver->paths.back() == DataPath({"ListCopy"}));
  REQUIRE(sourcePtr->eraseTuples({1}) == 0);
  REQUIRE(listStore->formats == std::vector<std::string>{"", "", ""});
  REQUIRE(resolver->paths.back() == DataPath({"Source"}));
}

namespace
{
/**
 * @class DetachedCopyList
 * @brief Provides a detached list to test missing destination policy context.
 */
class DetachedCopyList : public NeighborList<int32>
{
public:
  /**
   * @brief Retains source values while removing destination policy context.
   * @param source List with an allocated store.
   */
  explicit DetachedCopyList(const NeighborList<int32>& source)
  : NeighborList<int32>(source)
  {
    setDataStructure(nullptr);
  }
};
} // namespace

TEST_CASE("NeighborList missing destination paths preserve the original store", "[simplnx][DataStore][DeepCopy]")
{
  DataStructure ds;
  auto* sourcePtr = NeighborList<int32>::Create(ds, "Source", ShapeType{3});
  REQUIRE(sourcePtr != nullptr);
  sourcePtr->setList(0, std::make_shared<std::vector<int32>>(std::initializer_list<int32>{7, 9}));
  NeighborList<int32> orphan(*sourcePtr);
  DetachedCopyList detached(*sourcePtr);
  for(auto* destinationPtr : std::vector<NeighborList<int32>*>{&orphan, &detached})
  {
    const auto originalStore = destinationPtr->getStore();
    REQUIRE_THROWS_WITH(*destinationPtr = *sourcePtr, Catch::Contains("Source") && Catch::Contains("destination array path"));
    REQUIRE(destinationPtr->getStore() == originalStore);
    REQUIRE(destinationPtr->eraseTuples({1}) == NeighborList<int32>::k_MissingCopyDestinationError);
    REQUIRE(destinationPtr->getStore() == originalStore);
    REQUIRE(destinationPtr->getNumberOfTuples() == 3);
    REQUIRE(destinationPtr->getStore()->getList(0) == std::vector<int32>{7, 9});
    REQUIRE(destinationPtr->eraseTuples({}) == 0);
  }
}

TEST_CASE("Large metadata array copies resolve exact logical bytes without allocating values", "[simplnx][DataStore][DeepCopy]")
{
  constexpr uint64 k_LogicalBytes = 100'000'000'000ULL;
  if constexpr(std::numeric_limits<usize>::max() >= k_LogicalBytes)
  {
    const ShapeType tupleShape{static_cast<usize>(k_LogicalBytes / sizeof(int32))};
    const ShapeType componentShape{1};
    DataStructure ds;
    auto resolver = std::make_shared<CopyPathResolver>();
    ds.setFormatResolver(resolver);
    auto sourceStoreResult = EmptyDataStore<int32>::Create(tupleShape, componentShape, "");
    SIMPLNX_RESULT_REQUIRE_VALID(sourceStoreResult);
    std::shared_ptr<EmptyDataStore<int32>> sourceStore(std::move(sourceStoreResult.value()));
    auto* sourcePtr = DataArray<int32>::Create(ds, "LargeMetadata", sourceStore);
    REQUIRE(sourcePtr != nullptr);
    REQUIRE(sourceStore->getDataFormat().empty());
    REQUIRE(sourcePtr->getIDataStore() == sourceStore.get());
    const DataPath destinationPath({"LargeMetadataCopy"});
    auto copy = sourcePtr->deepCopy(destinationPath);
    REQUIRE(copy != nullptr);
    REQUIRE(resolver->paths == std::vector<DataPath>{destinationPath});
    REQUIRE(resolver->bytes == std::vector<uint64>{k_LogicalBytes});
    auto* copiedArrayPtr = dynamic_cast<DataArray<int32>*>(copy.get());
    REQUIRE(copiedArrayPtr != nullptr);
    REQUIRE(copiedArrayPtr->getIDataStore() != sourcePtr->getIDataStore());
    REQUIRE(dynamic_cast<EmptyDataStore<int32>*>(copiedArrayPtr->getIDataStore()) != nullptr);
    REQUIRE(copiedArrayPtr->getIDataStore()->getStoreType() == IDataStore::StoreType::Empty);
    REQUIRE(copiedArrayPtr->getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
    REQUIRE(copiedArrayPtr->getIDataStore()->getDataFormat().empty());
    REQUIRE(copiedArrayPtr->getTupleShape() == tupleShape);
    REQUIRE(copiedArrayPtr->getComponentShape() == componentShape);
    REQUIRE_THROWS_AS(copiedArrayPtr->getDataStoreRef().getValue(0), std::runtime_error);
    REQUIRE(sourceStore->getDataFormat().empty());
    REQUIRE(sourcePtr->getIDataStore() == sourceStore.get());
  }
  else
  {
    SUCCEED("A 100 GB logical array exceeds this host's supported usize range");
  }
}

TEMPLATE_TEST_CASE("DataStore common initialization API changes future values only", "[simplnx][DataStore][GrowthInitialization]", int32, bool)
{
  using T = TestType;
  static_assert(GetMudflap<bool>());
  DataStore<T> concrete({2}, {2}, static_cast<T>(7));
  AbstractDataStore<T>& store = concrete;
  CHECK(store.getInitValue() == std::optional<T>{static_cast<T>(7)});
  const std::array<T, 4> prefix{T{0}, T{1}, T{1}, T{0}};
  REQUIRE(store.copyFromBuffer(0, {prefix.data(), prefix.size()}).valid());
  store.setInitValue(T{0});
  CHECK(store.getInitValue() == std::optional<T>{T{0}});
  store.setInitValue(std::optional<T>{T{1}});
  CHECK(store.getInitValue() == std::optional<T>{T{1}});
  std::array<T, 4> unchanged{};
  REQUIRE(store.copyIntoBuffer(0, {unchanged.data(), unchanged.size()}).valid());
  CHECK(unchanged == prefix);
  REQUIRE(store.resizeTuples({3}).valid());
  std::array<T, 6> actual{};
  REQUIRE(store.copyIntoBuffer(0, {actual.data(), actual.size()}).valid());
  CHECK(actual == std::array<T, 6>{T{0}, T{1}, T{1}, T{0}, T{1}, T{1}});
  store.setInitValue(std::nullopt);
  CHECK_FALSE(store.getInitValue().has_value());
  REQUIRE(store.resizeTuples({4}).valid());
  const T diagnostic = [] {
    if constexpr(std::is_same_v<T, bool>)
    {
      return true;
    }
    else
    {
      return T{-1414812757};
    }
  }();
  std::array<T, 8> grown{};
  REQUIRE(store.copyIntoBuffer(0, {grown.data(), grown.size()}).valid());
  CHECK(grown == std::array<T, 8>{T{0}, T{1}, T{1}, T{0}, T{1}, T{1}, diagnostic, diagnostic});
}

TEST_CASE("DataStore copy and move retain optional initialization", "[simplnx][DataStore][GrowthInitialization]")
{
  const bool missingInitializer = GENERATE(false, true);
  const int operation = GENERATE(0, 1, 2);
  DYNAMIC_SECTION("missing=" << missingInitializer << " operation=" << operation)
  {
    const std::optional<int32> initial = missingInitializer ? std::nullopt : std::optional<int32>{7};
    DataStore<int32> source({2}, {2}, initial);
    const std::array<int32, 4> prefix{10, 11, 20, 21};
    REQUIRE(source.copyFromBuffer(0, {prefix.data(), prefix.size()}).valid());
    std::unique_ptr<DataStore<int32>> target;
    if(operation == 0)
    {
      target = std::make_unique<DataStore<int32>>(source);
      target->setValue(0, 99);
      CHECK(source.getValue(0) == 10);
      target->setValue(0, 10);
    }
    else if(operation == 1)
    {
      target = std::make_unique<DataStore<int32>>(std::move(source));
    }
    else
    {
      target = std::make_unique<DataStore<int32>>(ShapeType{1}, ShapeType{1}, int32{-9});
      *target = std::move(source);
    }
    CHECK(target->getInitValue() == initial);
    REQUIRE(target->resizeTuples({3}).valid());
    const int32 tail = missingInitializer ? -1414812757 : 7;
    std::array<int32, 6> actual{};
    REQUIRE(target->copyIntoBuffer(0, {actual.data(), actual.size()}).valid());
    CHECK(actual == std::array<int32, 6>{10, 11, 20, 21, tail, tail});
  }
}

TEMPLATE_TEST_CASE("DataStore owned buffers retain an engaged diagnostic initializer", "[simplnx][DataStore][GrowthInitialization]", int32, bool)
{
  using T = TestType;
  auto buffer = std::make_unique<T[]>(2);
  buffer[0] = T{0};
  buffer[1] = T{1};
  DataStore<T> store(std::move(buffer), {2}, {1});
  const T diagnostic = [] {
    if constexpr(std::is_same_v<T, bool>)
    {
      return true;
    }
    else
    {
      return T{-1414812757};
    }
  }();
  CHECK(store.getInitValue() == std::optional<T>{diagnostic});
  REQUIRE(store.resizeTuples({3}).valid());
  std::array<T, 3> actual{};
  REQUIRE(store.copyIntoBuffer(0, {actual.data(), actual.size()}).valid());
  CHECK(actual == std::array<T, 3>{T{0}, T{1}, diagnostic});
}

TEST_CASE("Numeric placeholders retain initialization through copy and move", "[simplnx][DataStore][GrowthInitialization]")
{
  const bool missingInitializer = GENERATE(false, true);
  const int operation = GENERATE(0, 1, 2);
  DYNAMIC_SECTION("missing=" << missingInitializer << " operation=" << operation)
  {
    auto sourceResult = EmptyDataStore<int32>::Create({8}, {2}, "");
    REQUIRE(sourceResult.valid());
    auto source = std::move(sourceResult.value());
    CHECK(source->getInitValue() == std::optional<int32>{0});
    const std::optional<int32> initial = missingInitializer ? std::nullopt : std::optional<int32>{7};
    source->setInitValue(initial);
    std::unique_ptr<IDataStore> copy;
    if(operation == 0)
    {
      copy = std::make_unique<EmptyDataStore<int32>>(*source);
    }
    else if(operation == 1)
    {
      copy = std::make_unique<EmptyDataStore<int32>>(std::move(*source));
    }
    else
    {
      copy = source->deepCopy("");
    }
    const auto* typedCopyPtr = dynamic_cast<const EmptyDataStore<int32>*>(copy.get());
    REQUIRE(typedCopyPtr != nullptr);
    CHECK(typedCopyPtr->getStoreType() == IDataStore::StoreType::Empty);
    CHECK(typedCopyPtr->getPlannedStoreType() == IDataStore::StoreType::InMemory);
    CHECK(typedCopyPtr->getTupleShape() == ShapeType{8});
    CHECK(typedCopyPtr->getComponentShape() == ShapeType{2});
    CHECK(typedCopyPtr->getInitValue() == initial);
    CHECK(typedCopyPtr->getDataFormat().empty());
  }
}

TEST_CASE("DataStore resident value copies retain destination initialization", "[simplnx][DataStore][GrowthInitialization]")
{
  const int operation = GENERATE(0, 1, 2);
  DataStore<int32> source({2}, {2}, int32{-1});
  DataStore<int32> destination({2}, {2}, int32{7});
  const std::array<int32, 4> prefix{10, 11, 20, 21};
  REQUIRE(source.copyFromBuffer(0, {prefix.data(), prefix.size()}).valid());
  if(operation == 0)
  {
    REQUIRE(destination.copyFromBuffer(0, {prefix.data(), prefix.size()}).valid());
  }
  else if(operation == 1)
  {
    REQUIRE(destination.copyFrom(0, source, 0, 2).valid());
  }
  else
  {
    REQUIRE(destination.copy(source));
  }
  CHECK(destination.getInitValue() == std::optional<int32>{7});
  CHECK(source.getInitValue() == std::optional<int32>{-1});
  REQUIRE(destination.resizeTuples({3}).valid());
  std::array<int32, 6> actual{};
  REQUIRE(destination.copyIntoBuffer(0, {actual.data(), actual.size()}).valid());
  CHECK(actual == std::array<int32, 6>{10, 11, 20, 21, 7, 7});
}

namespace
{
// Exercise buffered copying deterministically without claiming disk-backed storage.
class BufferedCopyTestStore : public DataStore<int32>
{
public:
  using DataStore<int32>::DataStore;
  BufferedCopyTestStore(const BufferedCopyTestStore&) = default;
  BufferedCopyTestStore(BufferedCopyTestStore&&) = default;
  BufferedCopyTestStore& operator=(const BufferedCopyTestStore&) = delete;
  BufferedCopyTestStore& operator=(BufferedCopyTestStore&&) = delete;
  ~BufferedCopyTestStore() override = default;
  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }
};
} // namespace

TEST_CASE("DataArrayUtilities CopyData overlap", "[DataArrayUtilities][L05]")
{
  UnitTest::LoadPlugins();
  const bool buffered = GENERATE(false, true);
  const bool sharedStore = GENERATE(false, true);
  const usize components = GENERATE(usize{1}, usize{3});
  const usize tuples = GENERATE(usize{8}, usize{70000});
  const usize destination = GENERATE(usize{0}, usize{1}, usize{2});
  const usize source = 1;
  const usize count = tuples - 2;
  CAPTURE(buffered, sharedStore, components, tuples, destination);
  DataStructure dataStructure;
  std::shared_ptr<AbstractDataStore<int32>> store;
  if(buffered)
  {
    store = std::make_shared<BufferedCopyTestStore>(ShapeType{tuples}, ShapeType{components}, 0);
  }
  else
  {
    store = std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{components}, 0);
  }
  auto* valuesPtr = Int32Array::Create(dataStructure, "Values", store);
  REQUIRE(valuesPtr != nullptr);
  for(usize i = 0; i < valuesPtr->getSize(); ++i)
  {
    (*valuesPtr)[i] = static_cast<int32>(i);
  }
  auto* sourceValuesPtr = sharedStore ? Int32Array::Create(dataStructure, "Alias", store) : valuesPtr;
  REQUIRE(sourceValuesPtr != nullptr);
  auto result = CopyFromArray::CopyData(*sourceValuesPtr, *valuesPtr, destination, source, count);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  for(usize i = 0; i < valuesPtr->getSize(); ++i)
  {
    const bool copied = i >= destination * components && i < (destination + count) * components;
    const int32 expected = static_cast<int32>(copied ? i - destination * components + source * components : i);
    CAPTURE(i);
    REQUIRE((*valuesPtr)[i] == expected);
  }
}

TEST_CASE("DataArrayUtilities CopyData string overlap", "[DataArrayUtilities][L05]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* valuesPtr = StringArray::CreateWithValues(dataStructure, "Values", {6}, {"a", "b", "c", "d", "e", "f"});
  REQUIRE(valuesPtr != nullptr);
  auto result = CopyFromArray::CopyData(*valuesPtr, *valuesPtr, 2, 1, 3);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const std::vector<std::string> expected = {"a", "b", "b", "c", "d", "f"};
  for(usize i = 0; i < expected.size(); ++i)
  {
    CHECK((*valuesPtr)[i] == expected[i]);
  }
}

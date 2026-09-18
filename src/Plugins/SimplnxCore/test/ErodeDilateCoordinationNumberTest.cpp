#include <catch2/catch.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "SimplnxCore/Filters/ErodeDilateCoordinationNumberFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_GeomName("ImageGeom");
const std::string k_CellDataName("CellData");

const DataPath k_GeomPath({k_GeomName});
const DataPath k_CellDataPath = k_GeomPath.createChildPath(k_CellDataName);
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");

const std::string k_EbsdScanDataName("EBSD Scan Data");
const DataPath k_InputDataPath({"Input Data"});
const DataPath k_EbsdScanDataPath = k_InputDataPath.createChildPath(k_EbsdScanDataName);
const DataPath k_ExemplarFeatureIdsPath = k_EbsdScanDataPath.createChildPath("FeatureIds");
const std::string k_ExemplarDataContainerName("Exemplar Coordination Number");

struct CoordinationLiteralFixture
{
  DataStructure dataStructure;
  DataPath geometryPath;
  DataPath featureIdsPath;
  DataPath transferredValuesPath;
  DataPath ignoredValuesPath;
  MultiArraySelectionParameter::ValueType ignoredPaths;
};

/**
 * @brief Creates a literal coordination fixture with transferable and ignored siblings.
 * @param dimX Specifies the image X dimension.
 * @param dimY Specifies the image Y dimension.
 * @param dimZ Specifies the image Z dimension.
 * @param inputFeatureIds Contains one literal Feature ID per image cell.
 * @return In-memory fixture with scalar siblings that identify each source tuple.
 */
CoordinationLiteralFixture CreateCoordinationLiteralFixture(const usize dimX, const usize dimY, const usize dimZ, const std::vector<int32>& inputFeatureIds = {1, 0, 0, 1})
{
  REQUIRE(inputFeatureIds.size() == dimX * dimY * dimZ);
  CoordinationLiteralFixture fixture;
  fixture.geometryPath = DataPath({"Coordination Literal Geometry"});
  const DataPath cellDataPath = fixture.geometryPath.createChildPath("Cell Data");
  fixture.featureIdsPath = cellDataPath.createChildPath("Feature Ids");
  fixture.transferredValuesPath = cellDataPath.createChildPath("Transferred Values");
  fixture.ignoredValuesPath = cellDataPath.createChildPath("Ignored Values");
  fixture.ignoredPaths = {fixture.ignoredValuesPath};
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};

  auto* imageGeomPtr = ImageGeom::Create(fixture.dataStructure, fixture.geometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cellDataPtr = AttributeMatrix::Create(fixture.dataStructure, cellDataPath.getTargetName(), cellTupleShape, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(fixture.dataStructure, fixture.featureIdsPath, cellTupleShape, {1});
  auto* featureIdsArrayPtr = Int32Array::Create(fixture.dataStructure, fixture.featureIdsPath.getTargetName(), featureIdsStore, cellDataPtr->getId());
  REQUIRE(featureIdsArrayPtr != nullptr);
  featureIdsStore->fill(0);

  auto transferredValuesStore = DataStoreUtilities::CreateDataStore<int32>(fixture.dataStructure, fixture.transferredValuesPath, cellTupleShape, {1});
  auto* transferredValuesArrayPtr = Int32Array::Create(fixture.dataStructure, fixture.transferredValuesPath.getTargetName(), transferredValuesStore, cellDataPtr->getId());
  REQUIRE(transferredValuesArrayPtr != nullptr);
  transferredValuesStore->fill(0);

  auto ignoredValuesStore = DataStoreUtilities::CreateDataStore<int32>(fixture.dataStructure, fixture.ignoredValuesPath, cellTupleShape, {1});
  auto* ignoredValuesArrayPtr = Int32Array::Create(fixture.dataStructure, fixture.ignoredValuesPath.getTargetName(), ignoredValuesStore, cellDataPtr->getId());
  REQUIRE(ignoredValuesArrayPtr != nullptr);
  ignoredValuesStore->fill(0);

  REQUIRE(featureIdsStore->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(transferredValuesStore->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(ignoredValuesStore->getStoreType() == IDataStore::StoreType::InMemory);
  for(usize tupleIdx = 0; tupleIdx < inputFeatureIds.size(); tupleIdx++)
  {
    (*featureIdsStore)[tupleIdx] = inputFeatureIds[tupleIdx];
    (*transferredValuesStore)[tupleIdx] = static_cast<int32>((tupleIdx + 1) * 10);
    (*ignoredValuesStore)[tupleIdx] = static_cast<int32>((tupleIdx + 1) * 100);
  }

  return fixture;
}

/**
 * @brief Executes one non-looping pass and returns its storage result.
 * @param fixture Contains the input arrays and geometry.
 * @param threshold Minimum number of opposite-state neighbors that selects a transfer.
 * @return Filter execution result after a valid preflight.
 */
Result<> RunCoordinationLiteralFixture(CoordinationLiteralFixture& fixture, int32 threshold = 1)
{
  ErodeDilateCoordinationNumberFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key, std::make_any<int32>(threshold));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_Loop_Key, std::make_any<bool>(false));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(fixture.featureIdsPath));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(fixture.ignoredPaths));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(fixture.geometryPath));

  const auto preflightResult = filter.preflight(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(fixture.dataStructure, args);
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  return executeResult.result;
}

void ExecuteCoordinationLiteralFixture(CoordinationLiteralFixture& fixture, int32 threshold = 1)
{
  auto coordinationFixtureResult = RunCoordinationLiteralFixture(fixture, threshold);
  SIMPLNX_RESULT_REQUIRE_VALID(coordinationFixtureResult);
}

/**
 * @brief Checks every component and prints complete actual and expected values.
 * @tparam T Array element type.
 * @tparam N Number of scalar values in the literal oracle.
 * @param arrayRef Contains the actual values.
 * @param expectedValues Contains the literal expected values.
 * @param componentCount Number of scalar values in each tuple.
 */
template <typename T, usize N>
void CheckCoordinationValues(const DataArray<T>& arrayRef, const std::array<T, N>& expectedValues, usize componentCount = 1)
{
  REQUIRE(arrayRef.getNumberOfComponents() == componentCount);
  REQUIRE(arrayRef.getNumberOfTuples() * componentCount == expectedValues.size());
  REQUIRE(arrayRef.getStoreType() == IDataStore::StoreType::InMemory);
  std::array<T, N> actualValues = {};
  for(usize valueIdx = 0; valueIdx < expectedValues.size(); valueIdx++)
  {
    actualValues[valueIdx] = arrayRef[valueIdx];
  }
  INFO(fmt::format("{} components={} actual=[{}] expected=[{}]", arrayRef.getName(), componentCount, fmt::join(actualValues, ", "), fmt::join(expectedValues, ", ")));
  for(usize valueIdx = 0; valueIdx < expectedValues.size(); valueIdx++)
  {
    CAPTURE(valueIdx);
    CHECK(actualValues[valueIdx] == expectedValues[valueIdx]);
  }
}

template <typename T, usize N>
void CheckCoordinationValues(const CoordinationLiteralFixture& fixture, const DataPath& path, const std::array<T, N>& expectedValues, usize componentCount = 1)
{
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<DataArray<T>>(path));
  CheckCoordinationValues(fixture.dataStructure.getDataRefAs<DataArray<T>>(path), expectedValues, componentCount);
}

/**
 * @brief Creates a sibling with literal component values in a separate store.
 * @tparam T Array element type.
 * @tparam N Number of scalar input values.
 * @param fixture Owns the image and cell AttributeMatrix.
 * @param name Name of the new sibling array.
 * @param inputValues Contains all initial scalar values.
 * @param componentCount Number of scalar values per tuple.
 * @return Path of the new in-memory sibling array.
 */
template <typename T, usize N>
DataPath AddCoordinationSibling(CoordinationLiteralFixture& fixture, const std::string& name, const std::array<T, N>& inputValues, usize componentCount)
{
  const DataPath cellDataPath = fixture.featureIdsPath.getParent();
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<AttributeMatrix>(cellDataPath));
  const auto& cellDataRef = fixture.dataStructure.getDataRefAs<AttributeMatrix>(cellDataPath);
  const DataPath path = cellDataPath.createChildPath(name);
  auto store = DataStoreUtilities::CreateDataStore<T>(fixture.dataStructure, path, cellDataRef.getShape(), {componentCount});
  REQUIRE(store->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(store->getSize() == N);
  auto storeWriteResult = store->copyFromBuffer(0, nonstd::span<const T>(inputValues.data(), inputValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(storeWriteResult);
  REQUIRE(DataArray<T>::Create(fixture.dataStructure, name, store, cellDataRef.getId()) != nullptr);
  return path;
}

/**
 * @brief Creates a distinct array that shares the original array's store.
 * @tparam T Array element type.
 * @param fixture Owns the original array and new alias.
 * @param originalPath Identifies the shared store's existing owner.
 * @return Path of the alias after checking object and store identity.
 */
template <typename T>
DataPath AddCoordinationAlias(CoordinationLiteralFixture& fixture, const DataPath& originalPath)
{
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<DataArray<T>>(originalPath));
  const auto& originalArrayRef = fixture.dataStructure.getDataRefAs<DataArray<T>>(originalPath);
  const DataPath parentPath = originalPath.getParent();
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<AttributeMatrix>(parentPath));
  const auto& cellDataRef = fixture.dataStructure.getDataRefAs<AttributeMatrix>(parentPath);
  const DataPath aliasPath = parentPath.createChildPath(originalPath.getTargetName() + " Alias");
  auto store = originalArrayRef.getDataStorePtr().lock();
  REQUIRE(store != nullptr);
  auto* aliasArrayPtr = DataArray<T>::Create(fixture.dataStructure, aliasPath.getTargetName(), store, cellDataRef.getId());
  REQUIRE(aliasArrayPtr != nullptr);
  REQUIRE(aliasArrayPtr != &originalArrayRef);
  REQUIRE(aliasArrayPtr->getId() != originalArrayRef.getId());
  REQUIRE(aliasArrayPtr->getIDataStore() == originalArrayRef.getIDataStore());
  REQUIRE(aliasArrayPtr->getStoreType() == IDataStore::StoreType::InMemory);
  return aliasPath;
}

constexpr std::array<float32, 8> k_ComponentInput = {1.25F, -11.0F, 2.25F, -22.0F, 3.25F, -33.0F, 4.25F, -44.0F};
constexpr std::array<float32, 8> k_ComponentExpected = {3.25F, -33.0F, 3.25F, -33.0F, 4.25F, -44.0F, 3.25F, -33.0F};
constexpr std::array<bool, 8> k_BoolInput = {false, false, true, true, true, false, false, true};
constexpr std::array<bool, 8> k_BoolExpected = {true, false, true, false, false, true, true, false};

/**
 * @class CoordinationBulkFailureStore
 * @brief Rejects selected bulk operations after fixture setup.
 *
 * All fault state belongs to this store. Successful writes distinguish a previous-slice read from the initial destination read.
 */
class CoordinationBulkFailureStore : public DataStore<int32>
{
public:
  /**
   * @enum Fault
   * @brief Selects a bulk operation by offset and completed-write state.
   */
  enum class Fault
  {
    None,                  ///< Observes operations without rejecting them.
    InitialFeatureIdsRead, ///< Rejects the first read of the Feature ID store.
    DestinationRead,       ///< Rejects the sibling's initial destination read.
    NextSliceRead,         ///< Rejects the source read at offset two before a write completes.
    PreviousSliceRead,     ///< Rejects the source read at offset zero after the next destination loads.
    DestinationWrite       ///< Rejects the destination write at offset zero.
  };

  static constexpr int32 k_ErrorCode = -8137;
  static constexpr std::string_view k_ErrorMessage = "Injected coordination bulk-transfer failure";

  /**
   * @brief Creates the initialized scalar store for a two-slice XZ fixture.
   */
  CoordinationBulkFailureStore()
  : DataStore<int32>(ShapeType{2, 1, 2}, ShapeType{1}, int32{0})
  {
  }

  /**
   * @brief Starts fault observation after all fixture writes finish.
   * @param fault Selects the operation to reject; None observes a successful run.
   */
  void arm(Fault fault)
  {
    m_Fault = fault;
    m_IsArmed = true;
    m_ReadCount = 0;
    m_WriteCount = 0;
    m_CompletedWriteCount = 0;
    m_FailureCount = 0;
    m_RejectedOffset = 0;
    m_HasReadNextAfterWrite = false;
  }

  /**
   * @brief Reads a slice unless its offset and write state select the fault.
   * @param offset First scalar value in the source store.
   * @param buffer Receives values on success.
   * @return Injected error or the underlying read result.
   */
  Result<> copyIntoBuffer(usize offset, nonstd::span<int32> buffer) const override
  {
    if(m_IsArmed)
    {
      ++m_ReadCount;
      const bool rejectInitialRead = (m_Fault == Fault::InitialFeatureIdsRead || m_Fault == Fault::DestinationRead) && offset == 0 && m_ReadCount == 1;
      const bool rejectNextRead = m_Fault == Fault::NextSliceRead && offset == 2 && m_CompletedWriteCount == 0;
      const bool rejectPreviousRead = m_Fault == Fault::PreviousSliceRead && offset == 0 && m_CompletedWriteCount == 1 && m_HasReadNextAfterWrite;
      if(rejectInitialRead || rejectNextRead || rejectPreviousRead)
      {
        ++m_FailureCount;
        m_RejectedOffset = offset;
        return MakeErrorResult(k_ErrorCode, std::string(k_ErrorMessage));
      }
    }
    auto result = DataStore<int32>::copyIntoBuffer(offset, buffer);
    if(m_IsArmed && result.valid() && offset == 2 && m_CompletedWriteCount == 1)
    {
      m_HasReadNextAfterWrite = true;
    }
    return result;
  }

  /**
   * @brief Writes a slice or rejects the selected destination write.
   * @param offset First scalar value in the destination store.
   * @param buffer Supplies the values to copy.
   * @return Injected error or the underlying write result.
   */
  Result<> copyFromBuffer(usize offset, nonstd::span<const int32> buffer) override
  {
    if(m_IsArmed)
    {
      ++m_WriteCount;
      if(m_Fault == Fault::DestinationWrite && offset == 0)
      {
        ++m_FailureCount;
        m_RejectedOffset = offset;
        return MakeErrorResult(k_ErrorCode, std::string(k_ErrorMessage));
      }
    }
    auto result = DataStore<int32>::copyFromBuffer(offset, buffer);
    if(m_IsArmed && result.valid())
    {
      ++m_CompletedWriteCount;
    }
    return result;
  }

  usize getReadCount() const
  {
    return m_ReadCount;
  }
  usize getWriteCount() const
  {
    return m_WriteCount;
  }
  usize getCompletedWriteCount() const
  {
    return m_CompletedWriteCount;
  }
  usize getFailureCount() const
  {
    return m_FailureCount;
  }
  usize getRejectedOffset() const
  {
    return m_RejectedOffset;
  }
  bool hasReadNextAfterWrite() const
  {
    return m_HasReadNextAfterWrite;
  }

private:
  Fault m_Fault = Fault::None;
  bool m_IsArmed = false;
  mutable usize m_ReadCount = 0;
  usize m_WriteCount = 0;
  usize m_CompletedWriteCount = 0;
  mutable usize m_FailureCount = 0;
  mutable usize m_RejectedOffset = 0;
  mutable bool m_HasReadNextAfterWrite = false;
};

void BuildTestData(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ, usize blockSize)
{
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};
  const usize sliceSize = dimX * dimY;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto featureIdsDataStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FeatureIdsPath, cellTupleShape, {1});
  auto* featureIdsArray = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsDataStore, cellAM->getId());
  auto& featureIdsStore = featureIdsArray->getDataStoreRef();

  auto eulerDataStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_CellDataPath.createChildPath("EulerAngles"), cellTupleShape, {3});
  auto* eulerArray = DataArray<float32>::Create(dataStructure, "EulerAngles", eulerDataStore, cellAM->getId());
  auto& eulerStore = eulerArray->getDataStoreRef();

  const usize blocksPerDimX = dimX / blockSize;
  const usize blocksPerDimY = dimY / blockSize;

  std::vector<int32> featureIdsBuf(sliceSize);
  std::vector<float32> eulerBuf(sliceSize * 3);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize inSlice = y * dimX + x;

        usize bx = x / blockSize;
        usize by = y / blockSize;
        usize bz = z / blockSize;
        int32 blockFeatureId = static_cast<int32>(bz * blocksPerDimY * blocksPerDimX + by * blocksPerDimX + bx + 1);

        bool isBad = ((x * 7 + y * 13 + z * 29) % 7 == 0);
        featureIdsBuf[inSlice] = isBad ? 0 : blockFeatureId;

        const usize eIdx = inSlice * 3;
        eulerBuf[eIdx] = static_cast<float32>(x) / static_cast<float32>(dimX);
        eulerBuf[eIdx + 1] = static_cast<float32>(y) / static_cast<float32>(dimY);
        eulerBuf[eIdx + 2] = static_cast<float32>(z) / static_cast<float32>(dimZ);
      }
    }
    const usize zOffset = z * sliceSize;
    auto featureIdsStoreWriteResult = featureIdsStore.copyFromBuffer(zOffset, nonstd::span<const int32>(featureIdsBuf.data(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(featureIdsStoreWriteResult);
    auto eulerStoreWriteResult = eulerStore.copyFromBuffer(zOffset * 3, nonstd::span<const float32>(eulerBuf.data(), sliceSize * 3));
    SIMPLNX_RESULT_REQUIRE_VALID(eulerStoreWriteResult);
  }
}

usize CountBadVoxels(const DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef();
  const usize sliceSize = dimX * dimY;
  std::vector<int32> buf(sliceSize);
  usize count = 0;
  for(usize z = 0; z < dimZ; z++)
  {
    auto featureIdsReadResult = featureIds.copyIntoBuffer(z * sliceSize, nonstd::span<int32>(buf.data(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(featureIdsReadResult);
    for(usize i = 0; i < sliceSize; i++)
    {
      if(buf[i] == 0)
      {
        count++;
      }
    }
  }
  return count;
}
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Generate Test Data", "[SimplnxCore][ErodeDilateCoordinationNumberFilter][.GenerateTestData]")
{
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "erode_dilate_coordination_number";
  fs::create_directories(outputDir);

  // The small fixture uses a 20-cubed volume and block size 5.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 20, 20, 20, 5);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "small_input.dream3d");
  }

  // The large fixture uses a 200-cubed volume and block size 25.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 200, 200, 200, 25);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "large_input.dream3d");
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_erode_dilate_test.tar.gz", "6_6_erode_dilate_test");
  const fs::path exemplarFilePath = fs::path(unit_test::k_TestFilesDir.view()) / "6_6_erode_dilate_test" / "6_6_erode_dilate_coordination_number.dream3d";
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_ExemplarFeatureIdsPath));
  const auto& featureIdsArrayRef = dataStructure.getDataRefAs<Int32Array>(k_ExemplarFeatureIdsPath);
  REQUIRE(featureIdsArrayRef.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);

  const ErodeDilateCoordinationNumberFilter filter;
  Arguments args;
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key, std::make_any<int32>(6));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_Loop_Key, std::make_any<bool>(false));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_ExemplarFeatureIdsPath));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputDataPath));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_EbsdScanDataPath, k_ExemplarDataContainerName);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Literal Scan Order", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  constexpr std::array<int32, 4> k_ExpectedFeatureIds = {0, 0, 1, 1};
  constexpr std::array<int32, 4> k_ExpectedTransferredValues = {20, 20, 40, 40};
  constexpr std::array<int32, 4> k_ExpectedIgnoredValues = {100, 200, 300, 400};

  for(const auto& [label, dimensions] : std::array<std::pair<std::string_view, std::array<usize, 3>>, 3>{{{"X", {4, 1, 1}}, {"Y", {1, 4, 1}}, {"Z", {1, 1, 4}}}})
  {
    DYNAMIC_SECTION(label)
    {
      CoordinationLiteralFixture fixture = CreateCoordinationLiteralFixture(dimensions[0], dimensions[1], dimensions[2]);
      REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(fixture.featureIdsPath));
      const auto& featureIdsArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(fixture.featureIdsPath);
      REQUIRE(featureIdsArrayRef.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);

      ExecuteCoordinationLiteralFixture(fixture);

      REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(fixture.featureIdsPath));
      const auto& actualFeatureIdsArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(fixture.featureIdsPath);
      REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(fixture.transferredValuesPath));
      const auto& actualTransferredValuesArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(fixture.transferredValuesPath);
      REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(fixture.ignoredValuesPath));
      const auto& actualIgnoredValuesArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(fixture.ignoredValuesPath);
      CheckCoordinationValues(actualFeatureIdsArrayRef, k_ExpectedFeatureIds);
      CheckCoordinationValues(actualTransferredValuesArrayRef, k_ExpectedTransferredValues);
      CheckCoordinationValues(actualIgnoredValuesArrayRef, k_ExpectedIgnoredValues);
    }
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Z Decisions and Threshold Controls", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);

  SECTION("Distinct positive IDs expose stale Z decisions")
  {
    auto fixture = CreateCoordinationLiteralFixture(1, 1, 3, {1, 0, 2});
    ExecuteCoordinationLiteralFixture(fixture);
    // Cell 1 sees the zero from cell 0, so Feature ID 2 supplies its only positive neighbor.
    CheckCoordinationValues(fixture, fixture.featureIdsPath, std::array<int32, 3>{0, 2, 2});
    CheckCoordinationValues(fixture, fixture.transferredValuesPath, std::array<int32, 3>{20, 30, 30});
    CheckCoordinationValues(fixture, fixture.ignoredValuesPath, std::array<int32, 3>{100, 200, 300});
  }

  SECTION("Ignoring FeatureIds keeps decisions fixed while sibling copies remain immediate")
  {
    auto fixture = CreateCoordinationLiteralFixture(4, 1, 1);
    fixture.ignoredPaths.push_back(fixture.featureIdsPath);
    ExecuteCoordinationLiteralFixture(fixture);
    CheckCoordinationValues(fixture, fixture.featureIdsPath, std::array<int32, 4>{1, 0, 0, 1});
    CheckCoordinationValues(fixture, fixture.transferredValuesPath, std::array<int32, 4>{20, 20, 40, 40});
    CheckCoordinationValues(fixture, fixture.ignoredValuesPath, std::array<int32, 4>{100, 200, 300, 400});
  }

  SECTION("Threshold two retains the last zero-neighbor source")
  {
    auto fixture = CreateCoordinationLiteralFixture(3, 1, 1, {0, 1, 0});
    ExecuteCoordinationLiteralFixture(fixture, 2);
    CheckCoordinationValues(fixture, fixture.featureIdsPath, std::array<int32, 3>{0, 0, 0});
    CheckCoordinationValues(fixture, fixture.transferredValuesPath, std::array<int32, 3>{10, 30, 30});
    CheckCoordinationValues(fixture, fixture.ignoredValuesPath, std::array<int32, 3>{100, 200, 300});
  }

  SECTION("Threshold zero does not transfer cells without an opposite-state neighbor")
  {
    auto fixture = CreateCoordinationLiteralFixture(3, 1, 1, {1, 1, 1});
    ExecuteCoordinationLiteralFixture(fixture, 0);
    CheckCoordinationValues(fixture, fixture.featureIdsPath, std::array<int32, 3>{1, 1, 1});
    CheckCoordinationValues(fixture, fixture.transferredValuesPath, std::array<int32, 3>{10, 20, 30});
    CheckCoordinationValues(fixture, fixture.ignoredValuesPath, std::array<int32, 3>{100, 200, 300});
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Immediate Component Copy Chains", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);

  for(const auto& [label, dimensions] : std::array<std::pair<std::string_view, std::array<usize, 3>>, 3>{{{"XY", {2, 2, 1}}, {"XZ", {2, 1, 2}}, {"YZ", {1, 2, 2}}}})
  {
    DYNAMIC_SECTION(label)
    {
      auto fixture = CreateCoordinationLiteralFixture(dimensions[0], dimensions[1], dimensions[2], {0, 0, 1, 0});
      const DataPath componentPath = AddCoordinationSibling(fixture, "Numeric Components", k_ComponentInput, 2);
      const DataPath boolPath = AddCoordinationSibling(fixture, "Boolean Components", k_BoolInput, 2);
      const DataPath ignoredComponentPath = AddCoordinationSibling(fixture, "Ignored Numeric Components", k_ComponentInput, 2);
      const DataPath ignoredBoolPath = AddCoordinationSibling(fixture, "Ignored Boolean Components", k_BoolInput, 2);
      fixture.ignoredPaths.push_back(ignoredComponentPath);
      fixture.ignoredPaths.push_back(ignoredBoolPath);

      REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Float32Array>(componentPath));
      REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Float32Array>(ignoredComponentPath));
      REQUIRE(fixture.dataStructure.getDataRefAs<Float32Array>(componentPath).getIDataStore() != fixture.dataStructure.getDataRefAs<Float32Array>(ignoredComponentPath).getIDataStore());
      REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<BoolArray>(boolPath));
      REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<BoolArray>(ignoredBoolPath));
      REQUIRE(fixture.dataStructure.getDataRefAs<BoolArray>(boolPath).getIDataStore() != fixture.dataStructure.getDataRefAs<BoolArray>(ignoredBoolPath).getIDataStore());

      ExecuteCoordinationLiteralFixture(fixture);
      // Cells copy sources 2, 0, 3, 1 in order. The original source tuples are therefore 2, 2, 3, 2.
      CheckCoordinationValues(fixture, fixture.featureIdsPath, std::array<int32, 4>{1, 1, 0, 1});
      CheckCoordinationValues(fixture, fixture.transferredValuesPath, std::array<int32, 4>{30, 30, 40, 30});
      CheckCoordinationValues(fixture, componentPath, k_ComponentExpected, 2);
      CheckCoordinationValues(fixture, boolPath, k_BoolExpected, 2);
      CheckCoordinationValues(fixture, ignoredComponentPath, k_ComponentInput, 2);
      CheckCoordinationValues(fixture, ignoredBoolPath, k_BoolInput, 2);
      CheckCoordinationValues(fixture, fixture.ignoredValuesPath, std::array<int32, 4>{100, 200, 300, 400});
    }
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Shared Store Aliases", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);

  struct AliasCase
  {
    std::string_view name;
    bool aliasesFeatureIds;
    bool ignoresOriginal;
    bool ignoresAlias;
  };
  const std::array<AliasCase, 6> aliasCases = {{{"Selected scalar and component aliases", false, false, false},
                                                {"Selected FeatureIds and selected alias", true, false, false},
                                                {"Ignored FeatureIds and selected alias", true, true, false},
                                                {"Selected FeatureIds and ignored alias", true, false, true},
                                                {"FeatureIds and alias both ignored", true, true, true},
                                                {"Selected scalar and ignored alias", false, false, true}}};

  for(const auto& aliasCase : aliasCases)
  {
    DYNAMIC_SECTION(aliasCase.name)
    {
      auto fixture = CreateCoordinationLiteralFixture(2, 2, 1, {0, 0, 1, 0});
      const DataPath originalPath = aliasCase.aliasesFeatureIds ? fixture.featureIdsPath : fixture.transferredValuesPath;
      const DataPath aliasPath = AddCoordinationAlias<int32>(fixture, originalPath);
      if(aliasCase.ignoresOriginal)
      {
        fixture.ignoredPaths.push_back(originalPath);
      }
      if(aliasCase.ignoresAlias)
      {
        fixture.ignoredPaths.push_back(aliasPath);
      }

      DataPath componentPath;
      DataPath componentAliasPath;
      DataPath boolPath;
      DataPath boolAliasPath;
      if(!aliasCase.aliasesFeatureIds && !aliasCase.ignoresAlias)
      {
        componentPath = AddCoordinationSibling(fixture, "Numeric Components", k_ComponentInput, 2);
        componentAliasPath = AddCoordinationAlias<float32>(fixture, componentPath);
        boolPath = AddCoordinationSibling(fixture, "Boolean Components", k_BoolInput, 2);
        boolAliasPath = AddCoordinationAlias<bool>(fixture, boolPath);
      }

      ExecuteCoordinationLiteralFixture(fixture);
      const bool featureIdsStayFixed = aliasCase.aliasesFeatureIds && aliasCase.ignoresOriginal && aliasCase.ignoresAlias;
      const std::array<int32, 4> expectedFeatureIds = featureIdsStayFixed ? std::array<int32, 4>{0, 0, 1, 0} : std::array<int32, 4>{1, 1, 0, 1};
      const std::array<int32, 4> expectedScalars = featureIdsStayFixed ? std::array<int32, 4>{30, 20, 40, 40} : std::array<int32, 4>{30, 30, 40, 30};
      CheckCoordinationValues(fixture, fixture.featureIdsPath, expectedFeatureIds);
      CheckCoordinationValues(fixture, fixture.transferredValuesPath, expectedScalars);
      CheckCoordinationValues(fixture, aliasPath, aliasCase.aliasesFeatureIds ? expectedFeatureIds : expectedScalars);
      CheckCoordinationValues(fixture, fixture.ignoredValuesPath, std::array<int32, 4>{100, 200, 300, 400});
      if(!componentPath.empty())
      {
        CheckCoordinationValues(fixture, componentPath, k_ComponentExpected, 2);
        CheckCoordinationValues(fixture, componentAliasPath, k_ComponentExpected, 2);
        CheckCoordinationValues(fixture, boolPath, k_BoolExpected, 2);
        CheckCoordinationValues(fixture, boolAliasPath, k_BoolExpected, 2);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Bulk Storage Failures", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  using Fault = CoordinationBulkFailureStore::Fault;
  const std::array<std::pair<std::string_view, Fault>, 6> faultCases = {{{"Success control", Fault::None},
                                                                         {"FeatureIds initial read", Fault::InitialFeatureIdsRead},
                                                                         {"Sibling destination read", Fault::DestinationRead},
                                                                         {"Sibling next-Z read", Fault::NextSliceRead},
                                                                         {"Sibling previous-Z read after committed slice", Fault::PreviousSliceRead},
                                                                         {"Sibling destination write", Fault::DestinationWrite}}};

  for(const auto& [label, fault] : faultCases)
  {
    DYNAMIC_SECTION(label)
    {
      auto fixture = CreateCoordinationLiteralFixture(2, 1, 2, {0, 0, 1, 0});
      const bool failsFeatureIds = fault == Fault::InitialFeatureIdsRead;
      const DataPath faultArrayPath = failsFeatureIds ? fixture.featureIdsPath : fixture.transferredValuesPath;
      auto faultStore = std::make_shared<CoordinationBulkFailureStore>();
      const std::array<int32, 4> inputValues = failsFeatureIds ? std::array<int32, 4>{0, 0, 1, 0} : std::array<int32, 4>{10, 20, 30, 40};
      auto faultStoreWriteResult = faultStore->copyFromBuffer(0, nonstd::span<const int32>(inputValues.data(), inputValues.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(faultStoreWriteResult);
      REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Int32Array>(faultArrayPath));
      auto& faultArrayRef = fixture.dataStructure.getDataRefAs<Int32Array>(faultArrayPath);
      auto faultArrayRefSetStoreResult = faultArrayRef.setDataStore(faultStore);
      SIMPLNX_RESULT_REQUIRE_VALID(faultArrayRefSetStoreResult);
      REQUIRE(faultArrayRef.getIDataStore() == faultStore.get());
      REQUIRE(faultArrayRef.getStoreType() == IDataStore::StoreType::InMemory);

      // Setup writes must not satisfy the condition that enables the previous-slice fault.
      faultStore->arm(fault);
      const Result<> result = RunCoordinationLiteralFixture(fixture);
      INFO(fmt::format("{}: reads={} writes={} completedWrites={} rejected={} rejectedOffset={} nextReadAfterWrite={}", label, faultStore->getReadCount(), faultStore->getWriteCount(),
                       faultStore->getCompletedWriteCount(), faultStore->getFailureCount(), faultStore->getRejectedOffset(), faultStore->hasReadNextAfterWrite()));
      if(fault == Fault::None)
      {
        SIMPLNX_RESULT_REQUIRE_VALID(result);
        CHECK(faultStore->getFailureCount() == 0);
        CHECK(faultStore->getCompletedWriteCount() == 2);
        CheckCoordinationValues(fixture, fixture.featureIdsPath, std::array<int32, 4>{1, 1, 0, 1});
        CheckCoordinationValues(fixture, fixture.transferredValuesPath, std::array<int32, 4>{30, 30, 40, 30});
      }
      else
      {
        REQUIRE(result.invalid());
        REQUIRE(result.errors().size() == 1);
        CHECK(result.errors()[0].code == CoordinationBulkFailureStore::k_ErrorCode);
        CHECK(result.errors()[0].message == CoordinationBulkFailureStore::k_ErrorMessage);
        CHECK(faultStore->getFailureCount() == 1);
        CHECK(faultStore->getRejectedOffset() == (fault == Fault::NextSliceRead ? 2 : 0));
        if(fault == Fault::PreviousSliceRead)
        {
          CHECK(faultStore->getCompletedWriteCount() == 1);
          CHECK(faultStore->hasReadNextAfterWrite());
        }
        else
        {
          CHECK(faultStore->getCompletedWriteCount() == 0);
          if(fault == Fault::InitialFeatureIdsRead || fault == Fault::DestinationRead)
          {
            CHECK(faultStore->getReadCount() == 1);
            CHECK(faultStore->getWriteCount() == 0);
          }
          if(fault == Fault::NextSliceRead)
          {
            CHECK(faultStore->getReadCount() == 2);
            CHECK(faultStore->getWriteCount() == 0);
          }
          if(fault == Fault::DestinationWrite)
          {
            CHECK(faultStore->getWriteCount() == 1);
          }
        }
      }
      CheckCoordinationValues(fixture, fixture.ignoredValuesPath, std::array<int32, 4>{100, 200, 300, 400});
      UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ErodeDilateCoordinationNumberFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";
  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ErodeDilateCoordinationNumberFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ErodeDilateCoordinationNumberFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      const auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      const auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);
      const auto* pipelineFilter = dynamic_cast<const PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);
      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ErodeDilateCoordinationNumberFilter>::uuid);
      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int32>(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key) == 5);
      CHECK(args.value<bool>(ErodeDilateCoordinationNumberFilter::k_Loop_Key));
      CHECK(args.value<DataPath>(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

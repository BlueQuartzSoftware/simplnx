#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// These constants define paths used by the loading API scenarios.
constexpr StringLiteral k_GroupName = "TopGroup";
constexpr StringLiteral k_SmallAttrMatName = "SmallAM";
constexpr StringLiteral k_LargeAttrMatName = "LargeAM";
constexpr StringLiteral k_SmallArrayName = "SmallArray";
constexpr StringLiteral k_LargeArrayName = "LargeArray";

constexpr usize k_SmallArraySize = 10;
constexpr usize k_LargeArraySize = 100;

// These paths select arrays and their ancestor containers.
const DataPath k_GroupPath({k_GroupName});
const DataPath k_SmallAMPath({k_GroupName, k_SmallAttrMatName});
const DataPath k_LargeAMPath({k_GroupName, k_LargeAttrMatName});
const DataPath k_SmallArrayPath({k_GroupName, k_SmallAttrMatName, k_SmallArrayName});
const DataPath k_LargeArrayPath({k_GroupName, k_LargeAttrMatName, k_LargeArrayName});

// These paths select the multi-group prune fixture.
constexpr StringLiteral k_GroupAName = "GroupA";
constexpr StringLiteral k_GroupBName = "GroupB";
constexpr StringLiteral k_AttrMatAName = "AttrMatA";
constexpr StringLiteral k_AttrMatBName = "AttrMatB";
constexpr StringLiteral k_ArrayA1Name = "ArrayA1";
constexpr StringLiteral k_ArrayA2Name = "ArrayA2";
constexpr StringLiteral k_ArrayB1Name = "ArrayB1";
constexpr StringLiteral k_ArrayB2Name = "ArrayB2";
constexpr usize k_PruneArraySize = 20;

const DataPath k_GroupAPath({k_GroupAName});
const DataPath k_AttrMatAPath({k_GroupAName, k_AttrMatAName});
const DataPath k_ArrayA1Path({k_GroupAName, k_AttrMatAName, k_ArrayA1Name});
const DataPath k_ArrayA2Path({k_GroupAName, k_AttrMatAName, k_ArrayA2Name});
const DataPath k_GroupBPath({k_GroupBName});
const DataPath k_AttrMatBPath({k_GroupBName, k_AttrMatBName});
const DataPath k_ArrayB1Path({k_GroupBName, k_AttrMatBName, k_ArrayB1Name});
const DataPath k_ArrayB2Path({k_GroupBName, k_AttrMatBName, k_ArrayB2Name});

// The helpers create and clean temporary DREAM3D files.

/**
 * @struct ScopedTempFile
 * @brief Removes one temporary file when the guard is destroyed.
 *
 * Ensures test-output files are cleaned up even when a REQUIRE assertion
 * throws and skips the remaining test body.
 */
struct ScopedTempFile
{
  explicit ScopedTempFile(fs::path p)
  : path(std::move(p))
  {
  }
  ~ScopedTempFile()
  {
    std::error_code ec;
    fs::remove(path, ec);
  }
  fs::path path;
};

/**
 * @brief Returns the binary test-output directory.
 * @return Directory for temporary DREAM3D files.
 */
fs::path GetTestOutputDir()
{
  return fs::path(unit_test::k_BinaryTestOutputDir.view());
}

/**
 * @brief Creates a two-array DataStructure for loading tests.
 * @return A hierarchy with independent small and large AttributeMatrices.
 *
 * Separate AttributeMatrices permit different tuple dimensions. SmallArray
 * values use `i * 3`; LargeArray values use `i * 1.5`.
 */
DataStructure CreateSimpleTestDataStructure()
{
  DataStructure ds;

  auto* group = DataGroup::Create(ds, k_GroupName);
  REQUIRE(group != nullptr);

  // SmallAM holds SmallArray (10 tuples)
  auto* smallAM = AttributeMatrix::Create(ds, k_SmallAttrMatName, {k_SmallArraySize}, group->getId());
  REQUIRE(smallAM != nullptr);

  auto smallStore = std::make_unique<DataStore<int32>>(std::vector<usize>{k_SmallArraySize}, std::vector<usize>{1}, static_cast<int32>(0));
  for(usize i = 0; i < k_SmallArraySize; ++i)
  {
    smallStore->setValue(i, static_cast<int32>(i * 3));
  }
  auto* smallArray = DataArray<int32>::Create(ds, k_SmallArrayName, std::move(smallStore), smallAM->getId());
  REQUIRE(smallArray != nullptr);

  // LargeAM holds LargeArray (100 tuples)
  auto* largeAM = AttributeMatrix::Create(ds, k_LargeAttrMatName, {k_LargeArraySize}, group->getId());
  REQUIRE(largeAM != nullptr);

  auto largeStore = std::make_unique<DataStore<float32>>(std::vector<usize>{k_LargeArraySize}, std::vector<usize>{1}, static_cast<float32>(0));
  for(usize i = 0; i < k_LargeArraySize; ++i)
  {
    largeStore->setValue(i, static_cast<float32>(i) * 1.5f);
  }
  auto* largeArray = DataArray<float32>::Create(ds, k_LargeArrayName, std::move(largeStore), largeAM->getId());
  REQUIRE(largeArray != nullptr);

  return ds;
}

/**
 * @brief Writes a DataStructure to a temporary DREAM3D file.
 * @param ds DataStructure to write.
 * @param fileName Output file name.
 * @return The written file path.
 */
fs::path WriteTestFile(const DataStructure& ds, const std::string& fileName)
{
  fs::path outputPath = GetTestOutputDir() / fileName;
  Result<> writeResult = DREAM3D::WriteFile(outputPath, ds);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  REQUIRE(fs::exists(outputPath));
  return outputPath;
}

/**
 * @brief Creates two groups with distinct integer and float arrays for pruning.
 * @return A populated multi-group DataStructure.
 */
DataStructure CreateMultiGroupTestDataStructure()
{
  DataStructure ds;

  auto* groupA = DataGroup::Create(ds, k_GroupAName);
  REQUIRE(groupA != nullptr);
  auto* attrMatA = AttributeMatrix::Create(ds, k_AttrMatAName, {k_PruneArraySize}, groupA->getId());
  REQUIRE(attrMatA != nullptr);

  auto storeA1 = std::make_unique<DataStore<int32>>(std::vector<usize>{k_PruneArraySize}, std::vector<usize>{1}, static_cast<int32>(0));
  for(usize i = 0; i < k_PruneArraySize; ++i)
  {
    storeA1->setValue(i, static_cast<int32>(i));
  }
  auto* arrayA1 = DataArray<int32>::Create(ds, k_ArrayA1Name, std::move(storeA1), attrMatA->getId());
  REQUIRE(arrayA1 != nullptr);

  auto storeA2 = std::make_unique<DataStore<int32>>(std::vector<usize>{k_PruneArraySize}, std::vector<usize>{1}, static_cast<int32>(0));
  for(usize i = 0; i < k_PruneArraySize; ++i)
  {
    storeA2->setValue(i, static_cast<int32>(i * 2));
  }
  auto* arrayA2 = DataArray<int32>::Create(ds, k_ArrayA2Name, std::move(storeA2), attrMatA->getId());
  REQUIRE(arrayA2 != nullptr);

  auto* groupB = DataGroup::Create(ds, k_GroupBName);
  REQUIRE(groupB != nullptr);
  auto* attrMatB = AttributeMatrix::Create(ds, k_AttrMatBName, {k_PruneArraySize}, groupB->getId());
  REQUIRE(attrMatB != nullptr);

  auto storeB1 = std::make_unique<DataStore<float32>>(std::vector<usize>{k_PruneArraySize}, std::vector<usize>{1}, static_cast<float32>(0));
  for(usize i = 0; i < k_PruneArraySize; ++i)
  {
    storeB1->setValue(i, static_cast<float32>(i) * 0.5f);
  }
  auto* arrayB1 = DataArray<float32>::Create(ds, k_ArrayB1Name, std::move(storeB1), attrMatB->getId());
  REQUIRE(arrayB1 != nullptr);

  auto storeB2 = std::make_unique<DataStore<float32>>(std::vector<usize>{k_PruneArraySize}, std::vector<usize>{1}, static_cast<float32>(0));
  for(usize i = 0; i < k_PruneArraySize; ++i)
  {
    storeB2->setValue(i, static_cast<float32>(i) * 0.25f);
  }
  auto* arrayB2 = DataArray<float32>::Create(ds, k_ArrayB2Name, std::move(storeB2), attrMatB->getId());
  REQUIRE(arrayB2 != nullptr);

  return ds;
}
} // namespace

// =============================================================================
// Test Scenarios
// =============================================================================

TEST_CASE("Dream3dLoadingApi: LoadDataStructure loads all arrays")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_LoadAll.dream3d");

  Result<DataStructure> result = DREAM3D::LoadDataStructure(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  const auto& smallPath = k_SmallArrayPath;
  const auto& largePath = k_LargeArrayPath;

  // Both arrays must exist
  auto* smallArray = ds.getDataAs<Int32Array>(smallPath);
  REQUIRE(smallArray != nullptr);
  REQUIRE(smallArray->getNumberOfTuples() == k_SmallArraySize);

  auto* largeArray = ds.getDataAs<Float32Array>(largePath);
  REQUIRE(largeArray != nullptr);
  REQUIRE(largeArray->getNumberOfTuples() == k_LargeArraySize);

  // Verify SmallArray values
  const auto& smallStore = smallArray->getDataStoreRef();
  for(usize i = 0; i < k_SmallArraySize; ++i)
  {
    CHECK(smallStore[i] == static_cast<int32>(i * 3));
  }

  // Verify LargeArray values
  const auto& largeStore = largeArray->getDataStoreRef();
  for(usize i = 0; i < k_LargeArraySize; ++i)
  {
    CHECK(largeStore[i] == Approx(static_cast<float32>(i) * 1.5f));
  }
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructureArrays loads only requested arrays")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_Selective.dream3d");

  const auto& smallPath = k_SmallArrayPath;
  const auto& largePath = k_LargeArrayPath;

  Result<DataStructure> result = DREAM3D::LoadDataStructureArrays(filePath, {smallPath});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  // SmallArray must exist with correct data
  auto* smallArray = ds.getDataAs<Int32Array>(smallPath);
  REQUIRE(smallArray != nullptr);
  REQUIRE(smallArray->getNumberOfTuples() == k_SmallArraySize);

  const auto& smallStore = smallArray->getDataStoreRef();
  for(usize i = 0; i < k_SmallArraySize; ++i)
  {
    CHECK(smallStore[i] == static_cast<int32>(i * 3));
  }

  // LargeArray must NOT exist
  auto* largeArray = ds.getDataAs<Float32Array>(largePath);
  CHECK(largeArray == nullptr);

  // Ancestor containers must exist
  CHECK(ds.getDataAs<DataGroup>(k_GroupPath) != nullptr);
  CHECK(ds.getDataAs<AttributeMatrix>(k_SmallAMPath) != nullptr);
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructureArraysMetadata loads only requested metadata")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_SelectiveMeta.dream3d");

  const auto& smallPath = k_SmallArrayPath;
  const auto& largePath = k_LargeArrayPath;

  Result<DataStructure> result = DREAM3D::LoadDataStructureArraysMetadata(filePath, {smallPath});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  // SmallArray must exist
  auto* smallArray = ds.getDataAs<IDataArray>(smallPath);
  REQUIRE(smallArray != nullptr);

  // SmallArray store must be Empty (no data loaded)
  CHECK(smallArray->getStoreType() == IDataStore::StoreType::Empty);

  // LargeArray must NOT exist
  auto* largeArray = ds.getDataAs<IDataArray>(largePath);
  CHECK(largeArray == nullptr);

  // Ancestor containers must exist
  CHECK(ds.getDataAs<DataGroup>(k_GroupPath) != nullptr);
  CHECK(ds.getDataAs<AttributeMatrix>(k_SmallAMPath) != nullptr);
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructureMetadata loads all metadata")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_AllMeta.dream3d");

  Result<DataStructure> result = DREAM3D::LoadDataStructureMetadata(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  const auto& smallPath = k_SmallArrayPath;
  const auto& largePath = k_LargeArrayPath;

  // Both arrays must exist
  auto* smallArray = ds.getDataAs<IDataArray>(smallPath);
  REQUIRE(smallArray != nullptr);
  auto* largeArray = ds.getDataAs<IDataArray>(largePath);
  REQUIRE(largeArray != nullptr);

  // Both must have Empty stores (no data loaded)
  CHECK(smallArray->getStoreType() == IDataStore::StoreType::Empty);
  CHECK(largeArray->getStoreType() == IDataStore::StoreType::Empty);
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructure with invalid path returns error")
{
  const fs::path bogusPath("/tmp/nonexistent_dream3d_file_12345.dream3d");

  // Suppress HDF5 error output temporarily
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);
  Result<DataStructure> result = DREAM3D::LoadDataStructure(bogusPath);
  // Restore default error handling
  H5Eset_auto(H5E_DEFAULT, (H5E_auto_t)H5Eprint, stderr);

  REQUIRE(result.invalid());
  REQUIRE(!result.errors().empty());
  CHECK(result.errors()[0].code == -1);
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructure with legacy file")
{
  const fs::path legacyPath = fs::path(unit_test::k_SourceDir.view()) / "test" / "Data" / "LegacyData.dream3d";
  REQUIRE(fs::exists(legacyPath));

  Result<DataStructure> result = DREAM3D::LoadDataStructure(legacyPath);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Verify the DataStructure has content (legacy files should load successfully)
  const DataStructure& ds = result.value();
  CHECK(!ds.getAllDataPaths().empty());
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructureArrays prune verification")
{
  DataStructure srcDs = CreateMultiGroupTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_Prune.dream3d");

  Result<DataStructure> result = DREAM3D::LoadDataStructureArrays(filePath, {k_ArrayA1Path});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  // ArrayA1 and its ancestors must exist
  auto* arrayA1 = ds.getDataAs<Int32Array>(k_ArrayA1Path);
  REQUIRE(arrayA1 != nullptr);
  REQUIRE(arrayA1->getNumberOfTuples() == k_PruneArraySize);
  CHECK(ds.getDataAs<DataGroup>(k_GroupAPath) != nullptr);
  CHECK(ds.getDataAs<AttributeMatrix>(k_AttrMatAPath) != nullptr);

  // ArrayA2 must NOT exist (same group, but not requested)
  CHECK(ds.getDataAs<Int32Array>(k_ArrayA2Path) == nullptr);

  // GroupB and its children must NOT exist
  CHECK(ds.getDataAs<DataGroup>(k_GroupBPath) == nullptr);
  CHECK(ds.getDataAs<Float32Array>(k_ArrayB1Path) == nullptr);
  CHECK(ds.getDataAs<Float32Array>(k_ArrayB2Path) == nullptr);
}

TEST_CASE("Dream3dLoadingApi: Recovery file with user data path redirect")
{
  // RAII guards ensure cleanup even when a REQUIRE throws on failure.
  ScopedTempFile filePathGuard{GetTestOutputDir() / "Dream3dLoadingApiTest_RecoveryRedirect.dream3d"};
  const fs::path& filePath = filePathGuard.path;
  const fs::path userDataPath = GetTestOutputDir() / "my_user_output.dream3d";

  // Write the minimal redirect variant — dataStructure/pipeline ignored.
  DataStructure emptyDs;
  Result<> writeResult = DREAM3D::WriteRecoveryFile(filePath, emptyDs, {}, userDataPath);
  REQUIRE(writeResult.valid());

  // File exists and is small (minimal variant is kilobytes, full variant is
  // MB-GB with real data). The sanity check is "< 64 KB" to catch a
  // regression where WriteRecoveryFile silently falls through to the full
  // path despite userDataFilePath being set.
  REQUIRE(fs::exists(filePath));
  REQUIRE(fs::file_size(filePath) < 64 * 1024);

  // Read back: attribute should contain the ABSOLUTE form of userDataPath.
  auto readResult = DREAM3D::ReadUserDataFilePathAttribute(filePath);
  REQUIRE(readResult.valid());
  REQUIRE(readResult.value().has_value());
  REQUIRE(readResult.value().value() == fs::absolute(userDataPath));

  // A standard recovery file (no redirect) should return nullopt.
  ScopedTempFile standardFilePathGuard{GetTestOutputDir() / "Dream3dLoadingApiTest_RecoveryStandard.dream3d"};
  const fs::path& standardFilePath = standardFilePathGuard.path;
  DataStructure simpleDs = CreateSimpleTestDataStructure();
  REQUIRE(DREAM3D::WriteRecoveryFile(standardFilePath, simpleDs).valid());

  auto readStandard = DREAM3D::ReadUserDataFilePathAttribute(standardFilePath);
  REQUIRE(readStandard.valid());
  REQUIRE_FALSE(readStandard.value().has_value());
}

TEST_CASE("Dream3dLoadingApi: Recovery file with all in-core data")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = GetTestOutputDir() / "Dream3dLoadingApiTest_Recovery.dream3d";

  // WriteRecoveryFile writes in-core stores' full data inline (only OOC stores
  // are written as lightweight placeholders pointing at their backing files).
  // This DataStructure is entirely in-core, so all array data is stored inline
  // in the recovery file.
  Result<> writeResult = DREAM3D::WriteRecoveryFile(filePath, srcDs);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  REQUIRE(fs::exists(filePath));

  Result<DataStructure> result = DREAM3D::LoadDataStructure(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  const auto& smallPath = k_SmallArrayPath;

  // Verify data integrity through the recovery round-trip
  const auto& smallStore = ds.getDataRefAs<Int32Array>(smallPath).getDataStoreRef();
  for(usize i = 0; i < k_SmallArraySize; ++i)
  {
    CHECK(smallStore[i] == static_cast<int32>(i * 3));
  }
}

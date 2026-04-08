#include "SimplnxCore/Filters/AlignSectionsListFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_AlignmentAMPath = Constants::k_DataContainerPath.createChildPath(Constants::k_AlignmentAMName);

struct CompareArraysFunctor
{
  template <typename T>
  void operator()(const IDataArray& computedArray, const IDataArray& exemplarArray)
  {
    UnitTest::CompareDataArrays<T>(computedArray, exemplarArray);
  }
};
} // namespace

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Relative Shifts execution", "[SimplnxCore][AlignSectionsListFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 600000, true);

  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOoc = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOoc);

  auto app = Application::GetOrCreateInstance();
  auto* filterList = app->getFilterList();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));
  UnitTest::RequireExpectedStoreType(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath newShiftsPath = DataPath({Constants::k_RelativeShiftsArrayName});
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_RelativeShiftsArrayName)));
  auto& exemplarShifts = exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_RelativeShiftsArrayName));
  UnitTest::CreateTestDataArray<int64>(dataStructure, Constants::k_RelativeShiftsArrayName, exemplarShifts.getTupleShape(), exemplarShifts.getComponentShape());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(newShiftsPath));
  auto& newShifts = dataStructure.getDataRefAs<Int64Array>(newShiftsPath);
  CopyFromArray::CopyData(exemplarShifts, newShifts, 0ULL, 0ULL, exemplarShifts.getNumberOfTuples());

  // MultiThreshold Objects Filter (From SimplnxCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Align Sections List Filter
  {
    // Instantiate the filter and an Arguments Object
    AlignSectionsListFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));

    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::Constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(newShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath);
  const auto& cellAttributeMatrix = imageGeom.getCellData();
  std::optional<std::vector<DataPath>> selectedCellArrays = GetAllChildDataPaths(dataStructure, nx::core::Constants::k_DataContainerPath.createChildPath(cellAttributeMatrix->getName()));
  REQUIRE(selectedCellArrays.has_value());

  for(const auto& path : selectedCellArrays.value())
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(path));
    REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(path));
    const auto& computedIDataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& exemplarIDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(path);

    ExecuteDataFunction(::CompareArraysFunctor{}, computedIDataArray.getDataType(), computedIDataArray, exemplarIDataArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_list/relative_align_sections_list.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Cumulative Shifts execution", "[SimplnxCore][AlignSectionsListFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 600000, true);

  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOoc = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOoc);

  auto* filterList = Application::GetOrCreateInstance()->getFilterList();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));
  UnitTest::RequireExpectedStoreType(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath newShiftsPath = DataPath({Constants::k_CumulativeShiftsArrayName});
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_CumulativeShiftsArrayName)));
  auto& exemplarShifts = exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_CumulativeShiftsArrayName));
  UnitTest::CreateTestDataArray<int64>(dataStructure, Constants::k_CumulativeShiftsArrayName, exemplarShifts.getTupleShape(), exemplarShifts.getComponentShape());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(newShiftsPath));
  auto& newShifts = dataStructure.getDataRefAs<Int64Array>(newShiftsPath);
  CopyFromArray::CopyData(exemplarShifts, newShifts, 0ULL, 0ULL, exemplarShifts.getNumberOfTuples());

  // MultiThreshold Objects Filter (From SimplnxCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Align Sections List Filter
  {
    // Instantiate the filter and an Arguments Object
    AlignSectionsListFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));

    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::Constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(newShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath);
  const auto& cellAttributeMatrix = imageGeom.getCellData();
  std::optional<std::vector<DataPath>> selectedCellArrays = GetAllChildDataPaths(dataStructure, nx::core::Constants::k_DataContainerPath.createChildPath(cellAttributeMatrix->getName()));
  REQUIRE(selectedCellArrays.has_value());

  for(const auto& path : selectedCellArrays.value())
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(path));
    REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(path));
    const auto& computedIDataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& exemplarIDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(path);

    ExecuteDataFunction(::CompareArraysFunctor{}, computedIDataArray.getDataType(), computedIDataArray, exemplarIDataArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_list/cumulative_align_sections_list.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Benchmark 200x200x200", "[SimplnxCore][AlignSectionsListFilter][.Benchmark]")
{
  UnitTest::LoadPlugins();
  // 200x200x200, largest cell array is EulerAngles float32 3-comp => 200*200*3*4 = 480,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 480000, true);
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOoc = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOoc);

  constexpr usize kDimX = 200;
  constexpr usize kDimY = 200;
  constexpr usize kDimZ = 200;
  constexpr usize kSliceVoxels = kDimX * kDimY;
  const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};
  const auto benchmarkFile = fs::path(fmt::format("{}/align_sections_list_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  // Stage 1: Build data programmatically and write to .dream3d
  {
    DataStructure buildDS;
    auto* imageGeom = ImageGeom::Create(buildDS, "DataContainer");
    imageGeom->setDimensions({kDimX, kDimY, kDimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    auto* cellAM = AttributeMatrix::Create(buildDS, "CellData", cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    auto* eulerArray = UnitTest::CreateTestDataArray<float32>(buildDS, "EulerAngles", cellTupleShape, {3}, cellAM->getId());
    auto& eulerStore = eulerArray->getDataStoreRef();
    auto* featureIdsArray = UnitTest::CreateTestDataArray<int32>(buildDS, "FeatureIds", cellTupleShape, {1}, cellAM->getId());
    auto& featureIdsStore = featureIdsArray->getDataStoreRef();
    auto* maskArray = UnitTest::CreateTestDataArray<uint8>(buildDS, "Mask", cellTupleShape, {1}, cellAM->getId());
    auto& maskStore = maskArray->getDataStoreRef();

    // Fill using slice-at-a-time bulk writes
    std::vector<float32> eulerBuf(kSliceVoxels * 3);
    std::vector<int32> featureIdsBuf(kSliceVoxels);
    std::vector<uint8> maskBuf(kSliceVoxels);

    for(usize z = 0; z < kDimZ; z++)
    {
      for(usize y = 0; y < kDimY; y++)
      {
        for(usize x = 0; x < kDimX; x++)
        {
          const usize localIdx = y * kDimX + x;
          eulerBuf[localIdx * 3 + 0] = static_cast<float32>(x) * 0.01f;
          eulerBuf[localIdx * 3 + 1] = static_cast<float32>(y) * 0.01f;
          eulerBuf[localIdx * 3 + 2] = static_cast<float32>(z) * 0.01f;
          featureIdsBuf[localIdx] = static_cast<int32>((x / 25) * 64 + (y / 25) * 8 + (z / 25));
          maskBuf[localIdx] = 1;
        }
      }
      eulerStore.copyFromBuffer(z * kSliceVoxels * 3, nonstd::span<const float32>(eulerBuf.data(), kSliceVoxels * 3));
      featureIdsStore.copyFromBuffer(z * kSliceVoxels, nonstd::span<const int32>(featureIdsBuf.data(), kSliceVoxels));
      maskStore.copyFromBuffer(z * kSliceVoxels, nonstd::span<const uint8>(maskBuf.data(), kSliceVoxels));
    }

    // Create shifts array (int64, 2-comp) at top level with varying shifts
    const ShapeType shiftsTupleShape = {kDimZ};
    auto* shiftsArray = UnitTest::CreateTestDataArray<int64>(buildDS, "RelativeShifts", shiftsTupleShape, {2});
    auto& shiftsStore = shiftsArray->getDataStoreRef();
    for(usize z = 0; z < kDimZ; z++)
    {
      shiftsStore[z * 2 + 0] = static_cast<int64>(3.0 * std::sin(static_cast<float64>(z) * 0.1));
      shiftsStore[z * 2 + 1] = static_cast<int64>(2.0 * std::cos(static_cast<float64>(z) * 0.07));
    }

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  // Stage 2: Reload (arrays become OOC-backed) and run filter
  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    AlignSectionsListFilter filter;
    Arguments args;

    args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
    args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(DataPath({"RelativeShifts"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);

  fs::remove(benchmarkFile);
}

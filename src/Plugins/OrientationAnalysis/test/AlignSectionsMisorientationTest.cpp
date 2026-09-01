#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/AlignSectionsMisorientationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_SmallGeomPath({"Small Image"});
const DataPath k_SmallCellPath = k_SmallGeomPath.createChildPath("Cell Data");
const DataPath k_SmallQuatsPath = k_SmallCellPath.createChildPath("Quats");
const DataPath k_SmallPhasesPath = k_SmallCellPath.createChildPath("Phases");
const DataPath k_SmallMaskPath = k_SmallCellPath.createChildPath("Mask");
const DataPath k_SmallNumericPath = k_SmallCellPath.createChildPath("Numeric Sibling");
const DataPath k_SmallBoolPath = k_SmallCellPath.createChildPath("Bool Sibling");
const DataPath k_SmallCrystalPath({"Crystal Structures"});

void CreateSmallFixture(DataStructure& dataStructure)
{
  const ShapeType cellShape = {2, 8, 8};
  constexpr usize kTupleCount = 2 * 8 * 8;
  auto* image = ImageGeom::Create(dataStructure, "Small Image");
  image->setDimensions({8, 8, 2});
  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", cellShape, image->getId());
  image->setCellData(*cellData);
  auto* quats = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Quats", cellShape, {4}, cellData->getId());
  auto* phases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Phases", cellShape, {1}, cellData->getId());
  auto* numeric = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Numeric Sibling", cellShape, {2}, cellData->getId());
  auto maskStore = std::make_shared<DataStore<bool>>(cellShape, ShapeType{1}, std::optional<bool>{});
  auto boolStore = std::make_shared<DataStore<bool>>(cellShape, ShapeType{1}, std::optional<bool>{});
  REQUIRE(quats != nullptr);
  REQUIRE(phases != nullptr);
  REQUIRE(numeric != nullptr);
  REQUIRE(BoolArray::Create(dataStructure, "Mask", maskStore, cellData->getId()) != nullptr);
  REQUIRE(BoolArray::Create(dataStructure, "Bool Sibling", boolStore, cellData->getId()) != nullptr);
  auto* crystal = UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, "Crystal Structures", {2}, {1});
  REQUIRE(crystal != nullptr);
  std::vector<float32> quatValues(kTupleCount * 4);
  std::vector<int32> phaseValues(kTupleCount);
  std::vector<float32> numericValues(kTupleCount * 2);
  auto boolValues = std::make_unique<bool[]>(kTupleCount);
  for(usize i = 0; i < kTupleCount; i++)
  {
    quatValues[i * 4] = 1.0F;
    phaseValues[i] = 1;
    numericValues[i * 2] = static_cast<float32>(i);
    numericValues[i * 2 + 1] = -static_cast<float32>(i);
    boolValues[i] = (i % 3) != 0;
  }
  const std::array<uint32, 2> crystalValues = {0, 1};
  SIMPLNX_RESULT_REQUIRE_VALID(quats->getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(quatValues.data(), quatValues.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(phases->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phaseValues.data(), phaseValues.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(numeric->getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(numericValues.data(), numericValues.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(maskStore->copyFromBuffer(0, nonstd::span<const bool>(boolValues.get(), kTupleCount)));
  SIMPLNX_RESULT_REQUIRE_VALID(boolStore->copyFromBuffer(0, nonstd::span<const bool>(boolValues.get(), kTupleCount)));
  SIMPLNX_RESULT_REQUIRE_VALID(crystal->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(crystalValues.data(), crystalValues.size())));
}

Arguments SmallArguments(const bool useMask)
{
  Arguments args;
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_SmallMaskPath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_SmallQuatsPath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_SmallPhasesPath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_SmallCrystalPath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SmallGeomPath));
  return args;
}
} // namespace

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientation: small direct and forced parity", "[OrientationAnalysis][AlignSectionsMisorientation]")
{
  UnitTest::LoadPlugins();
  for(const bool useMask : {false, true})
  {
    DataStructure directData;
    CreateSmallFixture(directData);
    UnitTest::AlgorithmTestScope directScope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
    AlignSectionsMisorientationFilter filter;
    SIMPLNX_RESULT_REQUIRE_VALID(directScope.executeFilter(filter, directData, SmallArguments(useMask)).result);
    DataStructure oocData;
    CreateSmallFixture(oocData);
    UnitTest::AlgorithmTestScope oocScope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    SIMPLNX_RESULT_REQUIRE_VALID(oocScope.executeFilter(filter, oocData, SmallArguments(useMask)).result);
    // Alignment transfers every cell-array type. Compare numeric and Boolean
    // siblings after forced out-of-core dispatch.
    UnitTest::CompareDataArrays<float32>(directData.getDataRefAs<IDataArray>(k_SmallNumericPath), oocData.getDataRefAs<IDataArray>(k_SmallNumericPath));
    UnitTest::CompareDataArrays<bool>(directData.getDataRefAs<IDataArray>(k_SmallBoolPath), oocData.getDataRefAs<IDataArray>(k_SmallBoolPath));
  }
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientation Small IN100 Pipeline", "[OrientationAnalysis][AlignSectionsMisorientation]")
{
  UnitTest::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  const std::string kDataInputArchive2 = "align_sections.tar.gz";
  const std::string kExpectedOutputTopLevel2 = "align_sections_misorientation.txt";
  const nx::core::UnitTest::TestFileSentinel testDataSentinel2(nx::core::unit_test::k_TestFilesDir, kDataInputArchive2, kExpectedOutputTopLevel2);

  auto* filterList = Application::Instance()->getFilterList();

  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/6_6_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));

  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  {
    Arguments args;
    AlignSectionsMisorientationFilter filter;

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, Constants::k_CellAttributeMatrix, Constants::k_ExemplarDataContainer);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_misorientation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: output test", "[Reconstruction][AlignSectionsMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  auto* filterList = Application::Instance()->getFilterList();

  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));

  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  {
    Arguments args;
    AlignSectionsMisorientationFilter filter;

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_AlignmentAMName_Key, std::make_any<std::string>(Constants::k_AlignmentAMName));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SlicesArrayName_Key, std::make_any<std::string>(Constants::k_SlicesArrayName));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_RelativeShiftsArrayName_Key, std::make_any<std::string>(Constants::k_RelativeShiftsArrayName));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>(Constants::k_CumulativeShiftsArrayName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath alignmentAMPath = Constants::k_DataContainerPath.createChildPath(Constants::k_AlignmentAMName);

  const DataPath slicesPath = alignmentAMPath.createChildPath(Constants::k_SlicesArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(slicesPath));
  UnitTest::CompareDataArrays<uint32>(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath), dataStructure.getDataRefAs<IDataArray>(slicesPath));

  const DataPath relativeShiftsPath = alignmentAMPath.createChildPath(Constants::k_RelativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  UnitTest::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));

  const DataPath cumulativeShiftsPath = alignmentAMPath.createChildPath(Constants::k_CumulativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  UnitTest::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/output_align_sections_misorientation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

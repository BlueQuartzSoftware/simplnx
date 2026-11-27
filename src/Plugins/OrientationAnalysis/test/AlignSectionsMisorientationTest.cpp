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
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
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
  constexpr usize k_TupleCount = 2 * 8 * 8;
  auto* imagePtr = ImageGeom::Create(dataStructure, "Small Image");
  imagePtr->setDimensions({8, 8, 2});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", cellShape, imagePtr->getId());
  imagePtr->setCellData(*cellDataPtr);
  auto* quatsPtr = Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, "Quats", cellShape, {4}, cellDataPtr->getId());
  auto* phasesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Phases", cellShape, {1}, cellDataPtr->getId());
  auto* numericPtr = Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, "Numeric Sibling", cellShape, {2}, cellDataPtr->getId());
  auto maskStore = std::make_shared<DataStore<bool>>(cellShape, ShapeType{1}, std::optional<bool>{});
  auto boolStore = std::make_shared<DataStore<bool>>(cellShape, ShapeType{1}, std::optional<bool>{});
  REQUIRE(quatsPtr != nullptr);
  REQUIRE(phasesPtr != nullptr);
  REQUIRE(numericPtr != nullptr);
  REQUIRE(BoolArrayType::Create(dataStructure, "Mask", maskStore, cellDataPtr->getId()) != nullptr);
  REQUIRE(BoolArrayType::Create(dataStructure, "Bool Sibling", boolStore, cellDataPtr->getId()) != nullptr);
  auto* crystalPtr = UInt32ArrayType::CreateWithStore<DataStore<uint32>>(dataStructure, "Crystal Structures", {2}, {1});
  REQUIRE(crystalPtr != nullptr);
  std::vector<float32> quatValues(k_TupleCount * 4);
  std::vector<int32> phaseValues(k_TupleCount);
  std::vector<float32> numericValues(k_TupleCount * 2);
  auto boolValues = std::make_unique<bool[]>(k_TupleCount);
  for(usize i = 0; i < k_TupleCount; i++)
  {
    quatValues[i * 4] = 1.0F;
    phaseValues[i] = 1;
    numericValues[i * 2] = static_cast<float32>(i);
    numericValues[i * 2 + 1] = -static_cast<float32>(i);
    boolValues[i] = (i % 3) != 0;
  }
  const std::array<uint32, 2> crystalValues = {0, 1};
  auto quatsWriteResult = quatsPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(quatValues.data(), quatValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(quatsWriteResult);
  auto phasesWriteResult = phasesPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phaseValues.data(), phaseValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(phasesWriteResult);
  auto numericWriteResult = numericPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(numericValues.data(), numericValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(numericWriteResult);
  auto maskStoreWriteResult = maskStore->copyFromBuffer(0, nonstd::span<const bool>(boolValues.get(), k_TupleCount));
  SIMPLNX_RESULT_REQUIRE_VALID(maskStoreWriteResult);
  auto boolStoreWriteResult = boolStore->copyFromBuffer(0, nonstd::span<const bool>(boolValues.get(), k_TupleCount));
  SIMPLNX_RESULT_REQUIRE_VALID(boolStoreWriteResult);
  auto crystalWriteResult = crystalPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(crystalValues.data(), crystalValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(crystalWriteResult);
}

Arguments SmallArguments(const bool useMask)
{
  Arguments args = AlignSectionsMisorientationFilter().getDefaultArguments();
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

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientation: small matrix with known shifted values", "[OrientationAnalysis][AlignSectionsMisorientation]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool useMask = GENERATE(false, true);
  CAPTURE(scenario, useMask);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  CreateSmallFixture(dataStructure);
  AlignSectionsMisorientationFilter filter;
  auto executeFilterResult = scope.executeFilter(filter, dataStructure, SmallArguments(useMask)).result;
  SIMPLNX_RESULT_REQUIRE_VALID(executeFilterResult);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(k_SmallNumericPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArrayType>(k_SmallBoolPath));
  const auto& numericStore = dataStructure.getDataRefAs<Float32ArrayType>(k_SmallNumericPath).getDataStoreRef();
  const auto& boolStore = dataStructure.getDataRefAs<BoolArrayType>(k_SmallBoolPath).getDataStoreRef();

  if(useMask)
  {
    // The mask pattern selects a two-row positive Y shift for slice zero.
    REQUIRE(numericStore[0] == 16.0F);
    REQUIRE(numericStore[1] == -16.0F);
    REQUIRE(numericStore[94] == 63.0F);
    REQUIRE(numericStore[95] == -63.0F);
    REQUIRE(numericStore[96] == 0.0F);
    REQUIRE(boolStore[0]);
    REQUIRE_FALSE(boolStore[8]);
    REQUIRE_FALSE(boolStore[48]);
  }
  else
  {
    // Identical unmasked slices have no shift.
    REQUIRE(numericStore[0] == 0.0F);
    REQUIRE(numericStore[1] == 0.0F);
    REQUIRE(numericStore[126] == 63.0F);
    REQUIRE(numericStore[127] == -63.0F);
    REQUIRE_FALSE(boolStore[0]);
    REQUIRE(boolStore[1]);
    REQUIRE_FALSE(boolStore[3]);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientation: Phase Index Bounds", "[OrientationAnalysis][AlignSectionsMisorientation]")
{
  unit_test::LoadPlugins();

  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool invalidReferenceSlice = GENERATE(false, true);
  const bool storeAlignmentShifts = GENERATE(false, true);
  CAPTURE(scenario, invalidReferenceSlice, storeAlignmentShifts);
  unit_test::AlgorithmTestScope scope(scenario);

  constexpr usize k_VoxelsPerSlice = 8 * 8;
  DataStructure dataStructure;
  CreateSmallFixture(dataStructure);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(k_SmallPhasesPath));
  auto& cellPhasesArrayRef = dataStructure.getDataRefAs<Int32ArrayType>(k_SmallPhasesPath);
  const usize invalidSliceStart = invalidReferenceSlice ? k_VoxelsPerSlice : 0;
  for(usize voxelIdx = invalidSliceStart; voxelIdx < invalidSliceStart + k_VoxelsPerSlice; ++voxelIdx)
  {
    cellPhasesArrayRef[voxelIdx] = 2;
  }

  AlignSectionsMisorientationFilter filter;
  Arguments args = SmallArguments(false);
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(storeAlignmentShifts));

  SECTION("Participating Phase returns an error")
  {
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    auto assertionResult = executeResult.result;
    SIMPLNX_RESULT_REQUIRE_INVALID(assertionResult);
    REQUIRE(executeResult.result.errors()[0].code == -53901);
  }

  SECTION("Masked Phase is ignored")
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArrayType>(k_SmallMaskPath));
    auto& maskArrayRef = dataStructure.getDataRefAs<BoolArrayType>(k_SmallMaskPath);
    for(usize voxelIdx = invalidSliceStart; voxelIdx < invalidSliceStart + k_VoxelsPerSlice; ++voxelIdx)
    {
      maskArrayRef[voxelIdx] = false;
    }
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(true));

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    auto assertionResult2 = executeResult.result;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult2);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientation Small IN100 Pipeline", "[OrientationAnalysis][AlignSectionsMisorientation]")
{
  unit_test::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  const std::string kDataInputArchive2 = "align_sections.tar.gz";
  const std::string kExpectedOutputTopLevel2 = "align_sections_misorientation.txt";
  const nx::core::unit_test::TestFileSentinel testDataSentinel2(nx::core::unit_test::k_TestFilesDir, kDataInputArchive2, kExpectedOutputTopLevel2);

  auto* filterListPtr = Application::Instance()->getFilterList();

  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/6_6_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(constants::k_PhasesArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(constants::k_PhasesArrayPath));

  small_in100::ExecuteMultiThresholdObjects(dataStructure, *filterListPtr);

  small_in100::ExecuteConvertOrientations(dataStructure, *filterListPtr);

  {
    Arguments args;
    AlignSectionsMisorientationFilter filter;

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(constants::k_MaskArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(constants::k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(constants::k_PhasesArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(constants::k_DataContainerPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    auto assertionResult3 = preflightResult.output_actions;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult3);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    auto assertionResult4 = executeResult.result;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult4);
  }

  unit_test::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, constants::k_CellAttributeMatrix, constants::k_ExemplarDataContainer);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_misorientation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure, small_in100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: output test", "[Reconstruction][AlignSectionsMisorientationFilter]")
{
  unit_test::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  auto* filterListPtr = Application::Instance()->getFilterList();

  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(constants::k_PhasesArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(constants::k_PhasesArrayPath));

  small_in100::ExecuteMultiThresholdObjects(dataStructure, *filterListPtr);

  small_in100::ExecuteConvertOrientations(dataStructure, *filterListPtr);

  {
    Arguments args;
    AlignSectionsMisorientationFilter filter;

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(constants::k_MaskArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(constants::k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(constants::k_PhasesArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(constants::k_DataContainerPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_AlignmentAMName_Key, std::make_any<std::string>(constants::k_AlignmentAMName));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SlicesArrayName_Key, std::make_any<std::string>(constants::k_SlicesArrayName));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_RelativeShiftsArrayName_Key, std::make_any<std::string>(constants::k_RelativeShiftsArrayName));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>(constants::k_CumulativeShiftsArrayName));

    auto preflightResult = filter.preflight(dataStructure, args);
    auto assertionResult5 = preflightResult.output_actions;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult5);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    auto assertionResult6 = executeResult.result;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult6);
  }

  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  const DataPath alignmentAMPath = constants::k_DataContainerPath.createChildPath(constants::k_AlignmentAMName);

  const DataPath slicesPath = alignmentAMPath.createChildPath(constants::k_SlicesArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(slicesPath));
  unit_test::CompareDataArrays<uint32>(exemplarDataStructure.getDataRefAs<IDataArray>(slicesPath), dataStructure.getDataRefAs<IDataArray>(slicesPath));

  const DataPath relativeShiftsPath = alignmentAMPath.createChildPath(constants::k_RelativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));
  unit_test::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(relativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(relativeShiftsPath));

  const DataPath cumulativeShiftsPath = alignmentAMPath.createChildPath(constants::k_CumulativeShiftsArrayName);
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));
  unit_test::CompareDataArrays<int64>(exemplarDataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath), dataStructure.getDataRefAs<IDataArray>(cumulativeShiftsPath));

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/output_align_sections_misorientation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure, small_in100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Geometry cell tuple counts", "[OrientationAnalysis][AlignSectionsMisorientation][TupleCount]")
{
  // Cases: all short, all long, all matching outside Cell Data, then each input short.
  const int testCase = GENERATE(0, 1, 2, 3, 4, 5, 6, 7);
  CAPTURE(testCase);
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "TupleGeometry");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 2});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 2, 2}, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);

  AlignSectionsMisorientationFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"TupleGeometry"})));
  const usize commonCount = testCase == 0 ? 4 : (testCase == 1 ? 12 : 8);
  const usize count0 = testCase == 3 ? 4 : commonCount;
  auto* inputAM0Ptr = AttributeMatrix::Create(dataStructure, "Input0", {count0});
  REQUIRE(inputAM0Ptr != nullptr);
  REQUIRE(DataArray<float32>::CreateWithStore<DataStore<float32>>(dataStructure, "Quats", {count0}, {4}, inputAM0Ptr->getId()) != nullptr);
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input0", "Quats"})));
  const usize count1 = testCase == 4 ? 4 : commonCount;
  auto* inputAM1Ptr = AttributeMatrix::Create(dataStructure, "Input1", {count1});
  REQUIRE(inputAM1Ptr != nullptr);
  REQUIRE(DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "Phases", {count1}, {1}, inputAM1Ptr->getId()) != nullptr);
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Input1", "Phases"})));
  const usize count2 = (testCase == 5 || testCase == 6) ? 4 : commonCount;
  auto* inputAM2Ptr = AttributeMatrix::Create(dataStructure, "Input2", {count2});
  REQUIRE(inputAM2Ptr != nullptr);
  REQUIRE(DataArray<uint8>::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {count2}, {1}, inputAM2Ptr->getId()) != nullptr);
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Input2", "Mask"})));
  REQUIRE(UInt32ArrayType::CreateWithStore<DataStore<uint32>>(dataStructure, "CrystalStructures", {2}, {1}) != nullptr);
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"CrystalStructures"})));
  // Disabled masks may be short or absent.
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(testCase < 6));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(false));
  if(testCase == 7)
  {
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"MissingMask"})));
  }

  const auto result = filter.preflight(dataStructure, args);
  if(testCase == 2 || testCase >= 6)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    const auto& errorRef = result.output_actions.errors()[0];
    const std::vector<DataPath> inputPaths = {DataPath({"Input0", "Quats"}), DataPath({"Input1", "Phases"}), DataPath({"Input2", "Mask"})};
    if(testCase >= 3)
    {
      CHECK(errorRef.code == -68063);
      CHECK(errorRef.message.find(fmt::format("DataPath: {}    | Tuple Count: 4", inputPaths[static_cast<usize>(testCase - 3)].toString())) != std::string::npos);
    }
    else
    {
      CHECK(errorRef.code == -68072);
      CHECK(errorRef.message.find(inputPaths[0].toString()) != std::string::npos);
      CHECK(errorRef.message.find(testCase == 1 ? "12 tuples" : "4 tuples") != std::string::npos);
      CHECK(errorRef.message.find("8 cells") != std::string::npos);
      CHECK(errorRef.message.find("TupleGeometry") != std::string::npos);
    }
  }
}

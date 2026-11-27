#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/AlignSectionsMutualInformationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <cmath>
#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("OrientationAnalysis::AlignSectionsMutualInformationFilter: Valid filter execution")
{
  unit_test::LoadPlugins();
  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_mutual_information.tar.gz", "align_sections_mutual_information");

  // The generated directory verifies that the filter creates missing output
  // directories.
  const uint64 millisFromEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  auto* filterListPtr = Application::Instance()->getFilterList();

  const DataPath kExemplarShiftsPath = constants::k_ExemplarDataContainerPath.createChildPath("Exemplar Shifts");

  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_mutual_information/6_5_align_sections_mutual_information.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(exemplarFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(constants::k_QuatsArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(constants::k_QuatsArrayPath));

  {
    AlignSectionsMutualInformationFilter filter;
    Arguments args;

    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(constants::k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(constants::k_PhasesArrayPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(constants::k_CrystalStructuresArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  unit_test::CompareExemplarToGeneratedData(dataStructure, dataStructure, constants::k_CellAttributeMatrix, constants::k_ExemplarDataContainer);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_mutual_information.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMutualInformationFilter: InValid filter execution")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(constants::k_QuatsArrayPath));

  AlignSectionsMutualInformationFilter filter;
  Arguments args;

  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(constants::k_DataContainerPath));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(constants::k_QuatsArrayPath));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(constants::k_PhasesArrayPath));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key,
                      std::make_any<DataPath>(DataPath({constants::k_DataContainer, constants::k_CellFeatureData, constants::k_ActiveName})));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(constants::k_CrystalStructuresArrayPath));

  SECTION("Mismatching cell data tuples")
  {
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key,
                        std::make_any<DataPath>(DataPath({constants::k_DataContainer, constants::k_CellFeatureData, constants::k_ActiveName})));
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors()[0].code == -3542);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMutualInformationFilter: Phase and Laue Index Bounds", "[OrientationAnalysis][AlignSectionsMutualInformationFilter]")
{
  unit_test::LoadPlugins();

  const bool invalidPhaseIdx = GENERATE(false, true);
  CAPTURE(invalidPhaseIdx);

  constexpr usize k_DimX = 8;
  constexpr usize k_DimY = 8;
  constexpr usize k_DimZ = 2;
  constexpr usize k_TotalVoxels = k_DimX * k_DimY * k_DimZ;
  constexpr usize k_NumCrystalStructures = 2;

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({k_DimX, k_DimY, k_DimZ});
  REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());

  const ShapeType cellTupleShape = {k_DimZ, k_DimY, k_DimX};
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", cellTupleShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  auto* ensembleDataPtr = AttributeMatrix::Create(dataStructure, "CellEnsembleData", ShapeType{k_NumCrystalStructures}, imageGeomPtr->getId());

  auto* quatsArrayPtr = unit_test::CreateTestDataArray<float32>(dataStructure, "Quats", cellTupleShape, {4}, cellDataPtr->getId());
  auto* cellPhasesArrayPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Phases", cellTupleShape, {1}, cellDataPtr->getId());
  auto* maskArrayPtr = unit_test::CreateTestDataArray<bool>(dataStructure, "Mask", cellTupleShape, {1}, cellDataPtr->getId());
  auto* crystalStructuresArrayPtr = unit_test::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{k_NumCrystalStructures}, {1}, ensembleDataPtr->getId());

  quatsArrayPtr->fill(0.0F);
  for(usize voxelIdx = 0; voxelIdx < k_TotalVoxels; ++voxelIdx)
  {
    (*quatsArrayPtr)[voxelIdx * 4 + 3] = 1.0F;
  }
  cellPhasesArrayPtr->fill(invalidPhaseIdx ? static_cast<int32>(k_NumCrystalStructures) : 1);
  maskArrayPtr->fill(true);
  (*crystalStructuresArrayPtr)[0] = 999U;
  (*crystalStructuresArrayPtr)[1] = invalidPhaseIdx ? 1U : 999U;

  const DataPath imageGeomPath({"ImageGeometry"});
  const DataPath cellDataPath = imageGeomPath.createChildPath("CellData");
  const DataPath ensembleDataPath = imageGeomPath.createChildPath("CellEnsembleData");

  AlignSectionsMutualInformationFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Quats")));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Phases")));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Mask")));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ensembleDataPath.createChildPath("CrystalStructures")));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(false));

  SECTION("Participating index returns an error")
  {
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == (invalidPhaseIdx ? -53703 : -53704));
  }

  SECTION("Masked Phase is ignored")
  {
    cellPhasesArrayPtr->fill(static_cast<int32>(k_NumCrystalStructures));
    maskArrayPtr->fill(false);
    (*crystalStructuresArrayPtr)[1] = 1U;
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_UseMask_Key, std::make_any<bool>(true));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMutualInformationFilter: output test", "[Reconstruction][AlignSectionsMutualInformationFilter]")
{
  unit_test::LoadPlugins();
  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_mutual_information.tar.gz", "align_sections_mutual_information");

  auto baseFilePath = fs::path(fmt::format("{}/align_sections_mutual_information/6_5_align_sections_mutual_information.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(constants::k_QuatsArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(constants::k_QuatsArrayPath));

  {
    AlignSectionsMutualInformationFilter filter;
    Arguments args;

    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(constants::k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(constants::k_PhasesArrayPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_AlignmentAMName_Key, std::make_any<std::string>(constants::k_AlignmentAMName));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_SlicesArrayName_Key, std::make_any<std::string>(constants::k_SlicesArrayName));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_RelativeShiftsArrayName_Key, std::make_any<std::string>(constants::k_RelativeShiftsArrayName));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>(constants::k_CumulativeShiftsArrayName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_mutual_information/output_align_sections_mutual_information.dream3d", unit_test::k_TestFilesDir));
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
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/output_align_sections_mutual_information.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMutualInformationFilter: Geometry cell tuple counts", "[OrientationAnalysis][AlignSectionsMutualInformationFilter][TupleCount]")
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

  AlignSectionsMutualInformationFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"TupleGeometry"})));
  const usize commonCount = testCase == 0 ? 4 : (testCase == 1 ? 12 : 8);
  const usize count0 = testCase == 3 ? 4 : commonCount;
  auto* inputAM0Ptr = AttributeMatrix::Create(dataStructure, "Input0", {count0});
  REQUIRE(inputAM0Ptr != nullptr);
  REQUIRE(DataArray<float32>::CreateWithStore<DataStore<float32>>(dataStructure, "Quats", {count0}, {4}, inputAM0Ptr->getId()) != nullptr);
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input0", "Quats"})));
  const usize count1 = testCase == 4 ? 4 : commonCount;
  auto* inputAM1Ptr = AttributeMatrix::Create(dataStructure, "Input1", {count1});
  REQUIRE(inputAM1Ptr != nullptr);
  REQUIRE(DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "Phases", {count1}, {1}, inputAM1Ptr->getId()) != nullptr);
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Input1", "Phases"})));
  const usize count2 = (testCase == 5 || testCase == 6) ? 4 : commonCount;
  auto* inputAM2Ptr = AttributeMatrix::Create(dataStructure, "Input2", {count2});
  REQUIRE(inputAM2Ptr != nullptr);
  REQUIRE(DataArray<uint8>::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {count2}, {1}, inputAM2Ptr->getId()) != nullptr);
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Input2", "Mask"})));
  REQUIRE(UInt32ArrayType::CreateWithStore<DataStore<uint32>>(dataStructure, "CrystalStructures", {2}, {1}) != nullptr);
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"CrystalStructures"})));
  // Disabled masks may be short or absent.
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_UseMask_Key, std::make_any<bool>(testCase < 6));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(false));
  if(testCase == 7)
  {
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"MissingMask"})));
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
      CHECK(errorRef.code == -3542);
      CHECK(errorRef.message.find(fmt::format("DataPath: {}    | Tuple Count: 4", inputPaths[static_cast<usize>(testCase - 3)].toString())) != std::string::npos);
    }
    else
    {
      CHECK(errorRef.code == -3543);
      CHECK(errorRef.message.find(inputPaths[0].toString()) != std::string::npos);
      CHECK(errorRef.message.find(testCase == 1 ? "12 tuples" : "4 tuples") != std::string::npos);
      CHECK(errorRef.message.find("8 cells") != std::string::npos);
      CHECK(errorRef.message.find("TupleGeometry") != std::string::npos);
    }
  }
}

#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/AlignSectionsMutualInformationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

TEST_CASE("OrientationAnalysis::AlignSectionsMutualInformationFilter: Valid filter execution")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_5_align_sections_mutual_information.tar.gz",
                                                             "6_5_align_sections_mutual_information");

  // We are just going to generate a big number so that we can use that in the output
  // file path. This tests the creation of intermediate directories that the filter
  // would be responsible to create.
  const uint64_t millisFromEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  const DataPath k_ExemplarShiftsPath = k_ExemplarDataContainerPath.createChildPath("Exemplar Shifts");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_align_sections_mutual_information/6_5_align_sections_mutual_information.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Constants::k_DataContainerPath);

  const int32 k_NumSlices = imageGeom.getNumZCells() - 1;
  const fs::path computedShiftsFile = (fmt::format("{}/{}/AlignSectionsMutualInformation_1.txt", unit_test::k_BinaryTestOutputDir, millisFromEpoch));

  const std::string k_InputDataContainer("InputDataContainer");
  // Align Sections Mutual Information Filter
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    AlignSectionsMutualInformationFilter filter;
    Arguments args;

    // Create valid Parameters for the filter.
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(computedShiftsFile));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumSlices)}};

  // Read Exemplar Shifts File
  {
    static constexpr StringLiteral k_InputFileKey = "input_file";
    static constexpr StringLiteral k_ScalarTypeKey = "scalar_type";
    static constexpr StringLiteral k_NTuplesKey = "n_tuples";
    static constexpr StringLiteral k_NCompKey = "n_comp";
    static constexpr StringLiteral k_NSkipLinesKey = "n_skip_lines";
    static constexpr StringLiteral k_DelimiterChoiceKey = "delimiter_choice";
    static constexpr StringLiteral k_DataArrayKey = "output_data_array";

    // Compare the output of the shifts file with the exemplar file

    auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    // read in the exemplar shift data file
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(
                                            fs::path(fmt::format("{}/6_5_align_sections_mutual_information/6_5_align_sections_mutual_information.txt", unit_test::k_TestFilesDir))));
    args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::int32));
    args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insertOrAssign(k_NCompKey, std::make_any<uint64>(6));
    args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(4));
    args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_ExemplarShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Read Computed Shifts File
  {
    static constexpr StringLiteral k_InputFileKey = "input_file";
    static constexpr StringLiteral k_ScalarTypeKey = "scalar_type";
    static constexpr StringLiteral k_NTuplesKey = "n_tuples";
    static constexpr StringLiteral k_NCompKey = "n_comp";
    static constexpr StringLiteral k_NSkipLinesKey = "n_skip_lines";
    static constexpr StringLiteral k_DelimiterChoiceKey = "delimiter_choice";
    static constexpr StringLiteral k_DataArrayKey = "output_data_array";

    // Compare the output of the shifts file with the exemplar file

    auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(computedShiftsFile));
    args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::int32));
    args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insertOrAssign(k_NCompKey, std::make_any<uint64>(6));
    args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(4));
    args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_CalculatedShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_mutual_information.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  CompareExemplarToGeneratedData(dataStructure, dataStructure, k_CellAttributeMatrix, k_ExemplarDataContainer);

  // Compare the shift values
  CompareArrays<int32>(dataStructure, k_CalculatedShiftsPath, k_ExemplarShiftsPath);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMutualInformationFilter: InValid filter execution")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_stats_test.tar.gz", "6_6_stats_test.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  // Instantiate the filter and an Arguments Object
  AlignSectionsMutualInformationFilter filter;
  Arguments args;
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_AlignmentShiftFileName_Key,
                      std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/AlignSectionsMutualInformation_2.txt", unit_test::k_BinaryDir))));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key,
                      std::make_any<DataPath>(DataPath({Constants::k_DataContainer, Constants::k_CellFeatureData, Constants::k_ActiveName})));
  args.insertOrAssign(AlignSectionsMutualInformationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));

  SECTION("Mismatching cell data tuples")
  {
    args.insertOrAssign(AlignSectionsMutualInformationFilter::k_MaskArrayPath_Key,
                        std::make_any<DataPath>(DataPath({Constants::k_DataContainer, Constants::k_CellFeatureData, Constants::k_ActiveName})));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -3542);
}

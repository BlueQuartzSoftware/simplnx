#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/AlignSectionsMisorientationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataStoreFormatParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

/**
 * Read H5Ebsd File
 * MultiThreshold Objects
 * Convert Orientation Representation (Euler->Quats)
 * Align Sections Misorientation
 *
 * Read DREAM3D File (read the exemplar 'align_sections_misorientation.dream3d' file from
 * [Optional] Write out dream3d file
 *
 *
 * Compare the shifts file 'align_sections_misorientation.txt' to what was written
 *
 * Compare all the data arrays from the "Exemplar Data / CellData"
 */

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientation Small IN100 Pipeline", "[OrientationAnalysis][AlignSectionsMisorientation]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_align_sections_misorientation.tar.gz",
                                                              "6_6_align_sections_misorientation.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d.tar.gz", "Small_IN100.dream3d");

  const std::string kDataInputArchive2 = "align_sections.tar.gz";
  const std::string kExpectedOutputTopLevel2 = "align_sections_misorientation.txt";
  const nx::core::UnitTest::TestFileSentinel testDataSentinel2(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, kDataInputArchive2, kExpectedOutputTopLevel2);

  // We are just going to generate a big number so that we can use that in the output
  // file path. This tests the creation of intermediate directories that the filter
  // would be responsible to create.
  const uint64_t millisFromEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  const DataPath k_ExemplarShiftsPath = k_DataContainerPath.createChildPath("Exemplar Shifts");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

  const fs::path computedShiftsFile = (fmt::format("{}/{}/AlignSectionsMutualInformation_1.txt", unit_test::k_BinaryTestOutputDir, millisFromEpoch));

  // Read Exemplar Shifts File
  {
    // Compare the output of the shifts file with the exemplar file

    auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    // read in the exemplar shift data file
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key,
                        std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/align_sections_misorientation.txt", unit_test::k_TestFilesDir))));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::int32));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(116)}}));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(6));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(4));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_ExemplarShiftsPath));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataFormat_Key, std::make_any<DataStoreFormatParameter::ValueType>(""));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // MultiThreshold Objects Filter (From SimplnxCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  // Align Sections Misorientation Filter (From OrientationAnalysis Plugin)
  {
    Arguments args;
    AlignSectionsMisorientationFilter filter;
    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(computedShiftsFile));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_CellAttributeMatrix, k_ExemplarDataContainer);

  // Use the Read Text File Filter to read in the generated Shift File
  {
    // Compare the output of the shifts file with the exemplar file

    auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedShiftsFile));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::int32));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(116)}}));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(6));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(4));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_CalculatedShiftsPath));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataFormat_Key, std::make_any<DataStoreFormatParameter::ValueType>(""));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_misorientation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare the shift values
  CompareArrays<int32>(dataStructure, k_CalculatedShiftsPath, k_ExemplarShiftsPath);
}

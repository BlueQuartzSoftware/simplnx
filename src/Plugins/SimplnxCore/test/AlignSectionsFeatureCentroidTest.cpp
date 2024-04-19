#include "SimplnxCore/Filters/AlignSectionsFeatureCentroidFilter.hpp"
#include "SimplnxCore/Filters/ReadTextDataArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("SimplnxCore::AlignSectionsFeatureCentroidFilter", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  // We are just going to generate a big number so that we can use that in the output
  // file path. This tests the creation of intermediate directories that the filter
  // would be responsible to create.
  const uint64_t millisFromEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  const DataPath k_ExemplarShiftsPath = k_ExemplarDataContainerPath.createChildPath("Exemplar Shifts");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_align_sections_feature_centroids.tar.gz",
                                                              "6_6_align_sections_feature_centroids");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_align_sections_feature_centroids/6_6_align_sections_feature_centroids.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);
  const int32 k_NumSlices = 59;
  const int32 k_NumColumns = 8;
  const int32 k_Comma = 0;
  const int32 k_SkipHeaderLines = 1;
  const fs::path computedShiftsFile = (fmt::format("{}/{}/AlignSectionsMutualInformation_1.txt", unit_test::k_BinaryTestOutputDir, millisFromEpoch));

  // Align Sections Feature Centroid Filter
  {

    AlignSectionsFeatureCentroidFilter filter;

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(computedShiftsFile));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskArrayPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedCellDataGroup_Key, std::make_any<DataPath>(k_CellAttributeMatrix));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  CompareExemplarToGeneratedData(dataStructure, dataStructure, k_CellAttributeMatrix, k_ExemplarDataContainer);

  const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NumSlices)}};

  // Read Exemplar Shifts File
  {
    // Compare the output of the shifts file with the exemplar file

    ReadTextDataArrayFilter filter;

    Arguments args;
    // read in the exemplar shift data file
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(
                                                                      fmt::format("{}/6_6_align_sections_feature_centroids/6_6_align_sections_feature_centroids.txt", unit_test::k_TestFilesDir))));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(k_NumColumns));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(k_SkipHeaderLines));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(k_Comma));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_ExemplarShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Read Computed Shifts File
  {
    // Compare the output of the shifts file with the exemplar file

    ReadTextDataArrayFilter filter;

    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedShiftsFile));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(k_NumColumns));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(k_SkipHeaderLines));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(k_Comma));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_CalculatedShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

// Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_feature_centroid.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare the shift values
  CompareArrays<float32>(dataStructure, k_CalculatedShiftsPath, k_ExemplarShiftsPath);
}

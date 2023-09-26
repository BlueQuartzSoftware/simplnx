#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/AlignSectionsFeatureCentroidFilter.hpp"
#include "ComplexCore/Filters/ReadTextDataArrayFilter.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

TEST_CASE("ComplexCore::AlignSectionsFeatureCentroidFilter", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  // We are just going to generate a big number so that we can use that in the output
  // file path. This tests the creation of intermediate directories that the filter
  // would be responsible to create.
  const uint64_t millisFromEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  const DataPath k_ExemplarShiftsPath = k_ExemplarDataContainerPath.createChildPath("Exemplar Shifts");

  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_align_sections_feature_centroids.tar.gz",
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

    // Parameter Keys
    constexpr StringLiteral k_WriteAlignmentShifts_Key = "write_alignment_shifts";
    constexpr StringLiteral k_AlignmentShiftFileName_Key = "alignment_shift_file_name";
    constexpr StringLiteral k_UseReferenceSlice_Key = "use_reference_slice";
    constexpr StringLiteral k_ReferenceSlice_Key = "reference_slice";
    constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
    constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry_path";
    constexpr StringLiteral k_SelectedCellDataGroup_Key = "selected_cell_data_path";

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(computedShiftsFile));
    args.insertOrAssign(k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskArrayPath));
    args.insertOrAssign(k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(k_SelectedCellDataGroup_Key, std::make_any<DataPath>(k_CellAttributeMatrix));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  CompareExemplarToGeneratedData(dataStructure, dataStructure, k_CellAttributeMatrix, k_ExemplarDataContainer);

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

    ReadTextDataArrayFilter filter;

    Arguments args;
    // read in the exemplar shift data file
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(
                                            fs::path(fmt::format("{}/6_6_align_sections_feature_centroids/6_6_align_sections_feature_centroids.txt", unit_test::k_TestFilesDir))));
    args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::float32));
    args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insertOrAssign(k_NCompKey, std::make_any<uint64>(k_NumColumns));
    args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(k_SkipHeaderLines));
    args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(k_Comma));
    args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_ExemplarShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
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

    ReadTextDataArrayFilter filter;

    Arguments args;
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(computedShiftsFile));
    args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::float32));
    args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insertOrAssign(k_NCompKey, std::make_any<uint64>(k_NumColumns));
    args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(k_SkipHeaderLines));
    args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(k_Comma));
    args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_CalculatedShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

// Write out the .dream3d file now
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_feature_centroid.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare the shift values
  CompareArrays<float32>(dataStructure, k_CalculatedShiftsPath, k_ExemplarShiftsPath);
}

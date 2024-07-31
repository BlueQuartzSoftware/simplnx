#pragma once

#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/AlignSectionsMisorientationFilter.hpp"
#include "OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.hpp"
#include "OrientationAnalysis/Filters/ConvertOrientationsFilter.hpp"
#include "OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/Filters/NeighborOrientationCorrelationFilter.hpp"

#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Parameters/ArrayThresholdsParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <fmt/format.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace EbsdLib
{
namespace Ang
{
const std::string ConfidenceIndex("Confidence Index");
const std::string ImageQuality("Image Quality");

} // namespace Ang
namespace CellData
{
inline const std::string EulerAngles("EulerAngles");
inline const std::string Phases("Phases");
} // namespace CellData
namespace EnsembleData
{
inline const std::string CrystalStructures("CrystalStructures");
inline const std::string LatticeConstants("LatticeConstants");
inline const std::string MaterialName("MaterialName");
} // namespace EnsembleData
} // namespace EbsdLib

namespace AlignSectionsFeatureCentroidFilter
{
// Parameter Keys
static inline constexpr StringLiteral k_WriteAlignmentShifts_Key = "write_alignment_shifts";
static inline constexpr StringLiteral k_AlignmentShiftFileName_Key = "alignment_shift_file_name";
static inline constexpr StringLiteral k_UseReferenceSlice_Key = "use_reference_slice";
static inline constexpr StringLiteral k_ReferenceSlice_Key = "reference_slice";
static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
static inline constexpr StringLiteral k_SelectedCellDataGroup_Key = "selected_cell_data_path";
} // namespace AlignSectionsFeatureCentroidFilter

namespace ReadTextDataArrayFilter
{
static inline constexpr StringLiteral k_InputFile_Key = "input_file";
static inline constexpr StringLiteral k_ScalarType_Key = "scalar_type_index";
static inline constexpr StringLiteral k_NTuples_Key = "number_tuples";
static inline constexpr StringLiteral k_NComp_Key = "number_comp";
static inline constexpr StringLiteral k_NSkipLines_Key = "skip_line_count";
static inline constexpr StringLiteral k_DelimiterChoice_Key = "delimiter_index";
static inline constexpr StringLiteral k_DataArrayPath_Key = "output_data_array_path";
static inline constexpr StringLiteral k_DataFormat_Key = "data_format";
static inline constexpr StringLiteral k_AdvancedOptions_Key = "set_tuple_dimensions";
} // namespace ReadTextDataArrayFilter

namespace nx::core
{
// Make sure we can instantiate the Import Text Filter
const Uuid k_ReadTextDataArrayFilterId = *Uuid::FromString("25f7df3e-ca3e-4634-adda-732c0e56efd4");
const FilterHandle k_ReadTextDataArrayFilterHandle(k_ReadTextDataArrayFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the Read DREAM3D Data File
const Uuid k_ReadDREAM3DFilterId = *Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
const FilterHandle k_ReadDREAM3DFilterHandle(k_ReadDREAM3DFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the Read RenameDataObjectFilter
const Uuid k_RenameDataObjectFilterId = *Uuid::FromString("911a3aa9-d3c2-4f66-9451-8861c4b726d5");
const FilterHandle k_RenameDataObjectFilterHandle(k_RenameDataObjectFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the Crop Image Geometry
const Uuid k_CropImageGeometryFilterId = *Uuid::FromString("e6476737-4aa7-48ba-a702-3dfab82c96e2");
const FilterHandle k_CropImageGeometryFilterHandle(k_CropImageGeometryFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the CopyDataGroup
const Uuid k_CopyDataGroupFilterId = *Uuid::FromString("ac8d51d8-9167-5628-a060-95a8863a76b1");
const FilterHandle k_CopyDataGroupFilterHandle(k_CopyDataGroupFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the RequireMinimumSizeFeaturesFilter
const Uuid k_RequireMinimumSizeFeaturesFilterId = *Uuid::FromString("074472d3-ba8d-4a1a-99f2-2d56a0d082a0");
const FilterHandle k_RequireMinimumSizeFeaturesFilterHandle(k_RequireMinimumSizeFeaturesFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the ComputeFeatureSizesFilter
const Uuid k_ComputeFeatureSizesFilterId = *Uuid::FromString("c666ee17-ca58-4969-80d0-819986c72485");
const FilterHandle k_ComputeFeatureSizesFilterHandle(k_ComputeFeatureSizesFilterId, k_SimplnxCorePluginId);
const Uuid k_ReadCSVFileFilterId = *Uuid::FromString("373be1f8-31cf-49f6-aa5d-e356f4f3f261");
const FilterHandle k_ReadCSVFileFilterHandle(k_ReadCSVFileFilterId, k_SimplnxCorePluginId);

const Uuid k_OrientationAnalysisPluginId = *Uuid::FromString("c09cf01b-014e-5adb-84eb-ea76fc79eeb1");
// Make sure we can instantiate the Convert Orientations
const Uuid k_ConvertOrientationsFilterId = *Uuid::FromString("501e54e6-a66f-4eeb-ae37-00e649c00d4b");
const FilterHandle k_ConvertOrientationsFilterHandle(k_ConvertOrientationsFilterId, k_OrientationAnalysisPluginId);
// Make sure we can instantiate the EbsdSegmentFeatures
const Uuid k_EbsdSegmentFeaturesFilterId = *Uuid::FromString("1810c2c7-63e3-41db-b204-a5821e6271c0");
const FilterHandle k_EbsdSegmentFeaturesFilterHandle(k_EbsdSegmentFeaturesFilterId, k_OrientationAnalysisPluginId);
// Make sure we can instantiate the Align Sections Misorientation
const Uuid k_AlignSectionMisorientationFilterId = *Uuid::FromString("8df2135c-7079-49f4-9756-4f3c028a5ced");
const FilterHandle k_AlignSectionMisorientationFilterHandle(k_AlignSectionMisorientationFilterId, k_OrientationAnalysisPluginId);
// Make sure we can instantiate the NeighborOrientationCorrelation
const Uuid k_NeighborOrientationCorrelationFilterId = *Uuid::FromString("4625c192-7e46-4333-a294-67a2eb64cb37");
const FilterHandle k_NeighborOrientationCorrelationFilterHandle(k_NeighborOrientationCorrelationFilterId, k_OrientationAnalysisPluginId);
// Make sure we can instantiate the
const Uuid k_BadDataNeighborOrientationCheckFilterId = *Uuid::FromString("3f342977-aea1-49e1-a9c2-f73760eba0d3");
const FilterHandle k_BadDataNeighborOrientationCheckFilterHandle(k_BadDataNeighborOrientationCheckFilterId, k_OrientationAnalysisPluginId);

const Uuid k_CorePluginId = *Uuid::FromString("65a0a3fc-8c93-5405-8ac6-182e7f313a69");

// Make sure we can instantiate the Align Sections Feature Centroid
const Uuid k_AlignSectionsFeatureCentroidFilterId = *Uuid::FromString("b83f9bae-9ccf-4932-96c3-7f2fdb091452");
const FilterHandle k_AlignSectionsFeatureCentroidFilterHandle(k_AlignSectionsFeatureCentroidFilterId, k_SimplnxCorePluginId);

const Uuid k_ComputeFeaturePhasesObjectsId = *Uuid::FromString("da5bb20e-4a8e-49d9-9434-fbab7bc434fc");
const FilterHandle k_FindNFeaturePhasesFilterHandle(k_ComputeFeaturePhasesObjectsId, k_SimplnxCorePluginId);
const Uuid k_FindNeighborsObjectsId = *Uuid::FromString("7177e88c-c3ab-4169-abe9-1fdaff20e598");
const FilterHandle k_FindNeighborsFilterHandle(k_FindNeighborsObjectsId, k_SimplnxCorePluginId);
} // namespace nx::core

using namespace nx::core;

namespace SmallIn100
{
inline constexpr StringLiteral k_SmallIN100ScanData("EBSD Scan Data");

//------------------------------------------------------------------------------
inline void ExecuteConvertOrientations(DataStructure& dataStructure, const FilterList& filterList)
{
  INFO(fmt::format("Error creating Filter '{}'  ", k_ConvertOrientationsFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_ConvertOrientationsFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;
  args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(ConvertOrientationsFilter::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(Constants::k_EulersArrayPath));
  args.insertOrAssign(ConvertOrientationsFilter::k_OutputOrientationArrayName_Key, std::make_any<std::string>(Constants::k_Quats));

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

//------------------------------------------------------------------------------
inline void ExecuteEbsdSegmentFeatures(DataStructure& dataStructure, const FilterList& filterList)
{
  INFO(fmt::format("Error creating Filter '{}'  ", k_EbsdSegmentFeaturesFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_EbsdSegmentFeaturesFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Grain_Data));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(Constants::k_ActiveName));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

//------------------------------------------------------------------------------
inline void ExecuteAlignSectionsMisorientation(DataStructure& dataStructure, const FilterList& filterList, const fs::path& shiftsFile)
{
  INFO(fmt::format("Error creating Filter '{}'  ", k_AlignSectionMisorientationFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_AlignSectionMisorientationFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(shiftsFile));

  args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

  args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));

  args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));

  args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));

  args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

//------------------------------------------------------------------------------
inline void ExecuteAlignSectionsFeatureCentroid(DataStructure& dataStructure, const FilterList& filterList, const fs::path& shiftsFile)
{
  INFO(fmt::format("Error creating Filter '{}'  ", k_AlignSectionsFeatureCentroidFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_AlignSectionsFeatureCentroidFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(shiftsFile));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_UseReferenceSlice_Key, std::make_any<bool>(true));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_ReferenceSlice_Key, std::make_any<int32>(0));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(AlignSectionsFeatureCentroidFilter::k_SelectedCellDataGroup_Key, std::make_any<DataPath>(Constants::k_CellAttributeMatrix));

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

//------------------------------------------------------------------------------
inline void ExecuteBadDataNeighborOrientationCheck(DataStructure& dataStructure, const FilterList& filterList)
{

  INFO(fmt::format("Error creating Filter '{}'  ", k_BadDataNeighborOrientationCheckFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_BadDataNeighborOrientationCheckFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

//------------------------------------------------------------------------------
inline void ExecuteNeighborOrientationCorrelation(DataStructure& dataStructure, const FilterList& filterList)
{
  INFO(fmt::format("Error creating Filter '{}'  ", k_NeighborOrientationCorrelationFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_NeighborOrientationCorrelationFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(0.2f));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(2));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CorrelationArrayPath_Key, std::make_any<DataPath>(Constants::k_ConfidenceIndexArrayPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>());

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}
} // namespace SmallIn100

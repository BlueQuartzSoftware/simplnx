#pragma once

#include <catch2/catch.hpp>

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

namespace nx::core
{
// Make sure we can instantiate the Import Text Filter
const Uuid k_ReadTextDataArrayFilterId = *Uuid::FromString("25f7df3e-ca3e-4634-adda-732c0e56efd4");
const FilterHandle k_ReadTextDataArrayFilterHandle(k_ReadTextDataArrayFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the Read DREAM3D Data File
const Uuid k_ReadDREAM3DFilterId = *Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
const FilterHandle k_ReadDREAM3DFilterHandle(k_ReadDREAM3DFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the Read RenameDataObject
const Uuid k_RenameDataObjectFilterId = *Uuid::FromString("911a3aa9-d3c2-4f66-9451-8861c4b726d5");
const FilterHandle k_RenameDataObjectFilterHandle(k_RenameDataObjectFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the Crop Image Geometry
const Uuid k_CropImageGeometryFilterId = *Uuid::FromString("e6476737-4aa7-48ba-a702-3dfab82c96e2");
const FilterHandle k_CropImageGeometryFilterHandle(k_CropImageGeometryFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the CopyDataGroup
const Uuid k_CopyDataGroupFilterId = *Uuid::FromString("ac8d51d8-9167-5628-a060-95a8863a76b1");
const FilterHandle k_CopyDataGroupFilterHandle(k_CopyDataGroupFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the RemoveMinimumSizeFeaturesFilter
const Uuid k_RemoveMinimumSizeFeaturesFilterId = *Uuid::FromString("074472d3-ba8d-4a1a-99f2-2d56a0d082a0");
const FilterHandle k_RemoveMinimumSizeFeaturesFilterHandle(k_RemoveMinimumSizeFeaturesFilterId, k_SimplnxCorePluginId);
// Make sure we can instantiate the CalculateFeatureSizesFilter
const Uuid k_CalculateFeatureSizesFilterId = *Uuid::FromString("c666ee17-ca58-4969-80d0-819986c72485");
const FilterHandle k_CalculateFeatureSizesFilterHandle(k_CalculateFeatureSizesFilterId, k_SimplnxCorePluginId);
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

const Uuid k_FindFeaturePhasesObjectsId = *Uuid::FromString("da5bb20e-4a8e-49d9-9434-fbab7bc434fc");
const FilterHandle k_FindNFeaturePhasesFilterHandle(k_FindFeaturePhasesObjectsId, k_SimplnxCorePluginId);
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

  // Parameter Keys from AlignSectionsMisorientation. If those change these will need to be updated
  constexpr StringLiteral k_InputType_Key = "input_type";
  constexpr StringLiteral k_OutputType_Key = "output_type";
  constexpr StringLiteral k_InputOrientationArrayPath_Key = "input_orientation_array_path";
  constexpr StringLiteral k_OutputOrientationArrayName_Key = "output_orientation_array_name";

  Arguments args;
  args.insertOrAssign(k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(k_InputOrientationArrayPath_Key, std::make_any<DataPath>(Constants::k_EulersArrayPath));
  args.insertOrAssign(k_OutputOrientationArrayName_Key, std::make_any<std::string>(Constants::k_Quats));

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

  // Parameter Keys
  constexpr StringLiteral k_GridGeomPath_Key = "input_image_geometry_path";
  constexpr StringLiteral k_MisorientationTolerance_Key = "misorientation_tolerance";
  constexpr StringLiteral k_UseMask_Key = "use_mask";
  constexpr StringLiteral k_QuatsArrayPath_Key = "cell_quats_array_path";
  constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  constexpr StringLiteral k_MaskArrayPath_Key = "cell_mask_array_path";
  constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  constexpr StringLiteral k_FeatureIdsArrayName_Key = "feature_ids_array_name";
  constexpr StringLiteral k_CellFeatureAttributeMatrixName_Key = "cell_feature_attribute_matrix_name";
  constexpr StringLiteral k_ActiveArrayName_Key = "active_array_name";
  constexpr StringLiteral k_RandomizeFeatures_Key = "randomize_features";

  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
  args.insertOrAssign(k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(k_GridGeomPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
  args.insertOrAssign(k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
  args.insertOrAssign(k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));
  args.insertOrAssign(k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));
  args.insertOrAssign(k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Grain_Data));
  args.insertOrAssign(k_ActiveArrayName_Key, std::make_any<std::string>(Constants::k_ActiveName));
  args.insertOrAssign(k_RandomizeFeatures_Key, std::make_any<bool>(false));

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

  // Parameter Keys
  constexpr StringLiteral k_WriteAlignmentShifts_Key = "write_alignment_shifts";
  constexpr StringLiteral k_AlignmentShiftFileName_Key = "alignment_shift_file_name";

  constexpr StringLiteral k_MisorientationTolerance_Key = "misorientation_tolerance";

  constexpr StringLiteral k_UseMask_Key = "use_mask";
  constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";

  constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";

  constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";

  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
  args.insertOrAssign(k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(shiftsFile));

  args.insertOrAssign(k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

  args.insertOrAssign(k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));

  args.insertOrAssign(k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
  args.insertOrAssign(k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));

  args.insertOrAssign(k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));

  args.insertOrAssign(k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

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

  // Parameter Keys
  constexpr StringLiteral k_WriteAlignmentShifts_Key = "write_alignment_shifts";
  constexpr StringLiteral k_AlignmentShiftFileName_Key = "alignment_shift_file_name";
  constexpr StringLiteral k_UseReferenceSlice_Key = "use_reference_slice";
  constexpr StringLiteral k_ReferenceSlice_Key = "reference_slice";
  constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
  constexpr StringLiteral k_SelectedCellDataGroup_Key = "selected_cell_data_path";

  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
  args.insertOrAssign(k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(shiftsFile));
  args.insertOrAssign(k_UseReferenceSlice_Key, std::make_any<bool>(true));
  args.insertOrAssign(k_ReferenceSlice_Key, std::make_any<int32>(0));
  args.insertOrAssign(k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
  args.insertOrAssign(k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(k_SelectedCellDataGroup_Key, std::make_any<DataPath>(Constants::k_CellAttributeMatrix));

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
  // Parameter Keys
  constexpr StringLiteral k_MisorientationTolerance_Key = "misorientation_tolerance";
  constexpr StringLiteral k_NumberOfNeighbors_Key = "number_of_neighbors";
  constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";

  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(k_NumberOfNeighbors_Key, std::make_any<int32>(4));
  args.insertOrAssign(k_ImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
  args.insertOrAssign(k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
  args.insertOrAssign(k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));

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

  // Parameter Keys
  constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  constexpr StringLiteral k_MinConfidence_Key = "min_confidence";
  constexpr StringLiteral k_MisorientationTolerance_Key = "misorientation_tolerance";
  constexpr StringLiteral k_Level_Key = "level";
  constexpr StringLiteral k_CorrelationArrayPath_Key = "correlation_array_path";
  constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  constexpr StringLiteral k_IgnoredDataArrayPaths_Key = "ignored_data_array_paths";

  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(k_ImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(k_MinConfidence_Key, std::make_any<float32>(0.2f));
  args.insertOrAssign(k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(k_Level_Key, std::make_any<int32>(2));
  args.insertOrAssign(k_CorrelationArrayPath_Key, std::make_any<DataPath>(Constants::k_ConfidenceIndexArrayPath));
  args.insertOrAssign(k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
  args.insertOrAssign(k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));
  args.insertOrAssign(k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>());

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}
} // namespace SmallIn100

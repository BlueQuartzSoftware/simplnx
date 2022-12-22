#pragma once

#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include <cmath>
#include <filesystem>
namespace fs = std::filesystem;

namespace complex
{
// Make sure we can load the needed filters from the plugins
const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
// Make sure we can instantiate the MultiThreshold Objects Filter
const Uuid k_MultiThresholdObjectsId = *Uuid::FromString("4246245e-1011-4add-8436-0af6bed19228");
const FilterHandle k_MultiThresholdObjectsFilterHandle(k_MultiThresholdObjectsId, k_ComplexCorePluginId);
// Make sure we can instantiate the Import Text Filter
const Uuid k_ImportTextFilterId = *Uuid::FromString("25f7df3e-ca3e-4634-adda-732c0e56efd4");
const FilterHandle k_ImportTextFilterHandle(k_ImportTextFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the Read DREAM3D Data File
const Uuid k_ImportDream3dFilterId = *Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
const FilterHandle k_ImportDream3dFilterHandle(k_ImportDream3dFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the Read RenameDataObject
const Uuid k_RenameDataObjectFilterId = *Uuid::FromString("911a3aa9-d3c2-4f66-9451-8861c4b726d5");
const FilterHandle k_RenameDataObjectFilterHandle(k_RenameDataObjectFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the Crop Image Geometry
const Uuid k_CropImageGeometryFilterId = *Uuid::FromString("e6476737-4aa7-48ba-a702-3dfab82c96e2");
const FilterHandle k_CropImageGeometryFilterHandle(k_CropImageGeometryFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the IdentifySample
const Uuid k_IdentifySampleFilterId = *Uuid::FromString("94d47495-5a89-4c7f-a0ee-5ff20e6bd273");
const FilterHandle k_IdentifySampleFilterHandle(k_IdentifySampleFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the CopyDataGroup
const Uuid k_CopyDataGroupFilterId = *Uuid::FromString("ac8d51d8-9167-5628-a060-95a8863a76b1");
const FilterHandle k_CopyDataGroupFilterHandle(k_CopyDataGroupFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the RemoveMinimumSizeFeaturesFilter
const Uuid k_RemoveMinimumSizeFeaturesFilterId = *Uuid::FromString("074472d3-ba8d-4a1a-99f2-2d56a0d082a0");
const FilterHandle k_RemoveMinimumSizeFeaturesFilterHandle(k_RemoveMinimumSizeFeaturesFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the CalculateFeatureSizesFilter
const Uuid k_CalculateFeatureSizesFilterId = *Uuid::FromString("c666ee17-ca58-4969-80d0-819986c72485");
const FilterHandle k_CalculateFeatureSizesFilterHandle(k_CalculateFeatureSizesFilterId, k_ComplexCorePluginId);
const Uuid k_ImportCSVDataFilterId = *Uuid::FromString("373be1f8-31cf-49f6-aa5d-e356f4f3f261");
const FilterHandle k_ImportCSVDataFilterHandle(k_ImportCSVDataFilterId, k_ComplexCorePluginId);

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
const FilterHandle k_AlignSectionsFeatureCentroidFilterHandle(k_AlignSectionsFeatureCentroidFilterId, k_CorePluginId);

const Uuid k_FindFeaturePhasesObjectsId = *Uuid::FromString("da5bb20e-4a8e-49d9-9434-fbab7bc434fc");
const FilterHandle k_FindNFeaturePhasesFilterHandle(k_FindFeaturePhasesObjectsId, k_ComplexCorePluginId);
const Uuid k_FindNeighborsObjectsId = *Uuid::FromString("7177e88c-c3ab-4169-abe9-1fdaff20e598");
const FilterHandle k_FindNeighborsFilterHandle(k_FindNeighborsObjectsId, k_ComplexCorePluginId);
} // namespace complex

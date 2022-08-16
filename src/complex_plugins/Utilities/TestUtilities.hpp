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

constexpr float EPSILON = 0.00001;

const std::string k_Quats("Quats");
const std::string k_Phases("Phases");
const std::string k_FeatureIds("FeatureIds");
const std::string k_ConfidenceIndex("Confidence Index");
const std::string k_ImageQuality("Image Quality");
const std::string k_Fit("Fit");
const std::string k_SEMSignal("SEM Signal");

const std::string k_Mask("Mask");
const std::string k_Active("Active");
const std::string k_GrainData("Grain Data");

// Top Level DataGroup
const std::string k_DataContainer("DataContainer");
const DataPath k_DataContainerPath({k_DataContainer});
// Feature Level Data
const std::string k_CellFeatureData("CellFeatureData");
const std::string k_Centroids("Centroids");
const std::string k_EulerAngles("EulerAngles");
// Cell Level Group
const std::string k_SmallIN100ScanData("EBSD Scan Data");
const std::string k_CellData("CellData");

const std::string k_ExemplarDataContainer("Exemplar Data");
const DataPath k_ExemplarDataContainerPath({k_ExemplarDataContainer});

const DataPath k_CalculatedShiftsPath = k_DataContainerPath.createChildPath("Calculated Shifts");

const DataPath k_CellAttributeMatrix = k_DataContainerPath.createChildPath(k_CellData);
const DataPath k_EulersArrayPath = k_CellAttributeMatrix.createChildPath(k_EulerAngles);
const DataPath k_QuatsArrayPath = k_CellAttributeMatrix.createChildPath(k_Quats);
const DataPath k_PhasesArrayPath = k_CellAttributeMatrix.createChildPath(k_Phases);
const DataPath k_FeatureIdsArrayPath = k_CellAttributeMatrix.createChildPath(k_FeatureIds);
const DataPath k_ConfidenceIndexArrayPath = k_CellAttributeMatrix.createChildPath(k_ConfidenceIndex);
const DataPath k_ImageQualityArrayPath = k_CellAttributeMatrix.createChildPath(k_ImageQuality);
const DataPath k_MaskArrayPath = k_CellAttributeMatrix.createChildPath(k_Mask);

const std::string k_EnsembleAttributeMatrix("CellEnsembleData");
const DataPath k_CellEnsembleAttributeMatrixPath = k_DataContainerPath.createChildPath(k_EnsembleAttributeMatrix);
const DataPath k_CrystalStructuresArrayPath = k_CellEnsembleAttributeMatrixPath.createChildPath("CrystalStructures");

const DataPath k_CellFeatureAttributeMatrix = k_DataContainerPath.createChildPath(k_GrainData);
const DataPath k_ActiveArrayPath = k_CellFeatureAttributeMatrix.createChildPath(k_Active);

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
const Uuid k_RenameDataObjectFilterId = *Uuid::FromString("d53c808f-004d-5fac-b125-0fffc8cc78d6");
const FilterHandle k_RenameDataObjectFilterHandle(k_RenameDataObjectFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the Crop Image Geometry
const Uuid k_CropImageGeometryFilterId = *Uuid::FromString("baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf");
const FilterHandle k_CropImageGeometryFilterHandle(k_CropImageGeometryFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the IdentifySample
const Uuid k_IdentifySampleFilterId = *Uuid::FromString("0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a");
const FilterHandle k_IdentifySampleFilterHandle(k_IdentifySampleFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the CopyDataGroup
const Uuid k_CopyDataGroupFilterId = *Uuid::FromString("ac8d51d8-9167-5628-a060-95a8863a76b1");
const FilterHandle k_CopyDataGroupFilterHandle(k_CopyDataGroupFilterId, k_ComplexCorePluginId);

const Uuid k_OrientationAnalysisPluginId = *Uuid::FromString("c09cf01b-014e-5adb-84eb-ea76fc79eeb1");
// Make sure we can instantiate the Convert Orientations
const Uuid k_ConvertOrientationsFilterId = *Uuid::FromString("e5629880-98c4-5656-82b8-c9fe2b9744de");
const FilterHandle k_ConvertOrientationsFilterHandle(k_ConvertOrientationsFilterId, k_OrientationAnalysisPluginId);
// Make sure we can instantiate the EbsdSegmentFeatures
const Uuid k_EbsdSegmentFeaturesFilterId = *Uuid::FromString("7861c691-b821-537b-bd25-dc195578e0ea");
const FilterHandle k_EbsdSegmentFeaturesFilterHandle(k_EbsdSegmentFeaturesFilterId, k_OrientationAnalysisPluginId);
// Make sure we can instantiate the Align Sections Misorientation
const Uuid k_AlignSectionMisorientationFilterId = *Uuid::FromString("4fb2b9de-3124-534b-b914-dbbbdbc14604");
const FilterHandle k_AlignSectionMisorientationFilterHandle(k_AlignSectionMisorientationFilterId, k_OrientationAnalysisPluginId);
// Make sure we can instantiate the NeighborOrientationCorrelation
const Uuid k_NeighborOrientationCorrelationFilterId = *Uuid::FromString("6427cd5e-0ad2-5a24-8847-29f8e0720f4f");
const FilterHandle k_NeighborOrientationCorrelationFilterHandle(k_NeighborOrientationCorrelationFilterId, k_OrientationAnalysisPluginId);
// Make sure we can instantiate the
const Uuid k_BadDataNeighborOrientationCheckFilterId = *Uuid::FromString("f4a7c2df-e9b0-5da9-b745-a862666d6c99");
const FilterHandle k_BadDataNeighborOrientationCheckFilterHandle(k_BadDataNeighborOrientationCheckFilterId, k_OrientationAnalysisPluginId);

const Uuid k_CorePluginId = *Uuid::FromString("65a0a3fc-8c93-5405-8ac6-182e7f313a69");

// Make sure we can instantiate the Align Sections Feature Centroid
const Uuid k_AlignSectionsFeatureCentroidFilterId = *Uuid::FromString("886f8b46-51b6-5682-a289-6febd10b7ef0");
const FilterHandle k_AlignSectionsFeatureCentroidFilterHandle(k_AlignSectionsFeatureCentroidFilterId, k_CorePluginId);

const Uuid k_FindFeaturePhasesObjectsId = *Uuid::FromString("6334ce16-cea5-5643-83b5-9573805873fa");
const FilterHandle k_FindNFeaturePhasesFilterHandle(k_FindFeaturePhasesObjectsId, k_ComplexCorePluginId);
const Uuid k_FindNeighborsObjectsId = *Uuid::FromString("97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac");
const FilterHandle k_FindNeighborsFilterHandle(k_FindNeighborsObjectsId, k_ComplexCorePluginId);

template <typename T>
void CompareDataArrays(const IDataArray& left, const IDataArray& right)
{
  const auto& oldDataStore = left.getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto& newDataStore = right.getIDataStoreRefAs<AbstractDataStore<T>>();
  usize start = 0;
  usize end = oldDataStore.getSize();
  for(usize i = start; i < end; i++)
  {
    if(oldDataStore[i] != newDataStore[i])
    {
      auto oldVal = oldDataStore[i];
      auto newVal = newDataStore[i];
      float diff = std::fabs(static_cast<float>(oldVal - newVal));
      REQUIRE(diff < EPSILON);
      break;
    }
  }
}

struct make_shared_enabler : public complex::Application
{
};

inline DataStructure LoadDataStructure(const fs::path& filepath)
{
  DataStructure exemplarDataStructure;
  REQUIRE(fs::exists(filepath));
  auto result = DREAM3D::ImportDataStructureFromFile(filepath);
  REQUIRE(result.valid());
  return result.value();
}

inline void WriteTestDataStructure(const DataStructure& dataStructure, const fs::path& filepath)
{
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filepath);
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataStructure.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

inline void CompareExemplarToGeneratedData(const DataStructure& dataStructure, const DataStructure& exemplarDataStructure, const DataPath attributeMatrix, const std::string& exemplarDataContainerName)
{
  auto& cellDataGroup = dataStructure.getDataRefAs<DataGroup>(attributeMatrix);
  std::vector<DataPath> selectedCellArrays;

  // Create the vector of selected cell DataPaths
  for(auto& child : cellDataGroup)
  {
    selectedCellArrays.push_back(attributeMatrix.createChildPath(child.second->getName()));
  }

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
    DataType type = generatedDataArray.getDataType();
    // Now generate the path to the exemplar data set in the exemplar data structure.
    std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
    generatedPathVector[0] = exemplarDataContainerName;
    DataPath exemplarDataArrayPath(generatedPathVector);

    // Check to see if there is something to compare against in the exemplar file.
    if(nullptr == exemplarDataStructure.getDataAs<IDataArray>(exemplarDataArrayPath))
    {
      continue;
    }

    auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath);
    DataType exemplarType = exemplarDataArray.getDataType();

    if(type != exemplarType)
    {
      std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray.getName(), exemplarDataArray.getName(), type, exemplarType)
                << std::endl;
      continue;
    }

    switch(type)
    {
    case DataType::boolean: {
      CompareDataArrays<bool>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::int8: {
      CompareDataArrays<int8>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::int16: {
      CompareDataArrays<int16>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::int32: {
      CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::int64: {
      CompareDataArrays<int64>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::uint8: {
      CompareDataArrays<uint8>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::uint16: {
      CompareDataArrays<uint16>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::uint32: {
      CompareDataArrays<uint32>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::uint64: {
      CompareDataArrays<uint64>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::float32: {
      CompareDataArrays<float32>(generatedDataArray, exemplarDataArray);
      break;
    }
    case DataType::float64: {
      CompareDataArrays<float64>(generatedDataArray, exemplarDataArray);
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }
  }
}

} // namespace complex

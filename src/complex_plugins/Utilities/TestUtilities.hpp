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
const std::string k_AvgQuats("AvgQuats");
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
const std::string k_NumCells("NumElements");
const std::string k_Neighborhoods("Neighborhoods");
const std::string k_NeighborhoodList("NeighborhoodList");
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
const DataPath k_NumCellsPath = k_CellFeatureAttributeMatrix.createChildPath(k_NumCells);
const DataPath k_FeaturePhasesPath = k_CellFeatureAttributeMatrix.createChildPath(k_Phases);
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
const Uuid k_CropImageGeometryFilterId = *Uuid::FromString("e6476737-4aa7-48ba-a702-3dfab82c96e2");
const FilterHandle k_CropImageGeometryFilterHandle(k_CropImageGeometryFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the IdentifySample
const Uuid k_IdentifySampleFilterId = *Uuid::FromString("94d47495-5a89-4c7f-a0ee-5ff20e6bd273");
const FilterHandle k_IdentifySampleFilterHandle(k_IdentifySampleFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the CopyDataGroup
const Uuid k_CopyDataGroupFilterId = *Uuid::FromString("ac8d51d8-9167-5628-a060-95a8863a76b1");
const FilterHandle k_CopyDataGroupFilterHandle(k_CopyDataGroupFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the RemoveMinimumSizeFeaturesFilter
const Uuid k_RemoveMinimumSizeFeaturesFilterId = *Uuid::FromString("53ac1638-8934-57b8-b8e5-4b91cdda23ec");
const FilterHandle k_RemoveMinimumSizeFeaturesFilterHandle(k_RemoveMinimumSizeFeaturesFilterId, k_ComplexCorePluginId);
// Make sure we can instantiate the CalculateFeatureSizesFilter
const Uuid k_CalculateFeatureSizesFilterId = *Uuid::FromString("656f144c-a120-5c3b-bee5-06deab438588");
const FilterHandle k_CalculateFeatureSizesFilterHandle(k_CalculateFeatureSizesFilterId, k_ComplexCorePluginId);

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
  auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(attributeMatrix);
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

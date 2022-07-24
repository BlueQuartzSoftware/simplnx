

#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/AlignSectionsFeatureCentroidFilter.hpp"

using namespace complex;

constexpr float EPSILON = 0.00001;

/**
 * Read H5Ebsd File
 * MultiThreshold Objects
 * Convert Orientation Representation (Euler->Quats)
 * Align Sections Misorientation
 * Identify Sample
 * Align Sections Feature Centroid
 *
 * Read DREAM3D File (read the exemplar 'align_sections_feature_centroid.dream3d' file from
 * [Optional] Write out dream3d file
 *
 *
 * Compare the shifts file 'align_sections_feature_centroid.txt' to what was written
 *
 * Compare all the data arrays from the "Exemplar Data / CellData"
 */

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

TEST_CASE("Core::AlignSectionsFeatureCentroidFilter: Small IN100 Pipeline", "[Reconstruction][AlignSectionsFeatureCentroidFilter]")
{
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Make sure we can load the needed filters from the plugins
  const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
  // Make sure we can instantiate the Import Text Filter
  const Uuid k_MultiThresholdObjectsId = *Uuid::FromString("4246245e-1011-4add-8436-0af6bed19228");
  const FilterHandle k_MultiThresholdObjectsFilterHandle(k_MultiThresholdObjectsId, k_ComplexCorePluginId);

  const Uuid k_ImportTextFilterId = *Uuid::FromString("25f7df3e-ca3e-4634-adda-732c0e56efd4");
  const FilterHandle k_ImportTextFilterHandle(k_ImportTextFilterId, k_ComplexCorePluginId);

  const Uuid k_ImportDream3dFilterId = *Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
  const FilterHandle k_ImportDream3dFilterHandle(k_ImportDream3dFilterId, k_ComplexCorePluginId);

  const Uuid k_OrientationAnalysisPluginId = *Uuid::FromString("c09cf01b-014e-5adb-84eb-ea76fc79eeb1");

  const Uuid k_CorePluginId = *Uuid::FromString("65a0a3fc-8c93-5405-8ac6-182e7f313a69");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const std::string k_Quats("Quats");
  const std::string k_Phases("Phases");
  const std::string k_ConfidenceIndex = EbsdLib::Ang::ConfidenceIndex;
  const std::string k_ImageQuality = EbsdLib::Ang::ImageQuality;

  const std::string k_Mask("Mask");
  const std::string k_DataContainer("DataContainer");
  const DataPath k_DataContainerPath({k_DataContainer});

  const std::string k_ExemplarDataContainer("Exemplar Data");
  const DataPath k_ExemplarDataContainerPath({k_ExemplarDataContainer});

  const DataPath k_CalculatedShiftsPath = k_DataContainerPath.createChildPath("Calculated Shifts");

  const DataPath k_CellAttributeMatrix = k_DataContainerPath.createChildPath("CellData");
  const DataPath k_EulersArrayPath = k_CellAttributeMatrix.createChildPath(EbsdLib::CellData::EulerAngles);
  const DataPath k_QuatsArrayPath = k_CellAttributeMatrix.createChildPath(k_Quats);
  const DataPath k_PhasesArrayPath = k_CellAttributeMatrix.createChildPath(k_Phases);
  const DataPath k_ConfidenceIndexArrayPath = k_CellAttributeMatrix.createChildPath(k_ConfidenceIndex);
  const DataPath k_ImageQualityArrayPath = k_CellAttributeMatrix.createChildPath(k_ImageQuality);
  const DataPath k_MaskArrayPath = k_CellAttributeMatrix.createChildPath(k_Mask);

  const DataPath k_CellEnsembleAttributeMatrix = k_DataContainerPath.createChildPath("CellEnsembleData");
  const DataPath k_CrystalStructuresArrayPath = k_CellEnsembleAttributeMatrix.createChildPath(EbsdLib::EnsembleData::CrystalStructures);

  const DataPath k_ExemplarShiftsPath = k_ExemplarDataContainerPath.createChildPath("Exemplar Shifts");

  DataStructure exemplarDataStructure;
  // Read Exemplar DREAM3D File Filter
  {
    constexpr StringLiteral k_ImportFileData = "Import_File_Data";

    auto filter = filterList->createFilter(k_ImportDream3dFilterHandle);
    REQUIRE(nullptr != filter);

    Dream3dImportParameter::ImportData parameter;
    parameter.FilePath = fs::path(fmt::format("{}/TestFiles/align_sections_feature_centroid.dream3d", unit_test::k_DREAM3DDataDir));

    Arguments args;
    args.insertOrAssign(k_ImportFileData, std::make_any<Dream3dImportParameter::ImportData>(parameter));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(exemplarDataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(exemplarDataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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

    auto filter = filterList->createFilter(k_ImportTextFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    // read in the exemplar shift data file
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/TestFiles/align_sections_feature_centroid.txt", unit_test::k_DREAM3DDataDir))));
    args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::int32));
    args.insertOrAssign(k_NTuplesKey, std::make_any<uint64>(116));
    args.insertOrAssign(k_NCompKey, std::make_any<uint64>(6));
    args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(4));
    args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_ExemplarShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(exemplarDataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(exemplarDataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  DataStructure dataStructure;
  // Read the Small IN100 Data set
  {
    constexpr StringLiteral k_ImportFileData = "Import_File_Data";

    auto filter = filterList->createFilter(k_ImportDream3dFilterHandle);
    REQUIRE(nullptr != filter);

    Dream3dImportParameter::ImportData parameter;
    parameter.FilePath = fs::path(fmt::format("{}/TestFiles/Small_IN100.dream3d", unit_test::k_DREAM3DDataDir));

    Arguments args;
    args.insertOrAssign(k_ImportFileData, std::make_any<Dream3dImportParameter::ImportData>(parameter));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // MultiThreshold Objects Filter (From ComplexCore Plugins)
  {
    constexpr StringLiteral k_ArrayThresholds_Key = "array_thresholds";
    constexpr StringLiteral k_CreatedDataPath_Key = "created_data_path";

    auto filter = filterList->createFilter(k_MultiThresholdObjectsFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;

    ArrayThresholdSet arrayThresholdset;
    ArrayThresholdSet::CollectionType thresholds;

    std::shared_ptr<ArrayThreshold> ciThreshold = std::make_shared<ArrayThreshold>();
    ciThreshold->setArrayPath(k_ConfidenceIndexArrayPath);
    ciThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    ciThreshold->setComparisonValue(0.1);
    thresholds.push_back(ciThreshold);

    std::shared_ptr<ArrayThreshold> iqThreshold = std::make_shared<ArrayThreshold>();
    iqThreshold->setArrayPath(k_ImageQualityArrayPath);
    iqThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    iqThreshold->setComparisonValue(120.0);
    thresholds.push_back(iqThreshold);

    arrayThresholdset.setArrayThresholds(thresholds);

    args.insertOrAssign(k_ArrayThresholds_Key, std::make_any<ArrayThresholdsParameter::ValueType>(arrayThresholdset));
    args.insertOrAssign(k_CreatedDataPath_Key, std::make_any<DataPath>(k_MaskArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  {
    const Uuid k_ConvertOrientationsFilterId = *Uuid::FromString("e5629880-98c4-5656-82b8-c9fe2b9744de");
    const FilterHandle k_ConvertOrientationsFilterHandle(k_ConvertOrientationsFilterId, k_OrientationAnalysisPluginId);

    auto filter = filterList->createFilter(k_ConvertOrientationsFilterHandle);
    REQUIRE(nullptr != filter);

    // Parameter Keys from AlignSectionsMisorientation. If those change these will need to be updated
    constexpr StringLiteral k_InputType_Key = "InputType";
    constexpr StringLiteral k_OutputType_Key = "OutputType";
    constexpr StringLiteral k_InputOrientationArrayPath_Key = "InputOrientationArrayPath";
    constexpr StringLiteral k_OutputOrientationArrayName_Key = "OutputOrientationArrayName";

    Arguments args;
    args.insertOrAssign(k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(2));
    args.insertOrAssign(k_InputOrientationArrayPath_Key, std::make_any<DataPath>(k_EulersArrayPath));
    args.insertOrAssign(k_OutputOrientationArrayName_Key, std::make_any<DataPath>(k_QuatsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Align Sections Misorientation Filter (From OrientationAnalysis Plugin)
  {
    const Uuid k_AlignSectionMisorientationFilterId = *Uuid::FromString("4fb2b9de-3124-534b-b914-dbbbdbc14604");
    const FilterHandle k_AlignSectionMisorientationFilterHandle(k_AlignSectionMisorientationFilterId, k_OrientationAnalysisPluginId);

    auto filter = filterList->createFilter(k_AlignSectionMisorientationFilterHandle);
    REQUIRE(nullptr != filter);

    // Parameter Keys
    constexpr StringLiteral k_WriteAlignmentShifts_Key = "WriteAlignmentShifts";
    constexpr StringLiteral k_AlignmentShiftFileName_Key = "AlignmentShiftFileName";

    constexpr StringLiteral k_MisorientationTolerance_Key = "MisorientationTolerance";

    constexpr StringLiteral k_GoodVoxels_Key = "UseGoodVoxels";
    constexpr StringLiteral k_GoodVoxelsArrayPath_Key = "GoodVoxelsArrayPath";

    constexpr StringLiteral k_QuatsArrayPath_Key = "QuatsArrayPath";
    constexpr StringLiteral k_CellPhasesArrayPath_Key = "CellPhasesArrayPath";
    constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";

    constexpr StringLiteral k_SelectedImageGeometry_Key = "SelectedImageGeometryPath";
    constexpr StringLiteral k_SelectedCellDataGroup_Key = "SelectedCellDataPath";

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/AlignSectionsMisorientation_1.txt", unit_test::k_BinaryDir))));

    args.insertOrAssign(k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

    args.insertOrAssign(k_GoodVoxels_Key, std::make_any<bool>(true));
    args.insertOrAssign(k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(k_MaskArrayPath));

    args.insertOrAssign(k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));

    args.insertOrAssign(k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    args.insertOrAssign(k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(k_SelectedCellDataGroup_Key, std::make_any<DataPath>(k_CellAttributeMatrix));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Identify Sample Filter
  {
    const Uuid k_IdentifySampleFilterId = *Uuid::FromString("0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a");
    const FilterHandle k_IdentifySampleFilterHandle(k_IdentifySampleFilterId, k_ComplexCorePluginId);

    auto filter = filterList->createFilter(k_IdentifySampleFilterHandle);
    REQUIRE(nullptr != filter);

    // Parameter Keys
    constexpr StringLiteral k_FillHoles_Key = "fill_holes";
    constexpr StringLiteral k_ImageGeom_Key = "image_geometry";
    constexpr StringLiteral k_GoodVoxels_Key = "good_voxels";

    Arguments args;
    args.insertOrAssign(k_FillHoles_Key, std::make_any<BoolParameter::ValueType>(false));
    args.insertOrAssign(k_ImageGeom_Key, std::make_any<GeometrySelectionParameter::ValueType>(k_DataContainerPath));
    args.insertOrAssign(k_GoodVoxels_Key, std::make_any<ArraySelectionParameter::ValueType>(k_MaskArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Align Sections Feature Centroid Filter
  {
    const Uuid k_AlignSectionsFeatureCentroidFilterId = *Uuid::FromString("886f8b46-51b6-5682-a289-6febd10b7ef0");
    const FilterHandle k_IdentifySampleFilterHandle(k_AlignSectionsFeatureCentroidFilterId, k_CorePluginId);

    auto filter = filterList->createFilter(k_IdentifySampleFilterHandle);
    REQUIRE(nullptr != filter);

    // Parameter Keys
    constexpr StringLiteral k_WriteAlignmentShifts_Key = "WriteAlignmentShifts";
    constexpr StringLiteral k_AlignmentShiftFileName_Key = "AlignmentShiftFileName";
    constexpr StringLiteral k_UseReferenceSlice_Key = "UseReferenceSlice";
    constexpr StringLiteral k_ReferenceSlice_Key = "ReferenceSlice";
    constexpr StringLiteral k_GoodVoxelsArrayPath_Key = "GoodVoxelsArrayPath";
    constexpr StringLiteral k_SelectedImageGeometry_Key = "SelectedImageGeometryPath";
    constexpr StringLiteral k_SelectedCellDataGroup_Key = "SelectedCellDataPath";

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(k_AlignmentShiftFileName_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/AlignSectionsFeatureCentroid_1.txt", unit_test::k_BinaryDir))));
    args.insertOrAssign(k_UseReferenceSlice_Key, std::make_any<bool>(true));
    args.insertOrAssign(k_ReferenceSlice_Key, std::make_any<int32>(0));
    args.insertOrAssign(k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(k_MaskArrayPath));
    args.insertOrAssign(k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(k_SelectedCellDataGroup_Key, std::make_any<DataPath>(k_CellAttributeMatrix));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Use the Read Text File Filter to read in the generated Shift File and compare with exemplar
  {
    static constexpr StringLiteral k_InputFileKey = "input_file";
    static constexpr StringLiteral k_ScalarTypeKey = "scalar_type";
    static constexpr StringLiteral k_NTuplesKey = "n_tuples";
    static constexpr StringLiteral k_NCompKey = "n_comp";
    static constexpr StringLiteral k_NSkipLinesKey = "n_skip_lines";
    static constexpr StringLiteral k_DelimiterChoiceKey = "delimiter_choice";
    static constexpr StringLiteral k_DataArrayKey = "output_data_array";

    // Compare the output of the shifts file with the exemplar file

    auto filter = filterList->createFilter(k_ImportTextFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/AlignSectionsFeatureCentroid_1.txt", unit_test::k_BinaryDir))));
    args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::int32));
    args.insertOrAssign(k_NTuplesKey, std::make_any<uint64>(116));
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

    const auto& calcShifts = dataStructure.getDataRefAs<Int32Array>(k_CalculatedShiftsPath);
    const auto& exemplarShifts = exemplarDataStructure.getDataRefAs<Int32Array>(k_ExemplarShiftsPath);

    size_t numElements = calcShifts.getSize();
    for(size_t i = 0; i < numElements; i++)
    {
      if(calcShifts[i] != exemplarShifts[i])
      {
        REQUIRE(calcShifts[i] == exemplarShifts[i]);
      }
    }
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    auto& cellDataGroup = dataStructure.getDataRefAs<DataGroup>(k_CellAttributeMatrix);
    std::vector<DataPath> selectedCellArrays;

    // Create the vector of selected cell DataPaths
    for(auto& child : cellDataGroup)
    {
      selectedCellArrays.push_back(k_CellAttributeMatrix.createChildPath(child.second->getName()));
    }

    for(const auto& cellArrayPath : selectedCellArrays)
    {
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      DataType type = generatedDataArray.getDataType();

      // Now generate the path to the exemplar data set in the exemplar data structure.
      std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
      generatedPathVector[0] = k_ExemplarDataContainer;
      DataPath exemplarDataArrayPath(generatedPathVector);
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

  {
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/align_sections_feature_centroid.dream3d", unit_test::k_BinaryDir));
    H5::FileWriter fileWriter = std::move(result.value());

    herr_t err = dataStructure.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  }
}

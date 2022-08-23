#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/AlignSectionsMisorientationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Common/Types.hpp"
#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "complex_plugins/Utilities/SmallIN100Utilties.hpp"
#include "complex_plugins/Utilities/TestUtilities.hpp"

using namespace complex;

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
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  const DataPath k_ExemplarShiftsPath = k_ExemplarDataContainerPath.createChildPath("Exemplar Shifts");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/TestFiles/6_6_align_sections_misorientation.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure exemplarDataStructure = complex::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/Small_IN100.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

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
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/TestFiles/align_sections_misorientation.txt", unit_test::k_DREAM3DDataDir))));
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

  // MultiThreshold Objects Filter (From ComplexCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  // Align Sections Misorientation Filter (From OrientationAnalysis Plugin)
  {
    Arguments args;
    AlignSectionsMisorientationFilter filter;
    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_WriteAlignmentShifts_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_AlignmentShiftFileName_Key,
                        std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/AlignSectionsMisorientation_1.txt", unit_test::k_BinaryDir))));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_GoodVoxels_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(k_MaskArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedCellDataGroup_Key, std::make_any<DataPath>(k_CellAttributeMatrix));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Use the Read Text File Filter to read in the generated Shift File
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
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/AlignSectionsMisorientation_1.txt", unit_test::k_BinaryDir))));
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

  WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_misorientation.dream3d", unit_test::k_BinaryTestOutputDir));
}

#if 0
{
  "isDisabled": false,
  "name": "align_sections_featur_centroid.d3dpipeline",
  "pinnedParams": [],
  "pipeline": [
    {
      "args": {
        "CellAttributeMatrixName": "Data Container/CellData",
        "CellEnsembleAttributeMatrixName": "Data Container/CellEnsembleData",
        "DataContainerName": "Exemplar Data",
        "ReadH5EbsdFilter": {
          "endSlice": 117,
          "eulerRepresentation": 0,
          "hdf5DataPaths": [
            "Confidence Index",
            "EulerAngles",
            "Image Quality",
            "Phases"
          ],
          "inputFilePath": "SmallIN100.h5ebsd",
          "startSlice": 1,
          "useRecommendedTransform": true
        }
      },
      "filter": {
        "name": "complex::ReadH5EbsdFilter",
        "uuid": "4ef7f56b-616e-5a80-9e68-1da8f35ad235"
      },
      "isDisabled": false
    },
    {
      "args": {
        "array_thresholds": {
          "inverted": false,
          "thresholds": [
            {
              "array_path": "Data Container/CellData/Confidence Index",
              "comparison": 0,
              "inverted": false,
              "type": "array",
              "union": 0,
              "value": 0.1
            },
            {
              "array_path": "Data Container/CellData/Image Quality",
              "comparison": 0,
              "inverted": false,
              "type": "array",
              "union": 0,
              "value": 120.0
            }
          ],
          "type": "collection",
          "union": 0
        },
        "created_data_path": "Data Container/CellData/Mask"
      },
      "filter": {
        "name": "complex::MultiThresholdObjects",
        "uuid": "4246245e-1011-4add-8436-0af6bed19228"
      },
      "isDisabled": false
    },
    {
      "args": {
        "InputOrientationArrayPath": "Data Container/CellData/EulerAngles",
        "InputType": 0,
        "OutputOrientationArrayName": "Data Container/CellData/Quats",
        "OutputType": 2
      },
      "filter": {
        "name": "complex::ConvertOrientations",
        "uuid": "e5629880-98c4-5656-82b8-c9fe2b9744de"
      },
      "isDisabled": false
    },
    {
      "args": {
        "AlignmentShiftFileName": "align_sections_misorientation.txt",
        "CellPhasesArrayPath": "Data Container/CellData/Phases",
        "CrystalStructuresArrayPath": "Data Container/CellEnsembleData/CrystalStructures",
        "GoodVoxelsArrayPath": "Data Container/CellData/Mask",
        "MisorientationTolerance": 5.0,
        "QuatsArrayPath": "Data Container/CellData/Quats",
        "SelectedCellDataPath": "Data Container/CellData",
        "SelectedImageGeometryPath": "Exemplar Data",
        "UseGoodVoxels": true,
        "WriteAlignmentShifts": true
      },
      "filter": {
        "name": "complex::AlignSectionsMisorientationFilter",
        "uuid": "4fb2b9de-3124-534b-b914-dbbbdbc14604"
      },
      "isDisabled": false
    },
    {
      "args": {
        "Export_File_Path": "align_sections_misorientations.dream3d"
      },
      "filter": {
        "name": "complex::ExportDREAM3DFilter",
        "uuid": "b3a95784-2ced-11ec-8d3d-0242ac130003"
      },
      "isDisabled": false
    },
    {
      "args": {
        "Import_File_Data": {
          "datapaths": [
            "Exemplar Data/CellEnsembleData",
            "Exemplar Data/CellEnsembleData/CrystalStructures",
            "Exemplar Data/CellEnsembleData/LatticeConstants",
            "Exemplar Data",
            "Exemplar Data/CellData",
            "Exemplar Data/CellData/Confidence Index",
            "Exemplar Data/CellData/EulerAngles",
            "Exemplar Data/CellData/Image Quality",
            "Exemplar Data/CellData/Phases",
            "Exemplar Data/CellData/Mask",
            "Exemplar Data/CellData/Quats"
          ],
          "filepath": "DREAM3D_Data/TestFiles/align_sections_misorientation.dream3d"
        }
      },
      "filter": {
        "name": "complex::ImportDREAM3DFilter",
        "uuid": "0dbd31c7-19e0-4077-83ef-f4a6459a0e2d"
      },
      "isDisabled": false
    }
  ],
  "workflowParams": []
}
#endif

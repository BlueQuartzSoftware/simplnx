#include "OrientationAnalysis/Filters/ReadH5EbsdFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/Parameters/ReadH5EbsdFileParameter.h"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace
{
inline const std::string k_MaterialName("MaterialName");
}

TEST_CASE("OrientationAnalysis::ReadH5Ebsd: Valid filter execution", "[OrientationAnalysis][ReadH5Ebsd]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d.tar.gz", "Small_IN100.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_h5ebsd.tar.gz", "Small_IN100.dream3d");

  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  // ReadH5EbsdFilter
  DataStructure dataStructure;
  {
    ReadH5EbsdFilter filter;
    Arguments args;

    ReadH5EbsdFileParameter::ValueType h5ebsdParamVal;
    h5ebsdParamVal.inputFilePath = fmt::format("{}/Small_IN100.h5ebsd", unit_test::k_TestFilesDir);
    h5ebsdParamVal.startSlice = 1;
    h5ebsdParamVal.endSlice = 117;
    h5ebsdParamVal.eulerRepresentation = EbsdLib::AngleRepresentation::Radians;
    h5ebsdParamVal.selectedArrayNames = {Constants::k_ConfidenceIndex, Constants::k_EulerAngles, Constants::k_Fit, Constants::k_ImageQuality, Constants::k_Phases, Constants::k_SEMSignal};
    h5ebsdParamVal.useRecommendedTransform = true;

    args.insertOrAssign(ReadH5EbsdFilter::k_ReadH5EbsdParameter_Key, std::make_any<ReadH5EbsdFileParameter::ValueType>(h5ebsdParamVal));
    args.insertOrAssign(ReadH5EbsdFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
    args.insertOrAssign(ReadH5EbsdFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(ReadH5EbsdFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_EnsembleAttributeMatrix));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellAttributeMatrix);
    auto& cellEnsembleDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellEnsembleAttributeMatrixPath);
    std::vector<DataPath> selectedArrays;

    // Create the vector of selected cell DataPaths
    for(auto& child : cellDataGroup)
    {
      selectedArrays.push_back(Constants::k_CellAttributeMatrix.createChildPath(child.second->getName()));
    }
    for(auto& child : cellEnsembleDataGroup)
    {
      selectedArrays.push_back(Constants::k_CellEnsembleAttributeMatrixPath.createChildPath(child.second->getName()));
    }

    for(const auto& arrayPath : selectedArrays)
    {
      if(arrayPath.getTargetName() == ::k_MaterialName)
      {
        continue;
      }
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(arrayPath);
      DataType type = generatedDataArray.getDataType();
      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(arrayPath);
      DataType exemplarType = exemplarDataArray.getDataType();

      if(type != exemplarType)
      {
        std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray.getName(), exemplarDataArray.getName(),
                                 fmt::underlying(type), fmt::underlying(exemplarType))
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
}

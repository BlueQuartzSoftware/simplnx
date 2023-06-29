#include "OrientationAnalysis/Filters/ReadH5EbsdFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/Parameters/H5EbsdReaderParameter.h"

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::UnitTest;

TEST_CASE("OrientationAnalysis::ReadH5Ebsd: Valid filter execution", "[OrientationAnalysis][ReadH5Ebsd]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel1(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "Small_IN100_dream3d.tar.gz", "Small_IN100.dream3d");

  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "Small_IN100_h5ebsd.tar.gz", "Small_IN100.dream3d");

  std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  // ReadH5EbsdFilter
  DataStructure dataStructure;
  {
    ReadH5EbsdFilter filter;
    Arguments args;

    H5EbsdReaderParameter::ValueType h5ebsdParamVal;
    h5ebsdParamVal.inputFilePath = fmt::format("{}/Small_IN100.h5ebsd", unit_test::k_TestFilesDir);
    h5ebsdParamVal.startSlice = 1;
    h5ebsdParamVal.endSlice = 117;
    h5ebsdParamVal.eulerRepresentation = EbsdLib::AngleRepresentation::Radians;
    h5ebsdParamVal.hdf5DataPaths = {Constants::k_ConfidenceIndex, Constants::k_EulerAngles, Constants::k_Fit, Constants::k_ImageQuality, Constants::k_Phases, Constants::k_SEMSignal};
    h5ebsdParamVal.useRecommendedTransform = true;

    args.insertOrAssign(ReadH5EbsdFilter::k_ReadH5EbsdFilter_Key, std::make_any<H5EbsdReaderParameter::ValueType>(h5ebsdParamVal));
    args.insertOrAssign(ReadH5EbsdFilter::k_DataContainerName_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
    args.insertOrAssign(ReadH5EbsdFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(ReadH5EbsdFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_EnsembleAttributeMatrix));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
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
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(arrayPath);
      DataType type = generatedDataArray.getDataType();
      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(arrayPath);
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
}

#include "OrientationAnalysis/Filters/ReadH5EbsdFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/Parameters/ReadH5EbsdFileParameter.h"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;
namespace
{
const std::string k_MaterialName("MaterialName");
}

TEST_CASE("OrientationAnalysis::ReadH5Ebsd: Valid filter execution", "[OrientationAnalysis][ReadH5Ebsd]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_h5ebsd.tar.gz", "Small_IN100.h5ebsd");

  auto* filterList = Application::Instance()->getFilterList();

  auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  DataStructure dataStructure;
  {
    ReadH5EbsdFilter filter;
    Arguments args;

    ReadH5EbsdFileParameter::ValueType h5ebsdParamVal;
    h5ebsdParamVal.inputFilePath = fmt::format("{}/Small_IN100.h5ebsd", unit_test::k_TestFilesDir);
    h5ebsdParamVal.startSlice = 1;
    h5ebsdParamVal.endSlice = 117;
    h5ebsdParamVal.eulerRepresentation = ebsdlib::AngleRepresentation::Radians;
    h5ebsdParamVal.selectedArrayNames = {Constants::k_ConfidenceIndex, Constants::k_EulerAngles, Constants::k_Fit, Constants::k_ImageQuality, Constants::k_Phases, Constants::k_SEMSignal};
    h5ebsdParamVal.useRecommendedTransform = true;

    args.insertOrAssign(ReadH5EbsdFilter::k_ReadH5EbsdParameter_Key, std::make_any<ReadH5EbsdFileParameter::ValueType>(h5ebsdParamVal));
    args.insertOrAssign(ReadH5EbsdFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
    args.insertOrAssign(ReadH5EbsdFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(ReadH5EbsdFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_EnsembleAttributeMatrix));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_h5ebsd_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellAttributeMatrix));
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellAttributeMatrix);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellEnsembleAttributeMatrixPath));
    auto& cellEnsembleDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellEnsembleAttributeMatrixPath);
    std::vector<DataPath> selectedArrays;

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
        // MaterialName is a StringArray. The numeric-array switch cannot
        // compare it.
        continue;
      }
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(arrayPath));
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(arrayPath);
      DataType type = generatedDataArray.getDataType();
      REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(arrayPath));
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5EbsdFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ReadH5EbsdFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadH5EbsdFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadH5EbsdFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ReadH5EbsdFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ReadH5EbsdFilter::k_CreatedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(ReadH5EbsdFilter::k_CellAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(ReadH5EbsdFilter::k_CellEnsembleAttributeMatrixName_Key) == "TestName");

      const auto h5EbsdValue = args.value<ReadH5EbsdFileParameter::ValueType>(ReadH5EbsdFilter::k_ReadH5EbsdParameter_Key);
      CHECK(h5EbsdValue.inputFilePath == "/test/path/file.h5ebsd");
      CHECK(h5EbsdValue.startSlice == 0);
      CHECK(h5EbsdValue.endSlice == 10);
      CHECK(h5EbsdValue.useRecommendedTransform == true);
      CHECK(h5EbsdValue.selectedArrayNames == std::vector<std::string>{"Confidence Index", "Image Quality"});
    }
  }
}

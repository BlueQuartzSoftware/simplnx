#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/WriteINLFileFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/IO/TSL/AngConstants.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <nonstd/span.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const std::string k_MaterialName = "MaterialName";
const std::string k_NumFeatures = "NumFeatures";

const fs::path k_ExemplarFilePath = fs::path(fmt::format("{}/INL_writer/INLWriterExemplar.inl", unit_test::k_TestFilesDir));
const fs::path k_WrittenFilePath = fs::path(fmt::format("{}/WriteINLFile.inl", unit_test::k_BinaryTestOutputDir));

void CompareResults()
{
  REQUIRE(fs::exists(k_WrittenFilePath));
  REQUIRE(fs::exists(k_ExemplarFilePath));
  std::ifstream exemplarFile(k_ExemplarFilePath, std::ios_base::in | std::ios_base::binary);
  if(!exemplarFile.is_open())
  {
    throw std::runtime_error(fmt::format("{} must be stream readable!", k_ExemplarFilePath.string()));
  }
  // Ignore the timestamp and version header lines.
  exemplarFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  exemplarFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::ifstream generatedFile(k_WrittenFilePath, std::ios_base::in | std::ios_base::binary);
  if(!exemplarFile.is_open())
  {
    throw std::runtime_error(fmt::format("{} must be stream readable!", k_WrittenFilePath.string()));
  }

  // Ignore the generated version line.
  generatedFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::string exemplar = "";
  std::string generated = "";
  for(usize i = 0; i < 1024; i++)
  {
    if(exemplarFile.eof())
    {
      break;
    }

    std::getline(exemplarFile, exemplar, '\n');
    std::getline(generatedFile, generated, '\n');

    std::string exem = exemplar.substr(0, exemplar.find('\r'));

    std::string gen = generated.substr(0, generated.find('\n'));
    REQUIRE(exem == gen);
  }
}
} // namespace

TEST_CASE("OrientationAnalysis::WriteINLFileFilter: Valid Filter Execution", "[OrientationAnalysis][WriteINLFileFilter]")
{
  UnitTest::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "INL_writer.tar.gz", "INL_writer");

  WriteINLFileFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/INL_writer/6_6_INL_writer.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  args.insertOrAssign(WriteINLFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_WrittenFilePath));
  args.insertOrAssign(WriteINLFileFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100})));
  args.insertOrAssign(WriteINLFileFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
  args.insertOrAssign(WriteINLFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_Phases})));
  args.insertOrAssign(WriteINLFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));
  args.insertOrAssign(WriteINLFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures})));
  args.insertOrAssign(WriteINLFileFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, ::k_MaterialName})));
  args.insertOrAssign(WriteINLFileFilter::k_NumFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, ::k_NumFeatures})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  ::CompareResults();

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteINLFileFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][WriteINLFileFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteINLFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteINLFileFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WriteINLFileFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteINLFileFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<DataPath>(WriteINLFileFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteINLFileFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteINLFileFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteINLFileFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteINLFileFilter::k_MaterialNameArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteINLFileFilter::k_NumFeaturesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteINLFileFilter::k_ImageGeomPath_Key) == DataPath({"DataContainer"}));
    }
  }
}

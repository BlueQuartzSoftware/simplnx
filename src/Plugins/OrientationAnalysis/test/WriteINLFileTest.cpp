#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/WriteINLFileFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
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
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
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

TEST_CASE("OrientationAnalysis::WriteINLFileFilter: cancellation leaves the existing output untouched", "[OrientationAnalysis][WriteINLFileFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "INL_writer.tar.gz", "INL_writer");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/INL_writer/6_6_INL_writer.dream3d", unit_test::k_TestFilesDir)));

  const fs::path outputPath = fs::temp_directory_path() / "nx_cancel_inl_writer.inl";
  auto outputFileGuard = MakeScopeGuard([&outputPath]() noexcept {
    std::error_code errorCode;
    fs::remove(outputPath, errorCode);
  });
  const std::string sentinel = "PRE-EXISTING USER OUTPUT";
  {
    std::ofstream outputStream(outputPath, std::ios::binary | std::ios::trunc);
    REQUIRE(outputStream.is_open());
    outputStream << sentinel;
  }

  WriteINLFileFilter filter;
  Arguments args;
  args.insertOrAssign(WriteINLFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteINLFileFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100})));
  args.insertOrAssign(WriteINLFileFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
  args.insertOrAssign(WriteINLFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_Phases})));
  args.insertOrAssign(WriteINLFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));
  args.insertOrAssign(WriteINLFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures})));
  args.insertOrAssign(WriteINLFileFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, ::k_MaterialName})));
  args.insertOrAssign(WriteINLFileFilter::k_NumFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, ::k_NumFeatures})));

  std::atomic_bool shouldCancel = false;
  IFilter::MessageHandler cancelOnFirstMessage{[&shouldCancel](const IFilter::Message&) { shouldCancel.store(true); }};
  auto executeResult = scope.executeFilter(filter, dataStructure, args, nullptr, cancelOnFirstMessage, shouldCancel);

  REQUIRE(shouldCancel.load());
  REQUIRE(executeResult.result.invalid());
  std::ifstream inputStream(outputPath, std::ios::binary);
  REQUIRE(inputStream.is_open());
  const std::string contents((std::istreambuf_iterator<char>(inputStream)), std::istreambuf_iterator<char>());
  REQUIRE(contents == sentinel);
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

TEST_CASE("OrientationAnalysis::WriteINLFileFilter: Geometry cell tuple counts", "[OrientationAnalysis][WriteINLFileFilter][TupleCount]")
{
  // Cases: all short, all long, all matching outside Cell Data, then each input short.
  const int testCase = GENERATE(0, 1, 2, 3, 4, 5);
  CAPTURE(testCase);
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "TupleGeometry");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 2});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 2, 2}, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);

  WriteINLFileFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteINLFileFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({"TupleGeometry"})));
  const usize commonCount = testCase == 0 ? 4 : (testCase == 1 ? 12 : 8);
  const usize count0 = testCase == 3 ? 4 : commonCount;
  auto* inputAM0Ptr = AttributeMatrix::Create(dataStructure, "Input0", {count0});
  REQUIRE(inputAM0Ptr != nullptr);
  REQUIRE(DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {count0}, {1}, inputAM0Ptr->getId()) != nullptr);
  args.insertOrAssign(WriteINLFileFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input0", "FeatureIds"})));
  const usize count1 = testCase == 4 ? 4 : commonCount;
  auto* inputAM1Ptr = AttributeMatrix::Create(dataStructure, "Input1", {count1});
  REQUIRE(inputAM1Ptr != nullptr);
  REQUIRE(DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "Phases", {count1}, {1}, inputAM1Ptr->getId()) != nullptr);
  args.insertOrAssign(WriteINLFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Input1", "Phases"})));
  const usize count2 = testCase == 5 ? 4 : commonCount;
  auto* inputAM2Ptr = AttributeMatrix::Create(dataStructure, "Input2", {count2});
  REQUIRE(inputAM2Ptr != nullptr);
  REQUIRE(DataArray<float32>::CreateWithStore<DataStore<float32>>(dataStructure, "EulerAngles", {count2}, {3}, inputAM2Ptr->getId()) != nullptr);
  args.insertOrAssign(WriteINLFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"Input2", "EulerAngles"})));
  REQUIRE(UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, "CrystalStructures", {2}, {1}) != nullptr);
  args.insertOrAssign(WriteINLFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"CrystalStructures"})));
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Materials", {2}, {"Unknown", "Phase1"}) != nullptr);
  REQUIRE(Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "NumFeatures", {2}, {1}) != nullptr);
  args.insertOrAssign(WriteINLFileFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"Materials"})));
  args.insertOrAssign(WriteINLFileFilter::k_NumFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath({"NumFeatures"})));
  args.insertOrAssign(WriteINLFileFilter::k_OutputFile_Key, std::make_any<fs::path>(fs::path(unit_test::k_BinaryTestOutputDir) / "tuple-count.inl"));

  const auto result = filter.preflight(dataStructure, args);
  if(testCase == 2)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    {
      const auto& errorRef = result.outputActions.errors()[0];
      CHECK(errorRef.code == -78431);
      const std::vector<DataPath> inputPaths = {DataPath({"Input0", "FeatureIds"}), DataPath({"Input1", "Phases"}), DataPath({"Input2", "EulerAngles"})};
      const usize inputIndex = testCase >= 3 ? static_cast<usize>(testCase - 3) : 0;
      CHECK(errorRef.message.find(inputPaths[inputIndex].toString()) != std::string::npos);
      CHECK(errorRef.message.find(testCase == 1 ? "12 tuples" : "4 tuples") != std::string::npos);
      CHECK(errorRef.message.find("8 cells") != std::string::npos);
      CHECK(errorRef.message.find("TupleGeometry") != std::string::npos);
    }
  }
}

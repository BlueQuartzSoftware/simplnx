#include "SimplnxCore/Filters/ExtractPipelineToFileFilter.hpp"
#include "SimplnxCore/Filters/ReadDREAM3DFilter.hpp"
#include "SimplnxCore/Filters/WriteDREAM3DFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include <catch2/catch.hpp>

#include <nlohmann/json.hpp>

#include <any>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <typeinfo>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const FilterHandle k_BadHandle(Uuid{}, Uuid{});
// TestOne Plugin
constexpr Uuid k_Test1PluginId = *Uuid::FromString("01ff618b-781f-4ac0-b9ac-43f26ce1854f");
constexpr Uuid k_Test1FilterId = *Uuid::FromString("5502c3f7-37a8-4a86-b003-1c856be02491");
const FilterHandle k_Test1FilterHandle(k_Test1FilterId, k_Test1PluginId);

// TestTwo Plugin
constexpr Uuid k_Test2PluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f33ce1854e");
constexpr Uuid k_Test2FilterId = *Uuid::FromString("ad9cf22b-bc5e-41d6-b02e-bb49ffd12c04");
const FilterHandle k_Test2FilterHandle(k_Test2FilterId, k_Test2PluginId);

const Uuid k_CreateDataGroupId = *Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101");
const Uuid k_CorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const FilterHandle k_CreateDataGroupHandle(k_CreateDataGroupId, k_CorePluginId);

const DataPath k_DeferredActionPath({"foo"});

class DeferredActionTestFilter : public IFilter
{
public:
  DeferredActionTestFilter() = default;

  ~DeferredActionTestFilter() noexcept override = default;

  DeferredActionTestFilter(const DeferredActionTestFilter&) = delete;
  DeferredActionTestFilter(DeferredActionTestFilter&&) noexcept = delete;

  DeferredActionTestFilter& operator=(const DeferredActionTestFilter&) = delete;
  DeferredActionTestFilter& operator=(DeferredActionTestFilter&&) noexcept = delete;

  std::string name() const override
  {
    return "DeferredActionTestFilter";
  }

  std::string className() const override
  {
    return "DeferredActionTestFilter";
  }

  Uuid uuid() const override
  {
    static constexpr Uuid uuid = *Uuid::FromString("907b9eb7-c48e-4666-9d9d-21cc70f426f1");
    return uuid;
  }

  std::string humanName() const override
  {
    return "Deferred Action Test Filter";
  }

  Parameters parameters() const override
  {
    return {};
  }

  VersionType parametersVersion() const override
  {
    return 1;
  }

  UniquePointer clone() const override
  {
    return std::make_unique<DeferredActionTestFilter>();
  }

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override
  {
    OutputActions outputActions;
    outputActions.appendAction(std::make_unique<CreateArrayAction>(DataType::int32, std::vector<usize>{10}, std::vector<usize>{1}, k_DeferredActionPath));
    outputActions.appendDeferredAction(std::make_unique<DeleteDataAction>(k_DeferredActionPath));
    return {std::move(outputActions)};
  }

  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override
  {
    // object should exist because the delete should happen after execute
    if(dataStructure.getData(k_DeferredActionPath) == nullptr)
    {
      return MakeErrorResult(-1, fmt::format("DataPath '{}' must exist", k_DeferredActionPath.toString()));
    }
    return {};
  }
};
} // namespace

TEST_CASE("PipelineTest:Execute Pipeline")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const AbstractPlugin* test1Plugin = app->getPlugin(k_Test1PluginId);
  REQUIRE(test1Plugin != nullptr);
  auto testFilter1 = filterList->createFilter(k_Test1FilterHandle);
  REQUIRE(testFilter1 != nullptr);
  FilterHandle tf1Handle(testFilter1->uuid(), test1Plugin->getId());

  const AbstractPlugin* test2Plugin = app->getPlugin(k_Test2PluginId);
  REQUIRE(test2Plugin != nullptr);
  auto testFilter2 = filterList->createFilter(k_Test2FilterHandle);
  REQUIRE(testFilter2 != nullptr);
  FilterHandle tf2Handle(testFilter2->uuid(), test2Plugin->getId());

  Pipeline pipeline;
  REQUIRE(pipeline.execute());
  REQUIRE(pipeline.push_front(tf1Handle));

  PipelineFilter* filterNode = dynamic_cast<PipelineFilter*>(pipeline.at(0));
  REQUIRE(filterNode != nullptr);

  Arguments args;
  GeneratedFileListParameter::ValueType defaultValue;
  defaultValue.startIndex = 0;
  defaultValue.endIndex = 1;
  defaultValue.filePrefix = "test_";
  defaultValue.fileExtension = ".txt";
  defaultValue.inputPath = nx::core::unit_test::k_BinaryTestOutputDir;

  const std::vector<std::string> fileList = defaultValue.generate();
  // We need to actually have files available for the test to pass so generate some files
  for(const auto& filepath : fileList)
  {
    std::fstream file(filepath, std::ios_base::out);
    file << "TEST" << std::endl;
  }

  args.insert("param3", std::move(defaultValue));
  filterNode->setArguments(args);

  REQUIRE(pipeline.execute());
  REQUIRE(pipeline.push_back(tf2Handle));
}

TEST_CASE("PipelineTest:Simplnx Pipeline")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  auto filterList = app->getFilterList();
  REQUIRE(filterList->size() != 0);

  const AbstractPlugin* test1Plugin = app->getPlugin(k_Test1PluginId);
  REQUIRE(test1Plugin != nullptr);

  auto testFilter1 = filterList->createFilter(k_Test1FilterHandle);
  REQUIRE(testFilter1 != nullptr);
  FilterHandle tf1Handle(testFilter1->uuid(), test1Plugin->getId());

  Pipeline pipeline;
  std::unique_ptr<PipelineFilter> node = PipelineFilter::Create(tf1Handle);
  REQUIRE(node != nullptr);
  REQUIRE(node->getParameters().empty() == false);
  REQUIRE(pipeline.push_back(std::move(node)));
  REQUIRE(pipeline.push_back(k_Test1FilterHandle));
  REQUIRE(pipeline.push_back(k_Test2FilterHandle));

  auto segment2 = std::make_shared<Pipeline>();
  segment2->push_back(tf1Handle);
  REQUIRE(pipeline.push_back(segment2));

  // Test Moving nodex
  {
    constexpr Pipeline::index_type fromIndex = 1;
    constexpr Pipeline::index_type toIndex = 3;

    auto movedNode = pipeline.at(fromIndex);

    REQUIRE(pipeline.move(fromIndex, toIndex));

    // Check node index
    {
      auto nodeIter = pipeline.find(movedNode);
      const auto nodeIndex = nodeIter - pipeline.begin();
      REQUIRE(nodeIndex == toIndex);
    }

    REQUIRE(pipeline.move(toIndex, fromIndex));

    // Check node index
    {
      auto nodeIter = pipeline.find(movedNode);
      const auto nodeIndex = nodeIter - pipeline.begin();
      REQUIRE(nodeIndex == fromIndex);
    }
  }
}

TEST_CASE("PipelineTest:PipelineJson")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  Pipeline pipeline("test");

  Arguments filter1Args;
  filter1Args.insert("param1", 1.2f);
  filter1Args.insert("param2", true);
  filter1Args.insert("param3", GeneratedFileListParameter::ValueType{});
  pipeline.push_back(k_Test1FilterHandle, filter1Args);

  Arguments filter2Args;
  filter2Args.insert("param1", 42);
  filter2Args.insert("param2", std::string("foobarbaz"));
  filter2Args.insert("param3_index", ChoicesParameter::ValueType{});
  pipeline.push_back(k_Test2FilterHandle, filter2Args);

  nlohmann::json pipelineJson = pipeline.toJson();

  Result<Pipeline> pipeline2Result = Pipeline::FromJson(pipelineJson);

  REQUIRE(pipeline2Result.valid());

  Pipeline pipeline2 = std::move(pipeline2Result.value());

  REQUIRE(pipeline2.getName() == pipeline.getName());
  REQUIRE(pipeline2.size() == pipeline.size());
  usize size = pipeline2.size();
  for(usize i = 0; i < size; i++)
  {
    const auto* originalNode = dynamic_cast<const PipelineFilter*>(pipeline.at(i));

    const AbstractPipelineNode* node = pipeline2.at(i);
    REQUIRE(node != nullptr);
    const auto* pipelineFilter = dynamic_cast<const PipelineFilter*>(node);
    REQUIRE(pipelineFilter != nullptr);
    const IFilter* filter = pipelineFilter->getFilter();
    REQUIRE(filter != nullptr);

    Uuid expectedUuid = originalNode->getFilter()->uuid();
    REQUIRE(filter->uuid() == expectedUuid);

    Arguments expectedArgs = originalNode->getArguments();

    Arguments args = pipelineFilter->getArguments();

    REQUIRE(args.size() == expectedArgs.size());

    for(const auto& [key, expectedValue] : expectedArgs)
    {
      REQUIRE(args.contains(key));
      const std::any& value = args.at(key);
      REQUIRE(value.has_value());
      REQUIRE(value.type() == expectedValue.type());
    }
  }

  nlohmann::json pipeline2Json = pipeline2.toJson();

  REQUIRE(pipelineJson == pipeline2Json);
}

TEST_CASE("PipelineTest:Rename Output")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  std::string group1Name = "Foo";
  std::string group2Name = "Bar";

  Arguments args1;
  args1.insert("data_object_path", std::make_any<DataPath>(DataPath({group1Name})));

  Arguments args2;
  args2.insert("data_object_path", std::make_any<DataPath>(DataPath({group1Name, group2Name})));

  Pipeline pipeline("Rename Test Pipeline");
  REQUIRE(pipeline.push_back(k_CreateDataGroupHandle, args1));
  REQUIRE(pipeline.push_back(k_CreateDataGroupHandle, args2));

  REQUIRE(pipeline.preflight());

  // Update arguments to test renaming
  {
    group1Name = "Bizz";
    args1.insertOrAssign("data_object_path", std::make_any<DataPath>(DataPath({group1Name})));

    auto* filterNode = dynamic_cast<PipelineFilter*>(pipeline.at(0));
    REQUIRE(filterNode != nullptr);
    filterNode->setArguments(args1);
  }

  REQUIRE(pipeline.preflight(false, true));

  // Check Node 2 arguments
  {
    auto* filterNode2 = dynamic_cast<PipelineFilter*>(pipeline.at(1));
    REQUIRE(filterNode2 != nullptr);
    auto args2Alt = filterNode2->getArguments();
    const auto& inPath = std::any_cast<DataPath>(args2Alt.at("data_object_path"));
    DataPath targetPath({group1Name, group2Name});
    REQUIRE(inPath == targetPath);
  }
}

TEST_CASE("PipelineTest:Not Renaming")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  std::string group1Name = "Foo";
  std::string group2Name = "Bar";

  Arguments args1;
  args1.insert("data_object_path", std::make_any<DataPath>(DataPath({group1Name})));

  Arguments args2;
  args2.insert("data_object_path", std::make_any<DataPath>(DataPath({group1Name, group2Name})));

  Pipeline pipeline("Rename Test Pipeline");
  REQUIRE(pipeline.push_back(k_CreateDataGroupHandle, args1));
  REQUIRE(pipeline.push_back(k_CreateDataGroupHandle, args2));

  REQUIRE(pipeline.preflight());

  // Update arguments to test renaming
  {
    group1Name = "Bizz";
    args1.insertOrAssign("data_object_path", std::make_any<DataPath>(DataPath({group1Name})));

    auto* filterNode = dynamic_cast<PipelineFilter*>(pipeline.at(0));
    REQUIRE(filterNode != nullptr);
    filterNode->setArguments(args1);
  }

  REQUIRE(pipeline.preflight(false, false));

  // Check Node 2 arguments
  {
    auto* filterNode2 = dynamic_cast<PipelineFilter*>(pipeline.at(1));
    REQUIRE(filterNode2 != nullptr);
    auto args2Alt = filterNode2->getArguments();
    const auto& inPath = std::any_cast<DataPath>(args2Alt.at("data_object_path"));
    DataPath targetPath({group1Name, group2Name});
    REQUIRE(inPath != targetPath);
  }
}

TEST_CASE("PipelineTest:PipelineDeferredActionTest")
{
  Pipeline pipeline;

  REQUIRE(pipeline.push_back(std::make_unique<DeferredActionTestFilter>()));

  DataStructure dataStructure;

  REQUIRE(pipeline.preflight(dataStructure, false));

  // should be deleted in preflight
  DataObject* preflightObject = dataStructure.getData(k_DeferredActionPath);
  REQUIRE(preflightObject == nullptr);

  // DeferredActionTestFilter checks the k_DeferredActionPath hasn't been deleted until after execute
  REQUIRE(pipeline.execute(dataStructure, false));

  // after execute the object will be deleted
  DataObject* executeObject = dataStructure.getData(k_DeferredActionPath);
  REQUIRE(executeObject == nullptr);
}

TEST_CASE("SimplnxCore::Pipeline persistent state: write extract replay", "[SimplnxCore][PipelineCopyState]")
{
  const fs::path outputDirectory = fs::path(unit_test::k_BinaryTestOutputDir.view()) / ("pipeline-state-" + Uuid::GenerateV4().str());
  REQUIRE(fs::create_directories(outputDirectory));
  const auto cleanup = MakeScopeGuard([&outputDirectory]() noexcept {
    std::error_code errorCode;
    fs::remove_all(outputDirectory, errorCode);
  });
  const fs::path missingInput = outputDirectory / "missing-input.dream3d";
  const fs::path writtenFile = outputDirectory / "written.dream3d";
  const fs::path extractedFile = outputDirectory / "extracted.d3dpipeline";
  const fs::path replayFile = outputDirectory / "replayed.dream3d";
  REQUIRE_FALSE(fs::exists(missingInput));

  const ReadDREAM3DFilter reader;
  Arguments readerArgs = reader.getDefaultArguments();
  readerArgs.insertOrAssign(ReadDREAM3DFilter::k_ImportFileData, std::make_any<Dream3dImportParameter::ValueType>(missingInput));
  auto readerPtr = std::make_shared<PipelineFilter>(std::make_unique<ReadDREAM3DFilter>(), readerArgs);
  readerPtr->setDisabled(true);
  readerPtr->setComments("Skip the missing reader during replay.");

  const WriteDREAM3DFilter writer;
  Arguments writerArgs = writer.getDefaultArguments();
  writerArgs.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, writtenFile);
  writerArgs.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
  auto writerPtr = std::make_shared<PipelineFilter>(std::make_unique<WriteDREAM3DFilter>(), writerArgs);
  writerPtr->setComments("Keep the writer comment in the embedded pipeline.");

  Pipeline pipeline("Persistent state write extract replay");
  REQUIRE(pipeline.push_back(readerPtr));
  REQUIRE(pipeline.push_back(writerPtr));
  DataStructure dataStructure;
  REQUIRE(pipeline.preflight(dataStructure, false));
  REQUIRE(pipeline.execute(dataStructure, false));
  REQUIRE(fs::exists(writtenFile));
  REQUIRE_FALSE(fs::exists(missingInput));
  CHECK(dataStructure.getAllDataPaths().empty());

  auto embeddedResult = DREAM3D::ImportPipelineJsonFromFile(writtenFile);
  SIMPLNX_RESULT_REQUIRE_VALID(embeddedResult);
  const auto& embeddedRef = embeddedResult.value();
  REQUIRE(embeddedRef.at("pipeline").size() == 2);
  CHECK(embeddedRef.at("pipeline").at(0).at("isDisabled") == true);
  CHECK(embeddedRef.at("pipeline").at(0).at("comments") == "Skip the missing reader during replay.");
  CHECK(embeddedRef.at("pipeline").at(1).at("isDisabled") == false);
  CHECK(embeddedRef.at("pipeline").at(1).at("comments") == "Keep the writer comment in the embedded pipeline.");

  const ExtractPipelineToFileFilter extractor;
  Arguments extractorArgs = extractor.getDefaultArguments();
  extractorArgs.insertOrAssign(ExtractPipelineToFileFilter::k_ImportFileData, writtenFile);
  extractorArgs.insertOrAssign(ExtractPipelineToFileFilter::k_OutputFile, extractedFile);
  auto extractPreflightResult = extractor.preflight(dataStructure, extractorArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(extractPreflightResult.outputActions);
  auto extractResult = extractor.execute(dataStructure, extractorArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(extractResult.result);
  REQUIRE(fs::exists(extractedFile));

  // Loading the extracted pipeline resolves filter UUIDs through the plugin registry.
  UnitTest::LoadPlugins();
  auto replayResult = Pipeline::FromFile(extractedFile);
  SIMPLNX_RESULT_REQUIRE_VALID(replayResult);
  auto& replayRef = replayResult.value();
  REQUIRE(replayRef.size() == 2);
  auto* replayReaderPtr = dynamic_cast<PipelineFilter*>(replayRef.at(0));
  auto* replayWriterPtr = dynamic_cast<PipelineFilter*>(replayRef.at(1));
  REQUIRE(replayReaderPtr != nullptr);
  REQUIRE(replayWriterPtr != nullptr);
  REQUIRE(replayReaderPtr->getFilter() != nullptr);
  REQUIRE(replayWriterPtr->getFilter() != nullptr);
  CHECK(replayReaderPtr->getFilter()->uuid() == reader.uuid());
  CHECK(replayWriterPtr->getFilter()->uuid() == writer.uuid());
  CHECK(replayReaderPtr->isDisabled());
  CHECK(replayReaderPtr->getComments() == "Skip the missing reader during replay.");
  CHECK_FALSE(replayWriterPtr->isDisabled());
  CHECK(replayWriterPtr->getComments() == "Keep the writer comment in the embedded pipeline.");
  CHECK(replayRef.toJson() == embeddedRef);

  Arguments replayWriterArgs = replayWriterPtr->getArguments();
  replayWriterArgs.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, replayFile);
  replayWriterPtr->setArguments(replayWriterArgs);
  DataStructure replayDataStructure;
  REQUIRE(replayRef.preflight(replayDataStructure, false));
  REQUIRE(replayRef.execute(replayDataStructure, false));
  CHECK(fs::exists(replayFile));
  CHECK_FALSE(fs::exists(missingInput));
  CHECK(replayDataStructure.getAllDataPaths().empty());
  CHECK(readerPtr->isDisabled());
  CHECK(readerPtr->getComments() == "Skip the missing reader during replay.");
  CHECK(writerPtr->getComments() == "Keep the writer comment in the embedded pipeline.");
  CHECK(writerPtr->getArguments().value<FileSystemPathParameter::ValueType>(WriteDREAM3DFilter::k_ExportFilePath) == writtenFile);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
  UnitTest::CheckArraysInheritTupleDims(replayDataStructure);
}

#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Plugin/PluginLoader.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "ImageProcessing/Filters/ImportFijiMontageFilter.hpp"
#include "ImageProcessing/Filters/ReadImageFilter.hpp"
#include "ImageProcessing/Filters/ReadImageStackFilter.hpp"
#include "ImageProcessing/Filters/ReadMhaFileFilter.hpp"
#include "ImageProcessing/Filters/ThresholdImageFilter.hpp"
#include "ImageProcessing/Filters/WriteImageFilter.hpp"
#include "ImageProcessing/ImageProcessingPlugin.hpp"

#include "simplnx/Plugin/AbstractPlugin.hpp"

#include <nlohmann/json.hpp>

#include <memory>

using namespace nx::core;

namespace
{
nlohmann::json MakeSavedFilterNode(const Uuid& uuid, const nlohmann::json& args)
{
  nlohmann::json node;
  node["filter"] = {{"uuid", uuid.str()}, {"name", "Write Image (ITK)"}};
  node["args"] = args;
  node["comments"] = "";
  return node;
}

std::shared_ptr<FilterList> MakeImageProcessingOnlyFilterList()
{
  auto filterList = std::make_shared<FilterList>();
  const Result<> addResult = filterList->addPlugin(std::make_shared<InMemoryPluginLoader>(std::make_shared<ImageProcessingPlugin>()));
  REQUIRE(addResult.valid());
  return filterList;
}
} // namespace

TEST_CASE("ImageProcessing::Plugin exposes ITK->new Threshold redirect", "[ImageProcessing]")
{
  // This test verifies the replacement map while both image-processing plugins are loaded.
  // The pipeline tests use isolated FilterLists to verify the fallback path.
  UnitTest::LoadPlugins();
  const auto plugins = Application::Instance()->getFilterList()->getLoadedPlugins();
  const AbstractPlugin* plugin = nullptr;
  for(const auto* candidate : plugins)
  {
    if(candidate->getName() == "ImageProcessing")
    {
      plugin = candidate;
      break;
    }
  }
  REQUIRE(plugin != nullptr);

  const auto replacements = plugin->getFilterReplacementMap();
  const Uuid oldItkUuid = *Uuid::FromString("ddf222f3-4af2-4583-967d-3eb9b86e77b4");
  const Uuid newUuid = *Uuid::FromString("9c8f6b2e-3a41-4e7d-b5c9-0f21a7d84e63");

  REQUIRE(replacements.count(oldItkUuid) == 1);
  REQUIRE(replacements.at(oldItkUuid) == newUuid);
  REQUIRE(newUuid == FilterTraits<ThresholdImageFilter>::uuid);
}

TEST_CASE("ImageProcessing::Plugin owns Write Image and exposes the ITK writer fallback", "[ImageProcessing][WriteImageFilter]")
{
  UnitTest::LoadPlugins();

  const auto plugins = Application::Instance()->getFilterList()->getLoadedPlugins();
  const AbstractPlugin* imageProcessingPlugin = nullptr;
  const AbstractPlugin* itkImageProcessingPlugin = nullptr;
  for(const auto* candidate : plugins)
  {
    if(candidate->getName() == "ImageProcessing")
    {
      imageProcessingPlugin = candidate;
    }
    else if(candidate->getName() == "ITKImageProcessing")
    {
      itkImageProcessingPlugin = candidate;
    }
  }
  REQUIRE(imageProcessingPlugin != nullptr);

  const Uuid oldItkUuid = *Uuid::FromString("a181ee3e-1678-4133-b9c5-a9dd7bfec62f");
  const Uuid newUuid = FilterTraits<WriteImageFilter>::uuid;
  if(itkImageProcessingPlugin != nullptr)
  {
    REQUIRE(itkImageProcessingPlugin->createFilter(oldItkUuid) != nullptr);
  }

  REQUIRE(imageProcessingPlugin->createFilter(newUuid) != nullptr);

  const auto replacements = imageProcessingPlugin->getFilterReplacementMap();
  REQUIRE(replacements.count(oldItkUuid) == 1);
  REQUIRE(replacements.at(oldItkUuid) == newUuid);
}

TEST_CASE("ImageProcessing::Pipeline loads the ITK writer as Write Image when ITKImageProcessing is absent", "[ImageProcessing][WriteImageFilter][PipelineUuidRedirect]")
{
  const std::shared_ptr<FilterList> filterList = MakeImageProcessingOnlyFilterList();
  const Uuid oldItkUuid = *Uuid::FromString("a181ee3e-1678-4133-b9c5-a9dd7bfec62f");
  REQUIRE(filterList->createFilter(oldItkUuid) == nullptr);

  nlohmann::json args;
  args[WriteImageFilter::k_Plane_Key.str()] = 2;
  args[WriteImageFilter::k_IndexOffset_Key.str()] = 37;
  const nlohmann::json node = MakeSavedFilterNode(oldItkUuid, args);

  Result<std::unique_ptr<PipelineFilter>> result = PipelineFilter::FromJson(node, *filterList);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.value()->getFilter() != nullptr);
  REQUIRE(result.value()->getFilter()->uuid() == FilterTraits<WriteImageFilter>::uuid);

  const Arguments resolvedArgs = result.value()->getArguments();
  REQUIRE(resolvedArgs.value<ChoicesParameter::ValueType>(WriteImageFilter::k_Plane_Key) == 2);
  REQUIRE(resolvedArgs.value<uint64>(WriteImageFilter::k_IndexOffset_Key) == 37);
}

// Legacy ITK reader UUIDs remain in saved pipelines after ITKImageProcessing is removed.
constexpr StringLiteral k_ITKImageReaderUuid = "d72eaf98-9b1d-44c9-88f2-a5c3cf57b4f2";
constexpr StringLiteral k_ITKMhaFileReaderUuid = "41c33a08-0052-4915-8d53-d503f85f30d9";
constexpr StringLiteral k_ITKImportImageStackUuid = "dcf980b7-ecca-46d1-af31-ac65f6e3b6bb";
constexpr StringLiteral k_ITKImportFijiMontageUuid = "4c48ea16-13ef-4281-89cf-315be5fb857d";

TEST_CASE("ImageProcessing::ITK reader pipeline UUID redirect (ITKImageProcessing absent)", "[ImageProcessing][PipelineUuidRedirect]")
{
  const std::shared_ptr<FilterList> filterList = MakeImageProcessingOnlyFilterList();

  // Sanity: the ITK plugin is absent from this list, so a legacy reader UUID does NOT resolve
  // directly -- the redirect fallback is what must resolve it.
  REQUIRE(filterList->createFilter(*Uuid::FromString(k_ITKImageReaderUuid)) == nullptr);

  SECTION("ITKImageReader -> ReadImage (identical parameter keys)")
  {
    nlohmann::json args;
    args[ReadImageFilter::k_ImageDataArrayPath_Key.str()] = "RedirectedImageData";
    args[ReadImageFilter::k_CellDataName_Key.str()] = "RedirectedCellData";

    const nlohmann::json node = MakeSavedFilterNode(*Uuid::FromString(k_ITKImageReaderUuid), args);
    Result<std::unique_ptr<PipelineFilter>> result = PipelineFilter::FromJson(node, *filterList);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    const std::unique_ptr<PipelineFilter>& pipelineFilter = result.value();
    REQUIRE(pipelineFilter->getFilter() != nullptr);
    REQUIRE(pipelineFilter->getFilter()->uuid() == FilterTraits<ReadImageFilter>::uuid);

    const Arguments resolvedArgs = pipelineFilter->getArguments();
    REQUIRE(resolvedArgs.value<std::string>(ReadImageFilter::k_ImageDataArrayPath_Key) == "RedirectedImageData");
    REQUIRE(resolvedArgs.value<std::string>(ReadImageFilter::k_CellDataName_Key) == "RedirectedCellData");
  }

  SECTION("ITKMhaFileReader -> ReadMhaFile (transform + data keys survive despite renamed C++ vars)")
  {
    nlohmann::json args;
    args[ReadMhaFileFilter::k_ImageDataArrayName_Key.str()] = "MhaRedirectedData";
    // Non-default value on a transform parameter (default is false) proves the transform-key mapping.
    args[ReadMhaFileFilter::k_ApplyImageTransformation_Key.str()] = true;

    const nlohmann::json node = MakeSavedFilterNode(*Uuid::FromString(k_ITKMhaFileReaderUuid), args);
    Result<std::unique_ptr<PipelineFilter>> result = PipelineFilter::FromJson(node, *filterList);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    const std::unique_ptr<PipelineFilter>& pipelineFilter = result.value();
    REQUIRE(pipelineFilter->getFilter() != nullptr);
    REQUIRE(pipelineFilter->getFilter()->uuid() == FilterTraits<ReadMhaFileFilter>::uuid);

    const Arguments resolvedArgs = pipelineFilter->getArguments();
    REQUIRE(resolvedArgs.value<std::string>(ReadMhaFileFilter::k_ImageDataArrayName_Key) == "MhaRedirectedData");
    REQUIRE(resolvedArgs.value<bool>(ReadMhaFileFilter::k_ApplyImageTransformation_Key) == true);
  }

  SECTION("ITKImportImageStack -> ReadImageStack (identical parameter keys)")
  {
    nlohmann::json args;
    args[ReadImageStackFilter::k_ImageDataArrayPath_Key.str()] = "StackRedirectedData";
    args[ReadImageStackFilter::k_CellDataName_Key.str()] = "StackRedirectedCellData";

    const nlohmann::json node = MakeSavedFilterNode(*Uuid::FromString(k_ITKImportImageStackUuid), args);
    Result<std::unique_ptr<PipelineFilter>> result = PipelineFilter::FromJson(node, *filterList);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    const std::unique_ptr<PipelineFilter>& pipelineFilter = result.value();
    REQUIRE(pipelineFilter->getFilter() != nullptr);
    REQUIRE(pipelineFilter->getFilter()->uuid() == FilterTraits<ReadImageStackFilter>::uuid);

    const Arguments resolvedArgs = pipelineFilter->getArguments();
    REQUIRE(resolvedArgs.value<std::string>(ReadImageStackFilter::k_ImageDataArrayPath_Key) == "StackRedirectedData");
    REQUIRE(resolvedArgs.value<std::string>(ReadImageStackFilter::k_CellDataName_Key) == "StackRedirectedCellData");
  }

  SECTION("ITKImportFijiMontage -> ImportFijiMontage (identical parameter keys)")
  {
    nlohmann::json args;
    args[ImportFijiMontageFilter::k_DataGroupName_Key.str()] = "RedirectedMontageGroup";
    args[ImportFijiMontageFilter::k_ImageDataArrayName_Key.str()] = "RedirectedMontageImage";

    const nlohmann::json node = MakeSavedFilterNode(*Uuid::FromString(k_ITKImportFijiMontageUuid), args);
    Result<std::unique_ptr<PipelineFilter>> result = PipelineFilter::FromJson(node, *filterList);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    const std::unique_ptr<PipelineFilter>& pipelineFilter = result.value();
    REQUIRE(pipelineFilter->getFilter() != nullptr);
    REQUIRE(pipelineFilter->getFilter()->uuid() == FilterTraits<ImportFijiMontageFilter>::uuid);

    const Arguments resolvedArgs = pipelineFilter->getArguments();
    REQUIRE(resolvedArgs.value<std::string>(ImportFijiMontageFilter::k_DataGroupName_Key) == "RedirectedMontageGroup");
    REQUIRE(resolvedArgs.value<std::string>(ImportFijiMontageFilter::k_ImageDataArrayName_Key) == "RedirectedMontageImage");
  }
}

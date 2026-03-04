#include "WriteSPParksSitesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteSPParksSites.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteSPParksSitesFilter::name() const
{
  return FilterTraits<WriteSPParksSitesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteSPParksSitesFilter::className() const
{
  return FilterTraits<WriteSPParksSitesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteSPParksSitesFilter::uuid() const
{
  return FilterTraits<WriteSPParksSitesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteSPParksSitesFilter::humanName() const
{
  return "Write SPParks Sites File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteSPParksSitesFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteSPParksSitesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File Path", "The path to the output file", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath, "Input Image Geometry", "The image geometry holding the arrays to write", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteSPParksSitesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteSPParksSitesFilter::clone() const
{
  return std::make_unique<WriteSPParksSitesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteSPParksSitesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  usize fIdsTupCount = dataStructure.getDataAs<AbstractDataArray>(pFeatureIdsArrayPathValue)->getNumberOfTuples();

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteSPParksSitesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  WriteSPParksSitesInputValues inputValues;

  inputValues.OutputFile = atomicFile.tempFilePath();
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath);

  auto result = WriteSPParksSites(dataStructure, messageHandler, shouldCancel, &inputValues)();
  if(result.valid())
  {
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  return result;
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputFileKey = "OutputFile";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";

} // namespace SIMPL
} // namespace

Result<Arguments> WriteSPParksSitesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteSPParksSitesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ImageGeomPath));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

#include "WriteSPParksSitesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteSPParksSites.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
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
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteSPParksSitesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType WriteSPParksSitesFilter::clone() const
{
  return std::make_unique<WriteSPParksSitesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteSPParksSitesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto geometryPath = filterArgs.value<DataPath>(k_ImageGeomPath);
  auto inputArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  if(auto tupleResult = ValidateCellTupleCount(dataStructure, geometryPath, {inputArrayPath}, -77453); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteSPParksSitesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  WriteSPParksSitesInputValues inputValues;

  inputValues.output_file = atomicFile.tempFilePath();
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.image_geom_path = filterArgs.value<DataPath>(k_ImageGeomPath);

  auto result = WriteSPParksSites(dataStructure, messageHandler, shouldCancel, &inputValues)();
  if(result.valid())
  {
    if(shouldCancel)
    {
      return MakeErrorResult(-1, "Filter cancelled");
    }
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
namespace simpl
{
constexpr StringLiteralType k_OutputFileKey = "OutputFile";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";

} // namespace simpl
} // namespace

Result<Arguments> WriteSPParksSitesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteSPParksSitesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_ImageGeomPath));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

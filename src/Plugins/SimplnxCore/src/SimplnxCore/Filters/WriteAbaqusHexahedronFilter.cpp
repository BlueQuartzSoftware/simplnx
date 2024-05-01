#include "WriteAbaqusHexahedronFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteAbaqusHexahedron.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/StringParameter.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteAbaqusHexahedronFilter::name() const
{
  return FilterTraits<WriteAbaqusHexahedronFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteAbaqusHexahedronFilter::className() const
{
  return FilterTraits<WriteAbaqusHexahedronFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteAbaqusHexahedronFilter::uuid() const
{
  return FilterTraits<WriteAbaqusHexahedronFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteAbaqusHexahedronFilter::humanName() const
{
  return "Write Abaqus Hexahedron";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteAbaqusHexahedronFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteAbaqusHexahedronFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Int32Parameter>(k_HourglassStiffness_Key, "Hourglass Stiffness Value", "The value to use for the Hourglass Stiffness", 250));
  params.insert(std::make_unique<StringParameter>(k_JobName_Key, "Job Name", "The name of the job", "SomeString"));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FilePrefix_Key, "Output File Prefix", "The prefix to use for each output file.", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Selected Image Geometry", "The input Image Geometry that will be written.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteAbaqusHexahedronFilter::clone() const
{
  return std::make_unique<WriteAbaqusHexahedronFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteAbaqusHexahedronFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pHourglassStiffnessValue = filterArgs.value<int32>(k_HourglassStiffness_Key);
  auto pJobNameValue = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pHexahedralGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Check Output Path
  if(!fs::exists(pOutputPathValue))
  {
    return MakePreflightErrorResult(-1111, "The supplied directory path doesn't exist");
  }
  else
  {
    if(!fs::is_directory(pOutputPathValue))
    {
      return MakePreflightErrorResult(-1112, "The supplied directory path isn't a directory");
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteAbaqusHexahedronFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  WriteAbaqusHexahedronInputValues inputValues;

  inputValues.HourglassStiffness = filterArgs.value<int32>(k_HourglassStiffness_Key);
  inputValues.JobName = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.FilePrefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);

  return WriteAbaqusHexahedron(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_HourglassStiffnessKey = "HourglassStiffness";
constexpr StringLiteral k_JobNameKey = "JobName";
constexpr StringLiteral k_OutputPathKey = "OutputPath";
constexpr StringLiteral k_FilePrefixKey = "FilePrefix";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteAbaqusHexahedronFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteAbaqusHexahedronFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_HourglassStiffnessKey, k_HourglassStiffness_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_JobNameKey, k_JobName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputPathKey, k_OutputPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_FilePrefixKey, k_FilePrefix_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

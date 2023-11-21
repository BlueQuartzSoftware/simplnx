#include "WriteAvizoUniformCoordinateFilter.hpp"

#include "ComplexCore/Filters/Algorithms/WriteAvizoUniformCoordinate.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include "complex/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteAvizoUniformCoordinateFilter::name() const
{
  return FilterTraits<WriteAvizoUniformCoordinateFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteAvizoUniformCoordinateFilter::className() const
{
  return FilterTraits<WriteAvizoUniformCoordinateFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteAvizoUniformCoordinateFilter::uuid() const
{
  return FilterTraits<WriteAvizoUniformCoordinateFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteAvizoUniformCoordinateFilter::humanName() const
{
  return "Write Avizo Uniform Coordinate";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteAvizoUniformCoordinateFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteAvizoUniformCoordinateFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "Amira Mesh .am file created", fs::path("Data/Output/AvizoUniform.am"),
                                                          FileSystemPathParameter::ExtensionsType{".am"}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteBinaryFile_Key, "Write Binary File", "Whether or not to write the output file as binary", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Image Geometry", "The path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<StringParameter>(k_Units_Key, "Units", "The units of the data", "microns"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteAvizoUniformCoordinateFilter::clone() const
{
  return std::make_unique<WriteAvizoUniformCoordinateFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteAvizoUniformCoordinateFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel) const
{
  auto pOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  auto pWriteBinaryFileValue = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pUnitsValue = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteAvizoUniformCoordinateFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  AvizoWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.WriteBinaryFile = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.GeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.Units = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  return WriteAvizoUniformCoordinate(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputFileKey = "OutputFile";
constexpr StringLiteral k_WriteBinaryFileKey = "WriteBinaryFile";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_UnitsKey = "Units";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteAvizoUniformCoordinateFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteAvizoUniformCoordinateFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_WriteBinaryFileKey, k_WriteBinaryFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_GeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_UnitsKey, k_Units_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex

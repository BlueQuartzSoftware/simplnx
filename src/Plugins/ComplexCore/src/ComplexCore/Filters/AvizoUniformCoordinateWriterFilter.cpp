#include "AvizoUniformCoordinateWriterFilter.hpp"

#include "ComplexCore/Filters/Algorithms/AvizoUniformCoordinateWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AvizoUniformCoordinateWriterFilter::name() const
{
  return FilterTraits<AvizoUniformCoordinateWriterFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AvizoUniformCoordinateWriterFilter::className() const
{
  return FilterTraits<AvizoUniformCoordinateWriterFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AvizoUniformCoordinateWriterFilter::uuid() const
{
  return FilterTraits<AvizoUniformCoordinateWriterFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AvizoUniformCoordinateWriterFilter::humanName() const
{
  return "Avizo Uniform Coordinate Exporter";
}

//------------------------------------------------------------------------------
std::vector<std::string> AvizoUniformCoordinateWriterFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters AvizoUniformCoordinateWriterFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "Amira Mesh .am file created", fs::path("Data/Output/AvizoUniform.am"),
                                                          FileSystemPathParameter::ExtensionsType{".am"}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteBinaryFile_Key, "Write Binary File", "Whether or not to write the output file as binary", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Image Geometry", "The path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<StringParameter>(k_Units_Key, "Units", "The units of the data", "microns"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AvizoUniformCoordinateWriterFilter::clone() const
{
  return std::make_unique<AvizoUniformCoordinateWriterFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AvizoUniformCoordinateWriterFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
Result<> AvizoUniformCoordinateWriterFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  AvizoWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.WriteBinaryFile = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.GeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.Units = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  return AvizoUniformCoordinateWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex

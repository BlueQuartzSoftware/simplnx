#include "AvizoRectilinearCoordinateWriterFilter.hpp"

#include "ComplexCore/Filters/Algorithms/AvizoRectilinearCoordinateWriter.hpp"

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
std::string AvizoRectilinearCoordinateWriterFilter::name() const
{
  return FilterTraits<AvizoRectilinearCoordinateWriterFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AvizoRectilinearCoordinateWriterFilter::className() const
{
  return FilterTraits<AvizoRectilinearCoordinateWriterFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AvizoRectilinearCoordinateWriterFilter::uuid() const
{
  return FilterTraits<AvizoRectilinearCoordinateWriterFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AvizoRectilinearCoordinateWriterFilter::humanName() const
{
  return "Avizo Rectilinear Coordinate Exporter";
}

//------------------------------------------------------------------------------
std::vector<std::string> AvizoRectilinearCoordinateWriterFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters AvizoRectilinearCoordinateWriterFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "Amira Mesh .am file created", fs::path("Data/Output/AvizoRectilinear.am"),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteBinaryFile_Key, "Write Binary File", "Whether or not to write the output file as binary", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Image Geometry", "The path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<StringParameter>(k_Units_Key, "Units", "The units of the data", "microns"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AvizoRectilinearCoordinateWriterFilter::clone() const
{
  return std::make_unique<AvizoRectilinearCoordinateWriterFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AvizoRectilinearCoordinateWriterFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
Result<> AvizoRectilinearCoordinateWriterFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  AvizoWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.WriteBinaryFile = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.GeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.Units = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  return AvizoRectilinearCoordinateWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex

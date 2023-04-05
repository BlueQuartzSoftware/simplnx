#include "AbaqusHexahedronWriterFilter.hpp"

#include "ComplexCore/Filters/Algorithms/AbaqusHexahedronWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AbaqusHexahedronWriterFilter::name() const
{
  return FilterTraits<AbaqusHexahedronWriterFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AbaqusHexahedronWriterFilter::className() const
{
  return FilterTraits<AbaqusHexahedronWriterFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AbaqusHexahedronWriterFilter::uuid() const
{
  return FilterTraits<AbaqusHexahedronWriterFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AbaqusHexahedronWriterFilter::humanName() const
{
  return "Abaqus Hexahedron Exporter";
}

//------------------------------------------------------------------------------
std::vector<std::string> AbaqusHexahedronWriterFilter::defaultTags() const
{
  return {"IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters AbaqusHexahedronWriterFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_HourglassStiffness_Key, "Hourglass Stiffness", "", 0));
  params.insert(std::make_unique<StringParameter>(k_JobName_Key, "Job Name", "", "SomeString"));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<StringParameter>(k_FilePrefix_Key, "Output File Prefix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Selected Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Hexahedral}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AbaqusHexahedronWriterFilter::clone() const
{
  return std::make_unique<AbaqusHexahedronWriterFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AbaqusHexahedronWriterFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto pHourglassStiffnessValue = filterArgs.value<int32>(k_HourglassStiffness_Key);
  auto pJobNameValue = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFilePrefixValue = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pHexahedralGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Check Output Path
  QDir dir;
  if(!dir.mkpath(m_OutputPath))
  {
    QString ss = QObject::tr("Error creating parent path '%1'").arg(m_OutputPath);
    setErrorCondition(-1, ss);
    return {};
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AbaqusHexahedronWriterFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  AbaqusHexahedronWriterInputValues inputValues;

  inputValues.HourglassStiffness = filterArgs.value<int32>(k_HourglassStiffness_Key);
  inputValues.JobName = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.FilePrefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);

  return AbaqusHexahedronWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex

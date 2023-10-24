#include "WriteINLFileFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/WriteINLFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteINLFileFilter::name() const
{
  return FilterTraits<WriteINLFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteINLFileFilter::className() const
{
  return FilterTraits<WriteINLFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteINLFileFilter::uuid() const
{
  return FilterTraits<WriteINLFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteINLFileFilter::humanName() const
{
  return "Write INL File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteINLFileFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteINLFileFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "The output .inl file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile));

  params.insertSeparator(Parameters::Separator{"Required Parent Geometry"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "The selected image geometry", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Cell in Bunge convention (Z-X-Z)", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_MaterialNameArrayPath_Key, "Material Names", "Name of each Ensemble", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumFeaturesArrayPath_Key, "Number of Features", "The number of Features per Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteINLFileFilter::clone() const
{
  return std::make_unique<WriteINLFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteINLFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  auto pMaterialNameArrayPathValue = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  IArray::ArrayType arrayType = dataStructure.getDataAs<IArray>(pMaterialNameArrayPathValue)->getArrayType();
  if(arrayType != IArray::ArrayType::StringArray)
  {
    return MakePreflightErrorResult(-78430, fmt::format("Array [{}] type is incorrect, must be of type StringArray.", pMaterialNameArrayPathValue.toString()));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteINLFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                      const std::atomic_bool& shouldCancel) const
{
  WriteINLFileInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.MaterialNameArrayPath = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);
  inputValues.NumFeaturesArrayPath = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);

  return WriteINLFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex

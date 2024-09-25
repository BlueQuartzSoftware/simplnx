#include "WriteINLFileFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/WriteINLFile.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
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
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "The output .inl file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "The selected image geometry", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Cell Euler Angles", "Three angles defining the orientation of the Cell in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
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
  nx::core::Result<OutputActions> resultOutputActions;
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
  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  WriteINLFileInputValues inputValues;

  inputValues.OutputFile = atomicFile.tempFilePath();
  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.MaterialNameArrayPath = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);
  inputValues.NumFeaturesArrayPath = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);

  auto result = WriteINLFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
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
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_MaterialNameArrayPathKey = "MaterialNameArrayPath";
constexpr StringLiteral k_NumFeaturesArrayPathKey = "NumFeaturesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteINLFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteINLFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellEulerAnglesArrayPathKey, k_CellEulerAnglesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaterialNameArrayPathKey, k_MaterialNameArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NumFeaturesArrayPathKey, k_NumFeaturesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_NumFeaturesArrayPathKey, k_ImageGeomPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

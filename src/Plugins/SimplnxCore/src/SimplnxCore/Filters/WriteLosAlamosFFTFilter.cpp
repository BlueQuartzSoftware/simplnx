#include "WriteLosAlamosFFTFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteLosAlamosFFT.hpp"

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
std::string WriteLosAlamosFFTFilter::name() const
{
  return FilterTraits<WriteLosAlamosFFTFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteLosAlamosFFTFilter::className() const
{
  return FilterTraits<WriteLosAlamosFFTFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteLosAlamosFFTFilter::uuid() const
{
  return FilterTraits<WriteLosAlamosFFTFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteLosAlamosFFTFilter::humanName() const
{
  return "Write Los Alamos FFT File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteLosAlamosFFTFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteLosAlamosFFTFilter::parameters() const
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
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Cell Euler Angles",
                                                          "Data Array containing the three angles defining the orientation for each of the Cell in Bunge convention (Z-X-Z)", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Data Array that specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteLosAlamosFFTFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType WriteLosAlamosFFTFilter::clone() const
{
  return std::make_unique<WriteLosAlamosFFTFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteLosAlamosFFTFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto geometryPath = filterArgs.value<DataPath>(k_ImageGeomPath);
  auto inputArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto cellEulerAnglesPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto cellPhasesPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  if(auto tupleResult = ValidateCellTupleCount(dataStructure, geometryPath, {inputArrayPath, cellEulerAnglesPath, cellPhasesPath}, -73461); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteLosAlamosFFTFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  WriteLosAlamosFFTInputValues inputValues;

  inputValues.output_file = atomicFile.tempFilePath();
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.cell_euler_angles_array_path = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.image_geom_path = filterArgs.value<DataPath>(k_ImageGeomPath);

  auto result = WriteLosAlamosFFT(dataStructure, messageHandler, shouldCancel, &inputValues)();
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
constexpr StringLiteralType k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> WriteLosAlamosFFTFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteLosAlamosFFTFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_ImageGeomPath));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellEulerAnglesArrayPathKey, k_CellEulerAnglesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

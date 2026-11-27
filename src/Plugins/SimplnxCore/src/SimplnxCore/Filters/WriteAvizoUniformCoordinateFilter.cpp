#include "WriteAvizoUniformCoordinateFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteAvizoUniformCoordinate.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
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
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "Amira Mesh .am file created", fs::path("Data/Output/AvizoUniform.am"),
                                                          FileSystemPathParameter::ExtensionsType{".am"}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteBinaryFile_Key, "Write Binary File", "Whether or not to write the output file as binary", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Image Geometry", "The path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<StringParameter>(k_Units_Key, "Units", "The units of the data", "microns"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteAvizoUniformCoordinateFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType WriteAvizoUniformCoordinateFilter::clone() const
{
  return std::make_unique<WriteAvizoUniformCoordinateFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteAvizoUniformCoordinateFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                          const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto geometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  auto inputArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  if(auto tupleResult = ValidateCellTupleCount(dataStructure, geometryPath, {inputArrayPath}, -5835); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteAvizoUniformCoordinateFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  AvizoWriterInputValues inputValues;

  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  inputValues.output_file = atomicFile.tempFilePath();
  inputValues.write_binary_file = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.geometry_path = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.units = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  auto result = WriteAvizoUniformCoordinate(dataStructure, messageHandler, shouldCancel, &inputValues)();
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
constexpr StringLiteralType k_WriteBinaryFileKey = "WriteBinaryFile";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_UnitsKey = "Units";
} // namespace simpl
} // namespace

Result<Arguments> WriteAvizoUniformCoordinateFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteAvizoUniformCoordinateFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_WriteBinaryFileKey, k_WriteBinaryFile_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_GeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_UnitsKey, k_Units_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

#include "WriteAvizoRectilinearCoordinateFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteAvizoRectilinearCoordinate.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteAvizoRectilinearCoordinateFilter::name() const
{
  return FilterTraits<WriteAvizoRectilinearCoordinateFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteAvizoRectilinearCoordinateFilter::className() const
{
  return FilterTraits<WriteAvizoRectilinearCoordinateFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteAvizoRectilinearCoordinateFilter::uuid() const
{
  return FilterTraits<WriteAvizoRectilinearCoordinateFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteAvizoRectilinearCoordinateFilter::humanName() const
{
  return "Write Avizo Rectilinear Coordinate";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteAvizoRectilinearCoordinateFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteAvizoRectilinearCoordinateFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "Amira Mesh .am file created", fs::path("Data/Output/AvizoRectilinear.am"),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteBinaryFile_Key, "Write Binary File", "Whether or not to write the output file as binary", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Image Geometry", "The path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<StringParameter>(k_Units_Key, "Units", "The units of the data", "microns"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteAvizoRectilinearCoordinateFilter::clone() const
{
  return std::make_unique<WriteAvizoRectilinearCoordinateFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteAvizoRectilinearCoordinateFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                              const std::atomic_bool& shouldCancel) const
{
  return {};
}

//------------------------------------------------------------------------------
Result<> WriteAvizoRectilinearCoordinateFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  AvizoWriterInputValues inputValues;

  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  inputValues.OutputFile = atomicFile.tempFilePath();
  inputValues.WriteBinaryFile = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.GeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.Units = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  auto result = WriteAvizoRectilinearCoordinate(dataStructure, messageHandler, shouldCancel, &inputValues)();
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
constexpr StringLiteral k_WriteBinaryFileKey = "WriteBinaryFile";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_UnitsKey = "Units";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteAvizoRectilinearCoordinateFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteAvizoRectilinearCoordinateFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_WriteBinaryFileKey, k_WriteBinaryFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_GeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_UnitsKey, k_Units_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

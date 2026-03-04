#include "WriteLAMMPSFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteLAMMPSFile.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteLAMMPSFileFilter::name() const
{
  return FilterTraits<WriteLAMMPSFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteLAMMPSFileFilter::className() const
{
  return FilterTraits<WriteLAMMPSFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteLAMMPSFileFilter::uuid() const
{
  return FilterTraits<WriteLAMMPSFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteLAMMPSFileFilter::humanName() const
{
  return "Write LAMMPS File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteLAMMPSFileFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteLAMMPSFileFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File Path", "The path to the output file", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_VertexGeomPath, "Input Vertex Geometry", "The vertex geometry holding the arrays to write", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AtomLabelsPath_Key, "Atom Feature Labels", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteLAMMPSFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteLAMMPSFileFilter::clone() const
{
  return std::make_unique<WriteLAMMPSFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteLAMMPSFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pVertexGeomPathValue = filterArgs.value<DataPath>(k_VertexGeomPath);
  auto pAtomLabelsPathValue = filterArgs.value<DataPath>(k_AtomLabelsPath_Key);

  std::vector<usize> vertsCount = dataStructure.getDataAs<VertexGeom>(pVertexGeomPathValue)->getVertices()->getTupleShape();
  std::vector<usize> atomLabelsCount = dataStructure.getDataAs<AbstractDataArray>(pAtomLabelsPathValue)->getTupleShape();

  if(vertsCount != atomLabelsCount)
  {
    return MakePreflightErrorResult(-77460, fmt::format("Tuple Dimensions don't match: Number of Vertices - {} || Number of Atom Labels - {}",
                                                        std::accumulate(vertsCount.begin(), vertsCount.end(), 1ULL, std::multiplies<>()),
                                                        std::accumulate(atomLabelsCount.begin(), atomLabelsCount.end(), 1ULL, std::multiplies<>())));
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteLAMMPSFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  WriteLAMMPSFileInputValues inputValues;

  inputValues.OutputFile = atomicFile.tempFilePath();
  inputValues.VertexGeomPath = filterArgs.value<DataPath>(k_VertexGeomPath);
  inputValues.AtomLabelsPath = filterArgs.value<DataPath>(k_AtomLabelsPath_Key);

  auto result = WriteLAMMPSFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
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
constexpr StringLiteral k_LammpsFileKey = "LammpsFile";
constexpr StringLiteral k_AtomFeatureLabelsPathKey = "AtomFeatureLabelsPath";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteLAMMPSFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteLAMMPSFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_LammpsFileKey, k_OutputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_AtomFeatureLabelsPathKey, k_VertexGeomPath));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AtomFeatureLabelsPathKey, k_AtomLabelsPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

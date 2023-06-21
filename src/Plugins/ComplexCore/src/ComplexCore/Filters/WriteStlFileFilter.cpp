#include "WriteStlFileFilter.hpp"

#include "ComplexCore/Filters/Algorithms/WriteStlFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
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
std::string WriteStlFileFilter::name() const
{
  return FilterTraits<WriteStlFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteStlFileFilter::className() const
{
  return FilterTraits<WriteStlFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteStlFileFilter::uuid() const
{
  return FilterTraits<WriteStlFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteStlFileFilter::humanName() const
{
  return "Export STL Files from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteStlFileFilter::defaultTags() const
{
  return {"IO", "Output", "Write", "Export", "Triangles", "SurfaceMesh"};
}

//------------------------------------------------------------------------------
Parameters WriteStlFileFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_GroupByFeature, "Group by Feature Phases", "Further partition the stl files by feature phases", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputStlDirectory_Key, "Output STL Directory", "Directory to dump the STL file(s) to", fs::path(),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(
      std::make_unique<StringParameter>(k_OutputStlPrefix_Key, "STL File Prefix", "The prefix name of created files (other values will be appended later - including the .stl extension)", "Triangle"));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeomPath_Key, "Selected Triangle Geometry", "The geometry to print", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Face labels", "The triangle feature ids array to order/index files by", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  //  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceNormalsPath_Key, "Face Normals", "The triangle normals array to be printed in the stl file", DataPath{},
  //                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesPath_Key, "Feature Phases", "The feature phases array to further order/index files by", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  // link params
  params.linkParameters(k_GroupByFeature, k_FeaturePhasesPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteStlFileFilter::clone() const
{
  return std::make_unique<WriteStlFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteStlFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto pGroupByPhasesValue = filterArgs.value<bool>(k_GroupByFeature);
  auto pOutputStlDirectoryValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputStlDirectory_Key);
  auto pTriangleGeomPathValue = filterArgs.value<DataPath>(k_TriangleGeomPath_Key);
  auto pFeatureIdsPathValue = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  auto pFeaturePhasesPathValue = filterArgs.value<DataPath>(k_FeaturePhasesPath_Key);
  //  auto pFaceNormalsPathValue = filterArgs.value<DataPath>(k_FaceNormalsPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeomPathValue);
  if(triangleGeom == nullptr)
  {
    return MakePreflightErrorResult(-27870, fmt::format("Triangle Geometry doesn't exist at: {}", pTriangleGeomPathValue.toString()));
  }

  if(triangleGeom->getNumberOfFaces() > std::numeric_limits<int32>::max())
  {
    return MakePreflightErrorResult(
        -27871, fmt::format("The number of triangles is {}, but the STL specification only supports triangle counts up to {}", triangleGeom->getNumberOfFaces(), std::numeric_limits<int32>::max()));
  }

  if(pGroupByPhasesValue)
  {
    if(auto* featurePhases = dataStructure.getDataAs<Int32Array>(pFeaturePhasesPathValue); featurePhases == nullptr)
    {
      return MakePreflightErrorResult(-27872, fmt::format("Feature Phases Array doesn't exist at: {}", pFeaturePhasesPathValue.toString()));
    }
  }

  if(auto* featureIds = dataStructure.getDataAs<Int32Array>(pFeatureIdsPathValue); featureIds == nullptr)
  {
    return MakePreflightErrorResult(-27873, fmt::format("Feature Ids Array doesn't exist at: {}", pFeatureIdsPathValue.toString()));
  }

  //  if(auto* normals = dataStructure.getDataAs<Float32Array>(pFaceNormalsPathValue); normals == nullptr)
  //  {
  //    return MakePreflightErrorResult(-27874, fmt::format("Face Normals Array doesn't exist at: {}", pFaceNormalsPathValue.toString()));
  //  }

  if(!exists(pOutputStlDirectoryValue))
  {
    return MakePreflightErrorResult(-27875, fmt::format("Directory {} doesn't exist.", pOutputStlDirectoryValue.string()));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteStlFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  WriteStlFileInputValues inputValues;

  inputValues.GroupByFeature = filterArgs.value<bool>(k_GroupByFeature);
  inputValues.OutputStlDirectory = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputStlDirectory_Key);
  inputValues.OutputStlPrefix = filterArgs.value<StringParameter::ValueType>(k_OutputStlPrefix_Key);
  inputValues.FeatureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  inputValues.FeaturePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesPath_Key);
  inputValues.TriangleGeomPath = filterArgs.value<DataPath>(k_TriangleGeomPath_Key);
  // inputValues.FaceNormalsPath = filterArgs.value<DataPath>(k_FaceNormalsPath_Key);

  return WriteStlFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex

#include "HierarchicalSmoothFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/HierarchicalSmooth.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string HierarchicalSmoothFilter::name() const
{
  return FilterTraits<HierarchicalSmoothFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string HierarchicalSmoothFilter::className() const
{
  return FilterTraits<HierarchicalSmoothFilter>::className;
}

//------------------------------------------------------------------------------
Uuid HierarchicalSmoothFilter::uuid() const
{
  return FilterTraits<HierarchicalSmoothFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string HierarchicalSmoothFilter::humanName() const
{
  return "Hierarchical Smoothing";
}

//------------------------------------------------------------------------------
std::vector<std::string> HierarchicalSmoothFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Smoothing", "Triangle Geometry"};
}

//------------------------------------------------------------------------------
Parameters HierarchicalSmoothFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Int32Parameter>(k_MaxIterations_Key, "Max Bisection Iterations",
                                                 "Maximum number of bisection iterations for the smoothing parameter optimization. Higher values give more precise results.", 53));
  params.insert(std::make_unique<Float64Parameter>(
      k_ErrorThreshold_Key, "Error Threshold",
      "Displacement rejection threshold as a multiple of the minimum edge length. Nodes displaced beyond this threshold are reset to their original positions.", 2.0));

  params.insertSeparator(Parameters::Separator{"Input Triangle Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryDataPath_Key, "Triangle Geometry",
                                                             "The complete path to the triangle geometry for which to apply hierarchical smoothing", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshNodeTypeArrayPath_Key, "Node Type", "The complete path to the array specifying the type of node in the Geometry", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "The complete path to the array specifying the grain IDs on either side of each face",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType HierarchicalSmoothFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer HierarchicalSmoothFilter::clone() const
{
  return std::make_unique<HierarchicalSmoothFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult HierarchicalSmoothFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pTriangleGeomPath = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  // Inform users that the geometry vertex coordinates are going to be modified in place
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pTriangleGeomPath, {});

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> HierarchicalSmoothFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  HierarchicalSmoothInputValues inputValues;

  inputValues.triangleGeometryDataPath = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);
  inputValues.nodeTypeArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshNodeTypeArrayPath_Key);
  inputValues.faceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.maxIterations = filterArgs.value<int32>(k_MaxIterations_Key);
  inputValues.errorThreshold = filterArgs.value<float64>(k_ErrorThreshold_Key);

  return HierarchicalSmooth(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> HierarchicalSmoothFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = HierarchicalSmoothFilter().getDefaultArguments();

  std::vector<Result<>> results;

  /* This is a NEW filter and not ported so this section does not matter */

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core

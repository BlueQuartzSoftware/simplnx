#include "ExtractFeatureBoundaries2DFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ExtractFeatureBoundaries2D.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/AbstractGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ExtractFeatureBoundaries2DFilter::name() const
{
  return FilterTraits<ExtractFeatureBoundaries2DFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ExtractFeatureBoundaries2DFilter::className() const
{
  return FilterTraits<ExtractFeatureBoundaries2DFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractFeatureBoundaries2DFilter::uuid() const
{
  return FilterTraits<ExtractFeatureBoundaries2DFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractFeatureBoundaries2DFilter::humanName() const
{
  return "Create Feature Boundaries (2D)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractFeatureBoundaries2DFilter::defaultTags() const
{
  return {className(), "Geometry", "Edge", "Boundary", "Feature"};
}

//------------------------------------------------------------------------------
Parameters ExtractFeatureBoundaries2DFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeometryPath_Key, "Input Image Geometry", "The input image geometry to create grain boundaries.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_FeatureIdsArrayPath_Key, "Feature Ids", "The feature ids that will be used to draw the boundary edge geometry.", DataPath{},
      ArraySelectionParameter::AllowedTypes{DataType::int8, DataType::int16, DataType::int32, DataType::int64, DataType::uint8, DataType::uint16, DataType::uint32, DataType::uint64},
      ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Z Value Options"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_ZValueChoice_Key, "Z Value Source", "Select the source for the Z coordinate of the generated vertices",
                                                                    to_underlying(ExtractFeatureBoundaries2DInputValues::ZValueChoiceType::UseMinZValue),
                                                                    ChoicesParameter::Choices{"Use min z value from Image geometry", "Use max z value from Image geometry", "Use Custom z Offset"}));
  params.insert(std::make_unique<Float32Parameter>(k_CustomZValue_Key, "Custom Z Value", "The custom Z offset value for the generated vertices", 0.0f));

  // Link the custom Z value parameter to only show when "Use Custom z Offset" is selected
  params.linkParameters(k_ZValueChoice_Key, k_CustomZValue_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ExtractFeatureBoundaries2DInputValues::ZValueChoiceType::UseCustomZValue)));

  params.insertSeparator(Parameters::Separator{"Edge Extraction Options"});
  params.insert(std::make_unique<BoolParameter>(k_ExtractVirtualSampleEdges_Key, "Extract Edges of Virtual Sample", "Whether to extract the outer edges of the image geometry", true));

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_OutputEdgeGeometryPath_Key, "Output Edge Geometry", "The name of the created Edge Geometry", DataPath({"Feature Boundaries"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ExtractFeatureBoundaries2DFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractFeatureBoundaries2DFilter::clone() const
{
  return std::make_unique<ExtractFeatureBoundaries2DFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractFeatureBoundaries2DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pInputImageGeometryPathValue = filterArgs.value<DataPath>(k_InputImageGeometryPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pOutputEdgeGeometryPathValue = filterArgs.value<DataPath>(k_OutputEdgeGeometryPath_Key);
  auto pZValueChoice = filterArgs.value<ChoicesParameter::ValueType>(k_ZValueChoice_Key);
  auto pCustomZValue = filterArgs.value<float32>(k_CustomZValue_Key);
  auto pExtractVirtualSampleEdges = filterArgs.value<bool>(k_ExtractVirtualSampleEdges_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pInputImageGeometryPathValue);

  SizeVec3 dims = imageGeom.getDimensions();
  if(dims.getZ() != 1)
  {
    return {MakeErrorResult<OutputActions>(-1001, fmt::format("Input image geometry must have a Z dimension of 1. Current Z dimension is {}", dims.getZ()))};
  }

  // Estimate the maximum number of edges:
  // Vertical internal edges: (dimX - 1) * dimY
  // Horizontal internal edges: dimX * (dimY - 1)
  // Outer boundary edges (if enabled): 2 * dimX + 2 * dimY
  usize dimX = dims.getX();
  usize dimY = dims.getY();
  usize maxEdges = (dimX - 1) * dimY + dimX * (dimY - 1);
  if(pExtractVirtualSampleEdges)
  {
    maxEdges += 2 * dimX + 2 * dimY;
  }
  usize maxVertices = maxEdges * 2; // Conservative estimate. It will be reduced by deduplication

  // Create the Edge Geometry action
  auto createEdgeGeomAction =
      std::make_unique<CreateEdgeGeometryAction>(pOutputEdgeGeometryPathValue, maxEdges, maxVertices, AbstractNodeGeometry0D::k_VertexAttributeMatrixName,
                                                 AbstractNodeGeometry1D::k_EdgeAttributeMatrixName, AbstractNodeGeometry0D::k_SharedVertexListName, AbstractNodeGeometry1D::k_SharedEdgeListName);
  resultOutputActions.value().appendAction(std::move(createEdgeGeomAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ExtractFeatureBoundaries2DFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ExtractFeatureBoundaries2DInputValues inputValues;

  inputValues.InputImageGeometryPath = filterArgs.value<DataPath>(k_InputImageGeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.OutputEdgeGeometryPath = filterArgs.value<DataPath>(k_OutputEdgeGeometryPath_Key);
  inputValues.ZValueChoice = static_cast<ExtractFeatureBoundaries2DInputValues::ZValueChoiceType>(filterArgs.value<ChoicesParameter::ValueType>(k_ZValueChoice_Key));
  inputValues.CustomZValue = filterArgs.value<float32>(k_CustomZValue_Key);
  inputValues.ExtractVirtualSampleEdges = filterArgs.value<bool>(k_ExtractVirtualSampleEdges_Key);

  return ExtractFeatureBoundaries2D(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

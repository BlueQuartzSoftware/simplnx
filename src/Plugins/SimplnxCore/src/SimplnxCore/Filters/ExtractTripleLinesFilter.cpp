#include "ExtractTripleLinesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ExtractTripleLines.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/ExecutionContext.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"

#include <fmt/format.h>

#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ExtractTripleLinesFilter::name() const
{
  return FilterTraits<ExtractTripleLinesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ExtractTripleLinesFilter::className() const
{
  return FilterTraits<ExtractTripleLinesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractTripleLinesFilter::uuid() const
{
  return FilterTraits<ExtractTripleLinesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractTripleLinesFilter::humanName() const
{
  return "Extract Triple Lines";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractTripleLinesFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Triple Lines", "Edge Geometry", "Generation"};
}

//------------------------------------------------------------------------------
Parameters ExtractTripleLinesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_IncludeExteriorTripleLines_Key, "Include Exterior Triple Lines",
                                                "If true, the outside of the volume counts as a distinct region, so grain boundaries reaching the free surface register as triple lines", false));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_CopyNodeTypes_Key, "Copy Node Types",
      "Copy the source mesh Node Types onto the created vertices so the Edge Geometry can be used by filters that need a Node Types array, such as Laplacian Smoothing.", true));

  params.insertSeparator(Parameters::Separator{"Required Input Triangle Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryPath_Key, "Triangle Geometry", "The surface mesh from which to extract the triple lines", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "The Array specifying which Features are on either side of each Face in the Triangle Geometry",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NodeTypesArrayPath_Key, "Node Types",
                                                          "The array specifies the type of each node in the source Triangle Geometry and is copied onto the created vertices.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Triple Line Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedTripleLineGeometryPath_Key, "Created Triple Line Geometry", "The name of the created Triple Line Edge Geometry",
                                                             DataPath({"Triple Lines"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexDataGroupName_Key, "Vertex Data", "The name of the Attribute Matrix holding the Vertex Data of the Triple Line Geometry",
                                                          INodeGeometry0D::k_VertexAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_EdgeDataGroupName_Key, "Edge Data", "The name of the Attribute Matrix holding the Edge Data of the Triple Line Geometry",
                                                          INodeGeometry1D::k_EdgeAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumFeaturesArrayName_Key, "Number of Features",
                                                          "The name of the Array holding the number of unique Feature Ids bordering each triple line segment (3 or 4, where 4 means four or more)",
                                                          "NumFeatures"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NodeTypesArrayName_Key, "Node Types", "The name of the created Array holding the Node Type of each triple line vertex", "NodeTypes"));

  params.linkParameters(k_CopyNodeTypes_Key, k_NodeTypesArrayPath_Key, true);
  params.linkParameters(k_CopyNodeTypes_Key, k_NodeTypesArrayName_Key, true);
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ExtractTripleLinesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractTripleLinesFilter::clone() const
{
  return std::make_unique<ExtractTripleLinesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractTripleLinesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pTriangleGeometryPath = filterArgs.value<DataPath>(k_TriangleGeometryPath_Key);
  auto pFaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  const bool copyNodeTypes = filterArgs.value<bool>(k_CopyNodeTypes_Key);
  auto pNodeTypesArrayPath = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pTripleLineGeometryPath = filterArgs.value<DataPath>(k_CreatedTripleLineGeometryPath_Key);
  auto pVertexDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  auto pEdgeDataName = filterArgs.value<std::string>(k_EdgeDataGroupName_Key);
  auto pNumFeaturesName = filterArgs.value<std::string>(k_NumFeaturesArrayName_Key);
  auto pNodeTypesName = filterArgs.value<std::string>(k_NodeTypesArrayName_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryPath);
  const auto& faceLabels = dataStructure.getDataRefAs<IDataArray>(pFaceLabelsArrayPath);

  // The selection parameters guarantee these arrays exist with the right type and component shape,
  // but not that they are sized to this particular geometry. Executing with a mismatch would read
  // past the end of one of them, so check here where the user still gets an actionable message.
  if(faceLabels.getNumberOfTuples() != triangleGeom.getNumberOfFaces())
  {
    return {MakeErrorResult<OutputActions>(-57402, fmt::format("Face Labels array '{}' has {} tuples but Triangle Geometry '{}' has {} faces. They must match.", pFaceLabelsArrayPath.toString(),
                                                               faceLabels.getNumberOfTuples(), pTriangleGeometryPath.toString(), triangleGeom.getNumberOfFaces()))};
  }
  if(copyNodeTypes)
  {
    const auto& nodeTypes = dataStructure.getDataRefAs<IDataArray>(pNodeTypesArrayPath);
    if(nodeTypes.getNumberOfTuples() != triangleGeom.getNumberOfVertices())
    {
      return {MakeErrorResult<OutputActions>(-57403, fmt::format("Node Types array '{}' has {} tuples but Triangle Geometry '{}' has {} vertices. They must match.", pNodeTypesArrayPath.toString(),
                                                                 nodeTypes.getNumberOfTuples(), pTriangleGeometryPath.toString(), triangleGeom.getNumberOfVertices()))};
    }
  }

  const std::string dataStoreFormat = faceLabels.getDataFormat();

  // Edge and vertex counts depend on the mesh scan. Create an empty geometry and resize it during execution.
  {
    auto createGeometryAction =
        std::make_unique<CreateEdgeGeometryAction>(pTripleLineGeometryPath, 0, 0, pVertexDataName, pEdgeDataName, EdgeGeom::k_SharedVertexListName, EdgeGeom::k_SharedEdgeListName);
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }
  {
    auto createNumFeaturesAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int8, std::vector<usize>{0}, std::vector<usize>{1},
                                                                       pTripleLineGeometryPath.createChildPath(pEdgeDataName).createChildPath(pNumFeaturesName), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createNumFeaturesAction));
  }
  if(copyNodeTypes)
  {
    auto createNodeTypesAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int8, std::vector<usize>{0}, std::vector<usize>{1},
                                                                     pTripleLineGeometryPath.createChildPath(pVertexDataName).createChildPath(pNodeTypesName), dataStoreFormat);
    resultOutputActions.value().appendAction(std::move(createNodeTypesAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ExtractTripleLinesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pTripleLineGeometryPath = filterArgs.value<DataPath>(k_CreatedTripleLineGeometryPath_Key);
  auto pVertexDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  auto pEdgeDataName = filterArgs.value<std::string>(k_EdgeDataGroupName_Key);

  ExtractTripleLinesInputValues inputValues;
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriangleGeometryPath_Key);
  inputValues.FaceLabelsPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  inputValues.TripleLineGeometryPath = pTripleLineGeometryPath;
  inputValues.NumFeaturesPath = pTripleLineGeometryPath.createChildPath(pEdgeDataName).createChildPath(filterArgs.value<std::string>(k_NumFeaturesArrayName_Key));
  inputValues.IncludeExteriorLines = filterArgs.value<bool>(k_IncludeExteriorTripleLines_Key);
  if(filterArgs.value<bool>(k_CopyNodeTypes_Key))
  {
    inputValues.SourceNodeTypes = &dataStructure.getDataRefAs<Int8Array>(filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key)).getDataStoreRef();
    inputValues.DestinationNodeTypes =
        &dataStructure.getDataRefAs<Int8Array>(pTripleLineGeometryPath.createChildPath(pVertexDataName).createChildPath(filterArgs.value<std::string>(k_NodeTypesArrayName_Key))).getDataStoreRef();
  }
  return ExtractTripleLines(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

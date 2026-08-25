#include "ExtractTripleLinesFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/Meshing/TripleLineUtilities.hpp"

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

  params.insertSeparator(Parameters::Separator{"Required Input Triangle Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryPath_Key, "Triangle Geometry", "The surface mesh from which to extract the triple lines", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "The Array specifying which Features are on either side of each Face in the Triangle Geometry",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NodeTypesArrayPath_Key, "Node Types", "The Array specifying the type of node in the Triangle Geometry. Copied onto the created vertices",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Triple Line Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedTripleLineGeometryPath_Key, "Created Triple Line Geometry", "The name of the created Triple Line Edge Geometry",
                                                             DataPath({"Triple Lines"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexDataGroupName_Key, "Vertex Data", "The name of the Attribute Matrix holding the Vertex Data of the Triple Line Geometry",
                                                          INodeGeometry0D::k_VertexAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_EdgeDataGroupName_Key, "Edge Data", "The name of the Attribute Matrix holding the Edge Data of the Triple Line Geometry",
                                                          INodeGeometry1D::k_EdgeAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumFeaturesArrayName_Key, "Number of Features",
                                                          "The name of the Array holding the number of unique Feature Ids bordering each triple line segment (3 or 4)", "NumFeatures"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NodeTypesArrayName_Key, "Node Types", "The name of the created Array holding the Node Type of each triple line vertex", "NodeTypes"));

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
  const auto& nodeTypes = dataStructure.getDataRefAs<IDataArray>(pNodeTypesArrayPath);

  // The selection parameters guarantee these arrays exist with the right type and component shape,
  // but not that they are sized to this particular geometry. Executing with a mismatch would read
  // past the end of one of them, so check here where the user still gets an actionable message.
  if(faceLabels.getNumberOfTuples() != triangleGeom.getNumberOfFaces())
  {
    return {MakeErrorResult<OutputActions>(-57200, fmt::format("Face Labels array '{}' has {} tuples but Triangle Geometry '{}' has {} faces. They must match.", pFaceLabelsArrayPath.toString(),
                                                               faceLabels.getNumberOfTuples(), pTriangleGeometryPath.toString(), triangleGeom.getNumberOfFaces()))};
  }
  if(nodeTypes.getNumberOfTuples() != triangleGeom.getNumberOfVertices())
  {
    return {MakeErrorResult<OutputActions>(-57201, fmt::format("Node Types array '{}' has {} tuples but Triangle Geometry '{}' has {} vertices. They must match.", pNodeTypesArrayPath.toString(),
                                                               nodeTypes.getNumberOfTuples(), pTriangleGeometryPath.toString(), triangleGeom.getNumberOfVertices()))};
  }

  const std::string dataStoreFormat = faceLabels.getDataFormat();

  // The edge and vertex counts are not known until the mesh has actually been scanned, so the
  // geometry is created empty and resized during execute, as the surface meshers do.
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

  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(filterArgs.value<DataPath>(k_TriangleGeometryPath_Key));
  const auto& faceLabelsRef = dataStructure.getDataRefAs<Int32Array>(filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key)).getDataStoreRef();
  const auto& sourceNodeTypesRef = dataStructure.getDataRefAs<Int8Array>(filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key)).getDataStoreRef();

  auto& tripleLineGeom = dataStructure.getDataRefAs<EdgeGeom>(pTripleLineGeometryPath);
  auto& numFeaturesRef =
      dataStructure.getDataRefAs<Int8Array>(pTripleLineGeometryPath.createChildPath(pEdgeDataName).createChildPath(filterArgs.value<std::string>(k_NumFeaturesArrayName_Key))).getDataStoreRef();
  auto& tripleLineNodeTypesRef =
      dataStructure.getDataRefAs<Int8Array>(pTripleLineGeometryPath.createChildPath(pVertexDataName).createChildPath(filterArgs.value<std::string>(k_NodeTypesArrayName_Key))).getDataStoreRef();

  MeshingUtilities::TripleLineOptions tripleLineOptions;
  tripleLineOptions.IncludeExteriorLines = filterArgs.value<bool>(k_IncludeExteriorTripleLines_Key);

  return MeshingUtilities::GenerateTripleLines(triangleGeom, faceLabelsRef, sourceNodeTypesRef, tripleLineGeom, numFeaturesRef, tripleLineNodeTypesRef, tripleLineOptions, shouldCancel,
                                               messageHandler);
}
} // namespace nx::core

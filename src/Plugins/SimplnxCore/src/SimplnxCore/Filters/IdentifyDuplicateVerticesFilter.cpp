#include "IdentifyDuplicateVerticesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/IdentifyDuplicateVertices.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string IdentifyDuplicateVerticesFilter::name() const
{
  return FilterTraits<IdentifyDuplicateVerticesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string IdentifyDuplicateVerticesFilter::className() const
{
  return FilterTraits<IdentifyDuplicateVerticesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid IdentifyDuplicateVerticesFilter::uuid() const
{
  return FilterTraits<IdentifyDuplicateVerticesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string IdentifyDuplicateVerticesFilter::humanName() const
{
  return "Identify Duplicate Vertices";
}

//------------------------------------------------------------------------------
std::vector<std::string> IdentifyDuplicateVerticesFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Cleanup", "Label"};
}

//------------------------------------------------------------------------------
Parameters IdentifyDuplicateVerticesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Face Data"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_InputGeomPath_Key, "Input Geometry", "The path to the target geometry, must have a SharedVertexList", DataPath{},
                                                   GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad}));

  params.insertSeparator(Parameters::Separator{"Output Vertex Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_DuplicateMaskPath_Key, "Duplicate Vertices Mask Name", "The location and name of the new duplicate vertices mask array",
                                                         DataPath({"Duplicate Vertices Mask"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType IdentifyDuplicateVerticesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer IdentifyDuplicateVerticesFilter::clone() const
{
  return std::make_unique<IdentifyDuplicateVerticesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult IdentifyDuplicateVerticesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pGeomPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_InputGeomPath_Key);
  const auto* geom = dataStructure.getDataAs<INodeGeometry0D>(pGeomPath);
  if(geom == nullptr || geom->getVertices() == nullptr)
  {
    return MakePreflightErrorResult(-62910, "Input Geometry must contain a SharedVertexList.");
  }

  OutputActions actions;

  actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, std::vector<usize>{geom->getVertices()->getNumberOfTuples()}, std::vector<usize>{1},
                                                           filterArgs.value<ArrayCreationParameter::ValueType>(k_DuplicateMaskPath_Key)));

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> IdentifyDuplicateVerticesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  IdentifyDuplicateVerticesInputValues inputValues;

  inputValues.TargetGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_InputGeomPath_Key);
  inputValues.DuplicatesMaskPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_DuplicateMaskPath_Key);

  return IdentifyDuplicateVertices(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

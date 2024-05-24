#include "RemoveFlaggedTrianglesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RemoveFlaggedTriangles.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string RemoveFlaggedTrianglesFilter::name() const
{
  return FilterTraits<RemoveFlaggedTrianglesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedTrianglesFilter::className() const
{
  return FilterTraits<RemoveFlaggedTrianglesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RemoveFlaggedTrianglesFilter::uuid() const
{
  return FilterTraits<RemoveFlaggedTrianglesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedTrianglesFilter::humanName() const
{
  return "Remove Flagged Triangles";
}

//------------------------------------------------------------------------------
std::vector<std::string> RemoveFlaggedTrianglesFilter::defaultTags() const
{
  return {"Surface Meshing", "Cleanup"};
}

//------------------------------------------------------------------------------
Parameters RemoveFlaggedTrianglesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedTriangleGeometryPath_Key, "Triangle|Quad Geometry", "The Triangle|Quad Geometry that will be processed.", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle, IGeometry::Type::Quad}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "The DataArrayPath to the mask array that marks each face as either true (remove) or false(keep).", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedTriangleGeometryPath_Key, "Created Geometry", "The name of the created Triangle Geometry", DataPath({"ReducedGeometry"})));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RemoveFlaggedTrianglesFilter::clone() const
{
  return std::make_unique<RemoveFlaggedTrianglesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RemoveFlaggedTrianglesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto pInitialGeometryPathValue = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  auto pReducedGeometryPathValue = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* initialGeom = dataStructure.getDataAs<INodeGeometry2D>(pInitialGeometryPathValue);

  if(initialGeom->getGeomType() == IGeometry::Type::Triangle)
  {
    auto createGeometryAction = std::make_unique<CreateGeometry2DAction<TriangleGeom>>(
        pReducedGeometryPathValue, initialGeom->getNumberOfFaces(), initialGeom->getNumberOfVertices(),
        (initialGeom->getVertexAttributeMatrix() == nullptr ? "VertexAM" : initialGeom->getVertexAttributeMatrix()->getName()),
        (initialGeom->getFaceAttributeMatrix() == nullptr ? "FaceAM" : initialGeom->getFaceAttributeMatrix()->getName()), initialGeom->getVertices()->getName(), initialGeom->getFaces()->getName());
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }

  if(initialGeom->getGeomType() == IGeometry::Type::Quad)
  {
    auto createGeometryAction = std::make_unique<CreateGeometry2DAction<QuadGeom>>(
        pReducedGeometryPathValue, initialGeom->getNumberOfFaces(), initialGeom->getNumberOfVertices(),
        (initialGeom->getVertexAttributeMatrix() == nullptr ? "VertexAM" : initialGeom->getVertexAttributeMatrix()->getName()),
        (initialGeom->getFaceAttributeMatrix() == nullptr ? "FaceAM" : initialGeom->getFaceAttributeMatrix()->getName()), initialGeom->getVertices()->getName(), initialGeom->getFaces()->getName());
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RemoveFlaggedTrianglesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  RemoveFlaggedTrianglesInputValues inputValues;

  inputValues.TriangleGeometry = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.ReducedTriangleGeometry = filterArgs.value<DataPath>(k_CreatedTriangleGeometryPath_Key);

  return RemoveFlaggedTriangles(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_TriangleGeometryKey = "TriangleGeometry";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_ReducedTriangleGeometryKey = "ReducedTriangleGeometry";
} // namespace SIMPL
} // namespace

Result<Arguments> RemoveFlaggedTrianglesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RemoveFlaggedTrianglesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_TriangleGeometryKey, k_SelectedTriangleGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_ReducedTriangleGeometryKey, k_CreatedTriangleGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

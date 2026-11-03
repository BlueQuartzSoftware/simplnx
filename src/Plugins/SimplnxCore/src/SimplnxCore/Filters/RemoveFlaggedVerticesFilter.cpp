#include "RemoveFlaggedVerticesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RemoveFlaggedVertices.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <string>

namespace nx::core
{

std::string RemoveFlaggedVerticesFilter::name() const
{
  return FilterTraits<RemoveFlaggedVerticesFilter>::name;
}

std::string RemoveFlaggedVerticesFilter::className() const
{
  return FilterTraits<RemoveFlaggedVerticesFilter>::className;
}

Uuid RemoveFlaggedVerticesFilter::uuid() const
{
  return FilterTraits<RemoveFlaggedVerticesFilter>::uuid;
}

std::string RemoveFlaggedVerticesFilter::humanName() const
{
  return "Remove Flagged Vertices";
}

std::vector<std::string> RemoveFlaggedVerticesFilter::defaultTags() const
{
  return {className(), "Remove", "Memory Management", "Vertex Geometry", "Delete", "Reduce"};
}

Parameters RemoveFlaggedVerticesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedVertexGeometryPath_Key, "Vertex Geometry", "Path to the target Vertex Geometry", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputMaskPath_Key, "Flagged Vertex Array (Mask)",
                                                          "The DataArrayPath to the mask array that marks each face as either true (remove) or false(keep).", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Feature Attribute Matrix Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ClearFeatureAttributeMatrix_Key, "Clear Feature Attribute Matrix",
                                                                 "Recreates the selected Feature Attribute Matrix in the reduced geometry with no arrays. Removing vertices changes which vertices "
                                                                 "belong to each feature, so every feature value computed before this "
                                                                 "filter is invalid. Recompute any feature data needed later in the pipeline.",
                                                                 true));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"Vertex Feature Data"})));
  params.insert(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features",
                                                "Renumber Feature Ids to 0..N and resize the cleared Feature Attribute Matrix to the features that remain after removing vertices", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VertexFeatureIdsPath_Key, "Vertex Feature Ids", "Specifies to which feature each vertex belongs.", DataPath({"Vertex Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Vertex Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedVertexGeometryPath_Key, "Reduced Vertex Geometry", "Created Vertex Geometry DataPath. This will be created during the filter.",
                                                             DataPath()));

  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_FeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_RenumberFeatures_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_VertexFeatureIdsPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RemoveFlaggedVerticesFilter::parametersVersion() const
{
  return 2;
}

IFilter::UniquePointer RemoveFlaggedVerticesFilter::clone() const
{
  return std::make_unique<RemoveFlaggedVerticesFilter>();
}

IFilter::PreflightResult RemoveFlaggedVerticesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto vertexGeomPath = filterArgs.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  auto reducedVertexPath = filterArgs.value<DataPath>(k_CreatedVertexGeometryPath_Key);

  const bool clearFeatureAttributeMatrix = filterArgs.value<BoolParameter::ValueType>(k_ClearFeatureAttributeMatrix_Key);
  const bool renumberFeatures = clearFeatureAttributeMatrix && filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  const auto featureAmPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAttributeMatrixPath_Key);
  const auto featureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_VertexFeatureIdsPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataArrayPaths;

  const auto& inputVertexGeom = dataStructure.getDataRefAs<VertexGeom>(vertexGeomPath);
  auto verticesId = inputVertexGeom.getSharedVertexDataArrayId();

  const std::string vertexAttrMatName = inputVertexGeom.getVertexAttributeMatrixDataPath().getTargetName();

  // Create vertex geometry
  const uint64 numVertices = inputVertexGeom.getNumberOfVertices();
  auto reduced = std::make_unique<CreateVertexGeometryAction>(reducedVertexPath, numVertices, vertexAttrMatName, VertexGeom::k_SharedVertexListName);
  const DataPath reducedVertexDataPath = reduced->getVertexDataPath();
  resultOutputActions.value().appendAction(std::move(reduced));

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(-2071, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

  // This section gets the cell attribute matrix for the input Image Geometry and
  // then creates new arrays from each array that is in that attribute matrix. We
  // also push this attribute matrix into the `ignorePaths` variable since we do
  // not need to manually copy these arrays to the destination image geometry
  {
    // Get the name of the Cell Attribute Matrix, so we can use that in the CreateImageGeometryAction
    const AttributeMatrix* selectedCellDataPtr = inputVertexGeom.getVertexAttributeMatrix();
    if(selectedCellDataPtr == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-5751, fmt::format("'{}' must have cell data attribute matrix", vertexGeomPath.toString()))};
    }
    const std::string cellDataName = selectedCellDataPtr->getName();
    ignorePaths.push_back(vertexGeomPath.createChildPath(cellDataName));
    ignorePaths.push_back(vertexGeomPath.createChildPath(VertexGeom::k_SharedVertexListName));

    // Now loop over each array in the source vertex geometry's vertex attribute matrix and create the corresponding arrays
    // in the destination vertex geometry's vertex attribute matrix
    const DataPath reducedVertGeomAttrMatPath = reducedVertexPath.createChildPath(vertexAttrMatName);
    for(const auto& [identifier, object] : *selectedCellDataPtr)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      const DataType dataType = srcArray.getDataType();
      const ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      const ShapeType tupleShape = srcArray.getIDataStoreRef().getTupleShape();
      const DataPath dataArrayPath = reducedVertGeomAttrMatPath.createChildPath(srcArray.getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, tupleShape, componentShape, dataArrayPath));
    }
  }

  // The cleared Feature AM keeps its source shape until optional renumbering.
  if(clearFeatureAttributeMatrix)
  {
    const auto* sourceFeatureAm = dataStructure.getDataAs<AttributeMatrix>(featureAmPath);
    if(sourceFeatureAm == nullptr || !inputVertexGeom.contains(sourceFeatureAm))
    {
      return MakePreflightErrorResult(
          -67421, fmt::format("The Feature Attribute Matrix '{}' must be a direct child of the selected Vertex Geometry '{}'.", featureAmPath.toString(), vertexGeomPath.toString()));
    }
    if(inputVertexGeom.getVertexAttributeMatrix() == sourceFeatureAm)
    {
      return MakePreflightErrorResult(-67422, fmt::format("The Feature Attribute Matrix '{}' is the Vertex Attribute Matrix of '{}'. Select the Attribute Matrix that holds Feature data, or turn "
                                                          "off 'Clear Feature Attribute Matrix'.",
                                                          featureAmPath.toString(), vertexGeomPath.toString()));
    }
    if(renumberFeatures)
    {
      if(!IsChildOfAttributeMatrix(dataStructure, featureIdsPath, *inputVertexGeom.getVertexAttributeMatrix()))
      {
        return MakePreflightErrorResult(
            -67423,
            fmt::format("**Vertex Feature Ids** DataArray '{}' must be in the Vertex Attribute Matrix '{}' of the selected Vertex Geometry '{}'. Select the DataArray from that Attribute Matrix.",
                        featureIdsPath.toString(), vertexGeomPath.createChildPath(vertexAttrMatName).toString(), vertexGeomPath.toString()));
      }
      const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
      if(featureIds.getNumberOfTuples() != numVertices)
      {
        return MakePreflightErrorResult(
            -67424, fmt::format("**Vertex Feature Ids** DataArray '{}' has {} tuples, but the selected Vertex Geometry '{}' has {} vertices. Select a DataArray with one tuple per vertex.",
                                featureIdsPath.toString(), featureIds.getNumberOfTuples(), vertexGeomPath.toString(), numVertices));
      }
    }
    // Ignore every path to the Feature AM, so a selection through another parent is not copied too.
    const auto featureAmPaths = sourceFeatureAm->getDataPaths();
    ignorePaths.insert(ignorePaths.end(), featureAmPaths.begin(), featureAmPaths.end());
    const DataPath destFeatureAmPath = reducedVertexPath.createChildPath(featureAmPath.getTargetName());
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(destFeatureAmPath, sourceFeatureAm->getShape()));

    std::string clearedList;
    for(const auto& [identifier, object] : *sourceFeatureAm)
    {
      clearedList += "\n" + featureAmPath.createChildPath(object->getName()).toString();
    }
    std::string message = fmt::format("The Feature Attribute Matrix '{}' is recreated in the reduced geometry with no arrays.", featureAmPath.toString());
    if(!clearedList.empty())
    {
      message += fmt::format(" Its values describe the original geometry and are not copied. Recompute any of the following Data Arrays:\n{}", clearedList);
    }
    if(renumberFeatures)
    {
      message += "\n\nFeature Ids are renumbered.\nThe Attribute Matrix is resized to the number of features remaining after removing vertices.";
    }
    preflightUpdatedValues.push_back({"Cleared Feature Data", std::move(message)});
  }
  else
  {
    preflightUpdatedValues.push_back({"Feature Data Not Cleared", "Attribute Matrices other than the Vertex Attribute Matrix are copied to the reduced geometry unchanged. If one of them holds "
                                                                  "Feature data, its values describe the original geometry and are not valid for the reduced geometry."});
  }

  // This section covers copying the other Attribute Matrix objects from the source geometry
  // to the destination geometry
  auto childPaths = GetAllChildDataPaths(dataStructure, vertexGeomPath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      // Specifically check if we are about to copy the original Shared Vertex List
      // because we do not want to do that.
      auto dataObjectId = dataStructure.getId(childPath);
      if(dataObjectId == verticesId.value())
      {
        continue;
      }

      // If we got past that check, then keep going.
      const DataPath copiedChildPath = childPath.rebase(vertexGeomPath, reducedVertexPath).value();
      AppendCopyDataObjectAction(dataStructure, resultOutputActions.value(), childPath, copiedChildPath, clearFeatureAttributeMatrix ? featureAmPath : DataPath{});
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

Result<> RemoveFlaggedVerticesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  RemoveFlaggedVerticesInputValues inputValues;
  inputValues.InputVertexGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedVertexGeometryPath_Key);
  inputValues.MaskPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_InputMaskPath_Key);
  inputValues.OutputVertexGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_CreatedVertexGeometryPath_Key);

  inputValues.ClearFeatureAttributeMatrix = filterArgs.value<BoolParameter::ValueType>(k_ClearFeatureAttributeMatrix_Key);
  inputValues.RenumberFeatures = inputValues.ClearFeatureAttributeMatrix && filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  inputValues.VertexFeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_VertexFeatureIdsPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAttributeMatrixPath_Key);

  return RemoveFlaggedVertices(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_VertexGeometryKey = "VertexGeometry";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_ReducedVertexGeometryKey = "ReducedVertexGeometry";
} // namespace SIMPL
} // namespace

Result<Arguments> RemoveFlaggedVerticesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RemoveFlaggedVerticesFilter().getDefaultArguments();
  // The legacy filter had no Feature parameters and copied every non-Vertex Attribute Matrix unchanged.
  args.insertOrAssign(k_ClearFeatureAttributeMatrix_Key, std::make_any<BoolParameter::ValueType>(false));

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_VertexGeometryKey, k_SelectedVertexGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_InputMaskPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_ReducedVertexGeometryKey, k_CreatedVertexGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

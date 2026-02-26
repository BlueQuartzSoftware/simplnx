#include "RemoveFlaggedVerticesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RemoveFlaggedVertices.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
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
  params.insertSeparator(Parameters::Separator{"Output Vertex Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedVertexGeometryPath_Key, "Reduced Vertex Geometry", "Created Vertex Geometry DataPath. This will be created during the filter.",
                                                             DataPath()));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RemoveFlaggedVerticesFilter::parametersVersion() const
{
  return 1;
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

  nx::core::Result<OutputActions> resultOutputActions;

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
      const std::string dataStoreFormat = srcArray.getDataFormat();
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, tupleShape, componentShape, dataArrayPath, dataStoreFormat));
    }
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
      const std::string copiedChildName = nx::core::StringUtilities::replace(childPath.toString(), vertexGeomPath.getTargetName(), reducedVertexPath.getTargetName());
      const DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
      if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
      {
        std::vector<DataPath> allCreatedPaths = {copiedChildPath};
        auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
        if(pathsToBeCopied.has_value())
        {
          for(const auto& sourcePath : pathsToBeCopied.value())
          {
            const std::string createdPathName = nx::core::StringUtilities::replace(sourcePath.toString(), vertexGeomPath.getTargetName(), reducedVertexPath.getTargetName());
            allCreatedPaths.push_back(DataPath::FromString(createdPathName).value());
          }
        }
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, allCreatedPaths));
      }
      else
      {
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, std::vector<DataPath>{copiedChildPath}));
      }
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

Result<> RemoveFlaggedVerticesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  RemoveFlaggedVerticesInputValues inputValues;
  inputValues.InputVertexGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedVertexGeometryPath_Key);
  inputValues.MaskPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_InputMaskPath_Key);
  inputValues.OutputVertexGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_CreatedVertexGeometryPath_Key);

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

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_VertexGeometryKey, k_SelectedVertexGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_InputMaskPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_ReducedVertexGeometryKey, k_CreatedVertexGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

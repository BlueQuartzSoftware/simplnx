#include "RemoveFlaggedVertices.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <string>

using namespace nx::core;

namespace
{
constexpr int32 k_VertexGeomNotFound = -277;

struct RemoveFlaggedVerticesFunctor
{
  // copy data to masked geometry
  template <class T>
  void operator()(const IDataArray& sourceIDataArray, IDataArray& destIDataArray, const BoolArray& mask, size_t numVerticesToKeep) const
  {
    using DataArrayType = DataArray<T>;
    const auto& sourceDataArray = dynamic_cast<const DataArrayType&>(sourceIDataArray);
    auto& destinationDataArray = dynamic_cast<DataArrayType&>(destIDataArray);
    destinationDataArray.getDataStore()->resizeTuples({numVerticesToKeep});

    const usize numInputTuples = sourceDataArray.getNumberOfTuples();
    const usize nComps = sourceDataArray.getNumberOfComponents();
    usize destTupleIndex = 0;
    for(usize inputIndex = 0; inputIndex < numInputTuples; inputIndex++)
    {
      if(!mask[inputIndex])
      {
        for(usize compIdx = 0; compIdx < nComps; compIdx++)
        {
          const usize sourceIndex = (nComps * inputIndex) + compIdx;
          const usize destinationIndex = (nComps * destTupleIndex) + compIdx;
          destinationDataArray[destinationIndex] = sourceDataArray[sourceIndex];
        }
        destTupleIndex++;
      }
    }
  }
};
} // namespace

namespace nx::core
{

std::string RemoveFlaggedVertices::name() const
{
  return FilterTraits<RemoveFlaggedVertices>::name;
}

std::string RemoveFlaggedVertices::className() const
{
  return FilterTraits<RemoveFlaggedVertices>::className;
}

Uuid RemoveFlaggedVertices::uuid() const
{
  return FilterTraits<RemoveFlaggedVertices>::uuid;
}

std::string RemoveFlaggedVertices::humanName() const
{
  return "Remove Flagged Vertices";
}

std::vector<std::string> RemoveFlaggedVertices::defaultTags() const
{
  return {className(), "Remove", "Memory Management", "Vertex Geometry", "Delete", "Reduce"};
}

Parameters RemoveFlaggedVertices::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_VertexGeomPath_Key, "Vertex Geometry", "Path to the target Vertex Geometry", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskPath_Key, "Flagged Vertex Array", "DataPath to the conditional array that will be used to decide which vertices are removed.",
                                                          DataPath(), ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Newly Reduced Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ReducedVertexGeometryPath_Key, "Reduced Vertex Geometry", "Created Vertex Geometry DataPath. This will be created during the filter.",
                                                             DataPath()));
  return params;
}

IFilter::UniquePointer RemoveFlaggedVertices::clone() const
{
  return std::make_unique<RemoveFlaggedVertices>();
}

IFilter::PreflightResult RemoveFlaggedVertices::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = filterArgs.value<DataPath>(k_VertexGeomPath_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskPath_Key);
  auto reducedVertexPath = filterArgs.value<DataPath>(k_ReducedVertexGeometryPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataArrayPaths;

  const auto* inputVertexGeomPtr = dataStructure.getDataAs<VertexGeom>(vertexGeomPath);
  if(inputVertexGeomPtr == nullptr)
  {
    const std::string errorMsg = fmt::format("Vertex Geometry not found at path: '{}'", vertexGeomPath.toString());
    return {MakeErrorResult<OutputActions>(::k_VertexGeomNotFound, errorMsg)};
  }
  auto verticesId = inputVertexGeomPtr->getSharedVertexDataArrayId();
  const auto* inputSharedVertexPtr = dataStructure.getDataAs<Float32Array>(verticesId);
  if(inputSharedVertexPtr == nullptr)
  {
    const std::string errorMsg = fmt::format("Vertex Geometry does not have a vertex list");
    return {MakeErrorResult<OutputActions>(::k_VertexGeomNotFound, errorMsg)};
  }

  const std::string vertexAttrMatName = inputVertexGeomPtr->getVertexAttributeMatrixDataPath().getTargetName();

  const auto* maskArrayPtr = dataStructure.getDataAs<BoolArray>(maskArrayPath);
  if(maskArrayPtr != nullptr)
  {
    dataArrayPaths.push_back(maskArrayPath);
  }

  // Create vertex geometry
  const uint64 numVertices = inputVertexGeomPtr->getNumberOfVertices();
  auto reduced = std::make_unique<CreateVertexGeometryAction>(reducedVertexPath, numVertices, vertexAttrMatName, CreateVertexGeometryAction::k_SharedVertexListName);
  const DataPath reducedVertexDataPath = reduced->getVertexDataPath();
  resultOutputActions.value().appendAction(std::move(reduced));

  dataStructure.validateNumberOfTuples(dataArrayPaths);
  std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

  // This section gets the cell attribute matrix for the input Image Geometry and
  // then creates new arrays from each array that is in that attribute matrix. We
  // also push this attribute matrix into the `ignorePaths` variable since we do
  // not need to manually copy these arrays to the destination image geometry
  {
    // Get the name of the Cell Attribute Matrix, so we can use that in the CreateImageGeometryAction
    const AttributeMatrix* selectedCellDataPtr = inputVertexGeomPtr->getVertexAttributeMatrix();
    if(selectedCellDataPtr == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-5551, fmt::format("'{}' must have cell data attribute matrix", vertexGeomPath.toString()))};
    }
    const std::string cellDataName = selectedCellDataPtr->getName();
    ignorePaths.push_back(vertexGeomPath.createChildPath(cellDataName));
    ignorePaths.push_back(vertexGeomPath.createChildPath(CreateVertexGeometryAction::k_SharedVertexListName));

    // Now loop over each array in the source vertex geometry's vertex attribute matrix and create the corresponding arrays
    // in the destination vertex geometry's vertex attribute matrix
    const DataPath reducedVertGeomAttrMatPath = reducedVertexPath.createChildPath(vertexAttrMatName);
    for(const auto& [identifier, object] : *selectedCellDataPtr)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      const DataType dataType = srcArray.getDataType();
      const IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      const IDataStore::ShapeType tupleShape = srcArray.getIDataStoreRef().getTupleShape();
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
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

Result<> RemoveFlaggedVertices::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeomPath_Key);
  auto maskArrayPath = args.value<DataPath>(k_MaskPath_Key);
  auto reducedVertexPath = args.value<DataPath>(k_ReducedVertexGeometryPath_Key);

  const VertexGeom& vertexGeom = data.getDataRefAs<VertexGeom>(vertexGeomPath);
  const std::string vertexDataName = vertexGeom.getVertexAttributeMatrixDataPath().getTargetName();

  auto& mask = data.getDataRefAs<BoolArray>(maskArrayPath);

  const size_t numVerticesToKeep = std::count(mask.begin(), mask.end(), false);
  const size_t numberOfVertices = vertexGeom.getNumberOfVertices();

  const std::vector<usize> tDims = {numVerticesToKeep};

  // Resize the reduced vertex geometry object
  VertexGeom& reducedVertexGeom = data.getDataRefAs<VertexGeom>(reducedVertexPath);
  reducedVertexGeom.resizeVertexList(numVerticesToKeep);
  reducedVertexGeom.getVertexAttributeMatrix()->resizeTuples(tDims);

  messageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, fmt::format("Copying vertices to reduced geometry")});

  size_t keepIndex = 0;
  // Loop over each vertex and only copy the vertices that were *NOT* flagged for removal
  for(size_t inputVertexIndex = 0; inputVertexIndex < numberOfVertices; inputVertexIndex++)
  {
    // If the mask value == FALSE we are keeping that vertex.
    if(!mask[inputVertexIndex])
    {
      reducedVertexGeom.setVertexCoordinate(keepIndex, vertexGeom.getVertexCoordinate(inputVertexIndex));
      keepIndex++;
    }
  }
  if(shouldCancel)
  {
    return {};
  }

  // Now copy the vertex data from the source arrays to the reduced vertex attribute matrix arrays
  const AttributeMatrix* sourceVertexAttrMatPtr = vertexGeom.getVertexAttributeMatrix();
  for(const auto& [identifier, object] : *sourceVertexAttrMatPtr)
  {
    const auto& src = dynamic_cast<const IDataArray&>(*object);

    const DataPath destinationPath = reducedVertexGeom.getVertexAttributeMatrixDataPath().createChildPath(src.getName());

    auto& dest = data.getDataRefAs<IDataArray>(destinationPath);
    messageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, fmt::format("Copying source array '{}' to reduced geometry vertex data.", src.getName())});

    ExecuteDataFunction(RemoveFlaggedVerticesFunctor{}, src.getDataType(), src, dest, mask, numVerticesToKeep);
  }

  return {};
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

Result<Arguments> RemoveFlaggedVertices::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RemoveFlaggedVertices().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_VertexGeometryKey, k_VertexGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_ReducedVertexGeometryKey, k_ReducedVertexGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

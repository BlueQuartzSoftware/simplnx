#include "MapPointCloudToRegularGridFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <cmath>

namespace nx::core
{
namespace
{
constexpr int64 k_BadGridDimensions = -2601;
constexpr int64 k_InvalidVertexGeometry = -2602;
constexpr int64 k_IncompatibleMaskVoxelArrays = -2603;
constexpr int64 k_MaskSelectedArrayInvalid = -2604;

Result<> CreateRegularGrid(DataStructure& dataStructure, const Arguments& args)
{
  const auto samplingGridType = args.value<uint64>(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key);

  if(samplingGridType == 1)
  {
    return {};
  }

  const auto vertexGeomPath = args.value<DataPath>(MapPointCloudToRegularGridFilter::k_SelectedVertexGeometryPath_Key);
  const auto newImageGeomPath = args.value<DataPath>(MapPointCloudToRegularGridFilter::k_CreatedImageGeometryPath_Key);
  const auto useMask = args.value<bool>(MapPointCloudToRegularGridFilter::k_UseMask_Key);
  const auto maskArrayPath = args.value<DataPath>(MapPointCloudToRegularGridFilter::k_InputMaskPath_Key);

  const auto* pointCloud = dataStructure.getDataAs<VertexGeom>(vertexGeomPath);
  auto* image = dataStructure.getDataAs<ImageGeom>(newImageGeomPath);

  usize numVerts = pointCloud->getNumberOfVertices();
  auto& vertex = pointCloud->getVertices()->getDataStoreRef();

  const auto* mask = useMask ? dataStructure.getDataAs<BoolArray>(maskArrayPath)->getDataStore() : nullptr;
  if(useMask && mask == nullptr)
  {
    return MakeErrorResult(k_MaskSelectedArrayInvalid, "Use Mask was selected but mask array doesn't exist.");
  }

  // Find the largest/smallest (x,y,z) dimensions of the incoming data to be used to define the maximum dimensions for the regular grid
  std::vector<float32> meshMaxExtents;
  std::vector<float32> meshMinExtents;
  for(size_t i = 0; i < 3; i++)
  {
    meshMaxExtents.push_back(std::numeric_limits<float>::lowest());
    meshMinExtents.push_back(std::numeric_limits<float>::max());
  }

  for(int64_t i = 0; i < numVerts; i++)
  {
    if(!useMask || mask->getValue(i))
    {
      if(vertex[3 * i] > meshMaxExtents[0])
      {
        meshMaxExtents[0] = vertex[3 * i];
      }
      if(vertex[3 * i + 1] > meshMaxExtents[1])
      {
        meshMaxExtents[1] = vertex[3 * i + 1];
      }
      if(vertex[3 * i + 2] > meshMaxExtents[2])
      {
        meshMaxExtents[2] = vertex[3 * i + 2];
      }
      if(vertex[3 * i] < meshMinExtents[0])
      {
        meshMinExtents[0] = vertex[3 * i];
      }
      if(vertex[3 * i + 1] < meshMinExtents[1])
      {
        meshMinExtents[1] = vertex[3 * i + 1];
      }
      if(vertex[3 * i + 2] < meshMinExtents[2])
      {
        meshMinExtents[2] = vertex[3 * i + 2];
      }
    }
  }

  SizeVec3 iDims = image->getDimensions();

  std::vector<float32> iRes(3, 0.0f);
  std::vector<float32> iOrigin(3, 0.0f);

  if(iDims[0] > 1)
  {
    iRes[0] = (meshMaxExtents[0] - meshMinExtents[0]) / (static_cast<float32>(iDims[0]));
    if(iRes[0] == 0.0f)
    {
      iRes[0] = 1.0f;
      iDims[0] = 1;
    }
    else
    {
      iDims[0] += 1;
    }
    iOrigin[0] = meshMinExtents[0] - (iRes[0] / 2.0f);
  }
  else
  {
    iRes[0] = 1.25f * (meshMaxExtents[0] - meshMinExtents[0]) / (static_cast<float>(iDims[0]));
    if(iRes[0] == 0.0f)
    {
      iRes[0] = 1.0f;
      iOrigin[0] = -0.5f;
    }
    else
    {
      iOrigin[0] = meshMinExtents[0] - (iRes[0] * 0.1f);
    }
  }

  if(iDims[1] > 1)
  {
    iRes[1] = (meshMaxExtents[1] - meshMinExtents[1]) / (static_cast<float32>(iDims[1]));
    if(iRes[1] == 0.0f)
    {
      iRes[1] = 1.0f;
      iDims[1] = 1;
    }
    else
    {
      iDims[1] += 1;
    }
    iOrigin[1] = meshMinExtents[1] - (iRes[1] / 2.0f);
  }
  else
  {
    iRes[1] = 1.25f * (meshMaxExtents[1] - meshMinExtents[1]) / (static_cast<float32>(iDims[1]));
    if(iRes[1] == 0.0f)
    {
      iRes[1] = 1.0f;
      iOrigin[1] = -0.5f;
    }
    else
    {
      iOrigin[1] = meshMinExtents[1] - (iRes[1] * 0.1f);
    }
  }

  if(iDims[2] > 1)
  {
    iRes[2] = (meshMaxExtents[2] - meshMinExtents[2]) / (static_cast<float32>(iDims[2]));
    if(iRes[2] == 0.0f)
    {
      iRes[2] = 1.0f;
      iDims[2] = 1;
    }
    else
    {
      iDims[2] += 1;
    }
    iOrigin[2] = meshMinExtents[2] - (iRes[2] / 2.0f);
  }
  else
  {
    iRes[2] = 1.25f * (meshMaxExtents[2] - meshMinExtents[2]) / (static_cast<float32>(iDims[2]));
    if(iRes[2] == 0.0f)
    {
      iRes[2] = 1.0f;
      iOrigin[2] = -0.5f;
    }
    else
    {
      iOrigin[2] = meshMinExtents[1] - (iRes[2] * 0.1f);
    }
  }

  image->setDimensions(iDims);
  image->setSpacing(iRes[0], iRes[1], iRes[2]);
  image->setOrigin(iOrigin[0], iOrigin[1], iOrigin[2]);
  image->getCellData()->resizeTuples({iDims[2], iDims[1], iDims[0]});

  return {};
}
} // namespace

//------------------------------------------------------------------------------
std::string MapPointCloudToRegularGridFilter::name() const
{
  return FilterTraits<MapPointCloudToRegularGridFilter>::name;
}

//------------------------------------------------------------------------------
std::string MapPointCloudToRegularGridFilter::className() const
{
  return FilterTraits<MapPointCloudToRegularGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MapPointCloudToRegularGridFilter::uuid() const
{
  return FilterTraits<MapPointCloudToRegularGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MapPointCloudToRegularGridFilter::humanName() const
{
  return "Map Point Cloud to Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> MapPointCloudToRegularGridFilter::defaultTags() const
{
  return {className(), "Alignment", "Point Cloud", "Grid", "Sampling", "Geometry"};
}

//------------------------------------------------------------------------------
Parameters MapPointCloudToRegularGridFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Sampling Grid Selection"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SamplingGridType_Key, "Sampling Grid Type", "Specifies how data is saved or accessed", 0,
                                                                    std::vector<std::string>{"Manual", "Use Existing Image Geometry"}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_GridDimensions_Key, "Grid Dimensions", "Target grid size", std::vector<int32>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Created Image Geometry", "Path to create the Image Geometry", DataPath()));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Existing Image Geometry", "Path to the existing Image Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Vertex Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedVertexGeometryPath_Key, "Vertex Geometry", "Path to the target Vertex Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputMaskPath_Key, "Mask", "DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_VoxelIndicesName_Key, "Created Voxel Indices", "Path to the created Voxel Indices array", "Voxel Indices"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellDataName_Key, "Created Cell Data Name", "The name of the cell data attribute matrix to be created within the created Image Geometry",
                                                          ImageGeom::k_CellDataName));

  params.linkParameters(k_UseMask_Key, k_InputMaskPath_Key, std::make_any<bool>(true));
  params.linkParameters(k_SamplingGridType_Key, k_GridDimensions_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_SamplingGridType_Key, k_CreatedImageGeometryPath_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_SamplingGridType_Key, k_CellDataName_Key, std::make_any<ChoicesParameter::ValueType>(0));

  params.linkParameters(k_SamplingGridType_Key, k_SelectedImageGeometryPath_Key, std::make_any<ChoicesParameter::ValueType>(1));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MapPointCloudToRegularGridFilter::clone() const
{
  return std::make_unique<MapPointCloudToRegularGridFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MapPointCloudToRegularGridFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel) const
{
  auto samplingGridType = args.value<uint64>(k_SamplingGridType_Key);
  auto vertexGeomPath = args.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  auto useMask = args.value<bool>(k_UseMask_Key);
  auto voxelIndicesName = args.value<std::string>(k_VoxelIndicesName_Key);

  OutputActions actions;

  if(samplingGridType == 0)
  {
    auto newImageGeomPath = args.value<DataPath>(k_CreatedImageGeometryPath_Key);
    auto cellDataName = args.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
    auto gridDimensions = args.value<std::vector<int32>>(k_GridDimensions_Key);

    if(gridDimensions[0] <= 0 || gridDimensions[1] <= 0 || gridDimensions[2] <= 0)
    {
      return MakePreflightErrorResult(k_BadGridDimensions, fmt::format("All grid dimensions must be positive.\n "
                                                                       "Current grid dimensions:\n x = {}\n y = {}\n z = {}\n",
                                                                       gridDimensions[0], gridDimensions[1], gridDimensions[2]));
    }

    CreateImageGeometryAction::DimensionType dims = {static_cast<usize>(gridDimensions[0]), static_cast<usize>(gridDimensions[1]), static_cast<usize>(gridDimensions[2])};
    CreateImageGeometryAction::OriginType origin = {0, 0, 0};
    CreateImageGeometryAction::SpacingType spacing = {1, 1, 1};
    auto imageAction = std::make_unique<CreateImageGeometryAction>(newImageGeomPath, dims, origin, spacing, cellDataName);
    actions.appendAction(std::move(imageAction));
  }

  const auto& vertexGeom = dataStructure.getDataRefAs<VertexGeom>(vertexGeomPath);
  const AttributeMatrix* vertexData = vertexGeom.getVertexAttributeMatrix();
  if(vertexData == nullptr)
  {
    return MakePreflightErrorResult(k_InvalidVertexGeometry, fmt::format("Could not find the vertex data attribute matrix in the given vertex geometry at path '{}'", vertexGeomPath.toString()));
  }
  const DataPath vertexDataPath = vertexGeomPath.createChildPath(vertexData->getName());
  const DataPath voxelIndicesPath = vertexDataPath.createChildPath(voxelIndicesName);

  if(useMask)
  {
    auto maskArrayPath = args.value<DataPath>(k_InputMaskPath_Key);
    const auto numMaskTuples = dataStructure.getDataRefAs<BoolArray>(maskArrayPath).getNumberOfTuples();
    const auto numVoxelTuples = vertexData->getNumTuples();
    if(numMaskTuples != numVoxelTuples)
    {
      return MakePreflightErrorResult(k_IncompatibleMaskVoxelArrays,
                                      fmt::format("The voxel indices array created in attribute matrix '{}' and the mask array at path '{}' have mismatching number of tuples.",
                                                  vertexDataPath.toString(), maskArrayPath.toString()));
    }
  }

  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint64, vertexData->getShape(), std::vector<usize>{1}, voxelIndicesPath);
  actions.appendAction(std::move(createArrayAction));

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> MapPointCloudToRegularGridFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  const auto samplingGridType = args.value<uint64>(k_SamplingGridType_Key);
  const auto vertexGeomPath = args.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  const auto useMask = args.value<bool>(k_UseMask_Key);
  const auto maskArrayPath = args.value<DataPath>(k_InputMaskPath_Key);
  const auto voxelIndicesName = args.value<std::string>(k_VoxelIndicesName_Key);

  const ImageGeom* image = nullptr;
  if(samplingGridType == 0)
  {
    // Create the regular grid
    messageHandler("Creating Regular Grid");
    auto result = CreateRegularGrid(dataStructure, args);
    if(result.invalid())
    {
      return result;
    }
    image = dataStructure.getDataAs<ImageGeom>(args.value<DataPath>(k_CreatedImageGeometryPath_Key));
  }
  else if(samplingGridType == 1)
  {
    image = dataStructure.getDataAs<ImageGeom>(args.value<DataPath>(k_SelectedImageGeometryPath_Key));
  }

  const auto& vertices = dataStructure.getDataRefAs<VertexGeom>(vertexGeomPath);
  const DataPath voxelIndicesPath = vertexGeomPath.createChildPath(vertices.getVertexAttributeMatrix()->getName()).createChildPath(voxelIndicesName);
  auto& voxelIndices = dataStructure.getDataAs<UInt64Array>(voxelIndicesPath)->getDataStoreRef();
  const auto* mask = useMask ? dataStructure.getDataAs<BoolArray>(maskArrayPath)->getDataStore() : nullptr;
  if(useMask && mask == nullptr)
  {
    return MakeErrorResult(k_MaskSelectedArrayInvalid, "Use Mask was selected but mask array doesn't exist.");
  }

  usize numVerts = vertices.getNumberOfVertices();
  SizeVec3 dims = image->getDimensions();
  FloatVec3 res = image->getSpacing();
  FloatVec3 origin = image->getOrigin();
  int64 progIncrement = numVerts / 100;
  int64 prog = 1;
  int64 progressInt = 0;
  int64 counter = 0;

  for(int64 i = 0; i < numVerts; i++)
  {
    if(!useMask || mask->getValue(i))
    {
      auto coords = vertices.getVertexCoordinate(i);
      const auto indexResult = image->getIndex(coords[0], coords[1], coords[2]);
      if(indexResult.has_value())
      {
        voxelIndices[i] = indexResult.value();
      }
      else
      {
        voxelIndices[i] = std::numeric_limits<usize>::max();
      }

      if(counter > prog)
      {
        progressInt = static_cast<int64>((static_cast<float>(counter) / numVerts) * 100.0f);
        std::string ss = fmt::format("Computing Point Cloud Voxel Indices || {}% Completed", progressInt);
        messageHandler(ss);
        prog = prog + progIncrement;
      }
      counter++;
    }
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SamplingGridTypeKey = "SamplingGridType";
constexpr StringLiteral k_GridDimensionsKey = "GridDimensions";
constexpr StringLiteral k_ImageDataContainerPathKey = "ImageDataContainerPath";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_UseMaskKey = "UseMask";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_VoxelIndicesArrayPathKey = "VoxelIndicesArrayPath";
constexpr StringLiteral k_CreatedImageDataContainerNameKey = "CreatedImageDataContainerName";
} // namespace SIMPL
} // namespace

Result<Arguments> MapPointCloudToRegularGridFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MapPointCloudToRegularGridFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_SamplingGridTypeKey, k_SamplingGridType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntVec3FilterParameterConverter>(args, json, SIMPL::k_GridDimensionsKey, k_GridDimensions_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_ImageDataContainerPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, "@SIMPLNX_PARAMETER_KEY@"));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseMaskKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_InputMaskPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_VoxelIndicesArrayPathKey, k_VoxelIndicesName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_CreatedImageDataContainerNameKey, k_CreatedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

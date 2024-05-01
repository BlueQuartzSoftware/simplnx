#include "InterpolatePointCloudToRegularGridFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <cmath>

namespace nx::core
{
namespace
{
struct MapPointCloudDataByKernelFunctor
{
  template <typename T>
  void operator()(IDataArray* source, INeighborList* dynamic, std::vector<float>& kernelVals, const int64 kernel[3], const usize dims[3], usize curX, usize curY, usize curZ, usize vertIdx)
  {
    auto* inputDataArray = dynamic_cast<DataArray<T>*>(source);
    auto& inputData = inputDataArray->getDataStoreRef();
    auto* interpolatedDataPtr = dynamic_cast<NeighborList<T>*>(dynamic);

    usize index = 0;
    int64 startKernel[3] = {0, 0, 0};
    int64 endKernel[3] = {0, 0, 0};
    usize counter = 0;

    kernel[0] > static_cast<int64>(curX) ? startKernel[0] = 0 : startKernel[0] = static_cast<int64>(curX) - kernel[0];
    kernel[1] > static_cast<int64>(curY) ? startKernel[1] = 0 : startKernel[1] = static_cast<int64>(curY) - kernel[1];
    kernel[2] > static_cast<int64>(curZ) ? startKernel[2] = 0 : startKernel[2] = static_cast<int64>(curZ) - kernel[2];

    static_cast<int64>(curX) + kernel[0] >= static_cast<int64>(dims[0]) ? endKernel[0] = static_cast<int64>(dims[0]) - 1 : endKernel[0] = static_cast<int64>(curX) + kernel[0];
    static_cast<int64>(curY) + kernel[1] >= static_cast<int64>(dims[1]) ? endKernel[1] = static_cast<int64>(dims[1]) - 1 : endKernel[1] = static_cast<int64>(curY) + kernel[1];
    endKernel[2] = static_cast<int64>(curZ);

    for(int64 z = startKernel[2]; z <= endKernel[2]; z++)
    {
      for(int64 y = startKernel[1]; y <= endKernel[1]; y++)
      {
        for(int64 x = startKernel[0]; x <= endKernel[0]; x++)
        {
          if(kernelVals[counter] == 0.0f)
          {
            continue;
          }
          index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;
          interpolatedDataPtr->addEntry(index, kernelVals[counter] * inputData[vertIdx]);
          counter++;
        }
      }
    }
  }
};

void determineKernel(uint64 interpolationTechnique, const FloatVec3& sigmas, std::vector<float32>& kernel, const int64 kernelNumVoxels[3])
{
  usize counter = 0;

  for(int64 z = -kernelNumVoxels[2]; z <= kernelNumVoxels[2]; z++)
  {
    for(int64 y = -kernelNumVoxels[1]; y <= kernelNumVoxels[1]; y++)
    {
      for(int64 x = -kernelNumVoxels[0]; x <= kernelNumVoxels[0]; x++)
      {
        if(interpolationTechnique == InterpolatePointCloudToRegularGridFilter::k_Uniform)
        {
          kernel[counter] = 1.0f;
        }
        else if(interpolationTechnique == InterpolatePointCloudToRegularGridFilter::k_Gaussian)
        {
          kernel[counter] = std::exp(-((x * x) / (2 * sigmas[0] * sigmas[0]) + (y * y) / (2 * sigmas[1] * sigmas[1]) + (z * z) / (2 * sigmas[2] * sigmas[2])));
        }
        counter++;
      }
    }
  }
}

void determineKernelDistances(std::vector<float32>& kernelValDistances, int64 kernelNumVoxels[3], FloatVec3 res)
{
  usize counter = 0;

  for(int64 z = -kernelNumVoxels[2]; z <= kernelNumVoxels[2]; z++)
  {
    for(int64 y = -kernelNumVoxels[1]; y <= kernelNumVoxels[1]; y++)
    {
      for(int64 x = -kernelNumVoxels[0]; x <= kernelNumVoxels[0]; x++)
      {
        kernelValDistances[counter] = (x * x * res[0] * res[0]) + (y * y * res[1] * res[1]) + (z * z * res[2] * res[2]);
        kernelValDistances[counter] = std::sqrt(kernelValDistances[counter]);
        counter++;
      }
    }
  }
}

void mapKernelDistances(NeighborList<float32>* kernelDistances, std::vector<float32>& kernelValDistances, std::vector<float32>& kernel, int64 kernelNumVoxels[3], usize dims[3], usize curX, usize curY,
                        usize curZ)
{
  usize index = 0;
  int64 startKernel[3] = {0, 0, 0};
  int64 endKernel[3] = {0, 0, 0};
  usize counter = 0;

  kernelNumVoxels[0] > static_cast<int64>(curX) ? startKernel[0] = 0 : startKernel[0] = static_cast<int64>(curX) - kernelNumVoxels[0];
  kernelNumVoxels[1] > static_cast<int64>(curY) ? startKernel[1] = 0 : startKernel[1] = static_cast<int64>(curY) - kernelNumVoxels[1];
  kernelNumVoxels[2] > static_cast<int64>(curZ) ? startKernel[2] = 0 : startKernel[2] = static_cast<int64>(curZ) - kernelNumVoxels[2];

  static_cast<int64>(curX) + kernelNumVoxels[0] >= static_cast<int64>(dims[0]) ? endKernel[0] = static_cast<int64>(dims[0]) - 1 : endKernel[0] = static_cast<int64>(curX) + kernelNumVoxels[0];
  static_cast<int64>(curY) + kernelNumVoxels[1] >= static_cast<int64>(dims[1]) ? endKernel[1] = static_cast<int64>(dims[1]) - 1 : endKernel[1] = static_cast<int64>(curY) + kernelNumVoxels[1];
  endKernel[2] = static_cast<int64>(curZ);

  for(int64 z = startKernel[2]; z <= endKernel[2]; z++)
  {
    for(int64 y = startKernel[1]; y <= endKernel[1]; y++)
    {
      for(int64 x = startKernel[0]; x <= endKernel[0]; x++)
      {
        if(kernel[counter] == 0.0f)
        {
          continue;
        }
        index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;
        kernelDistances->addEntry(index, kernelValDistances[counter]);
        counter++;
      }
    }
  }
}
} // namespace

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGridFilter::name() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::name;
}

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGridFilter::className() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid InterpolatePointCloudToRegularGridFilter::uuid() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGridFilter::humanName() const
{
  return "Interpolate Point Cloud to Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> InterpolatePointCloudToRegularGridFilter::defaultTags() const
{
  return {className(), "Geometry", "Gaussian", "Kernel", "Interpolation", "Point Cloud", "Vertex Geometry"};
}

//------------------------------------------------------------------------------
Parameters InterpolatePointCloudToRegularGridFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Specifies whether or not to use a mask array", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreKernelDistances_Key, "Store Kernel Distances", "Specifies whether or not to store kernel distances", false));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_InterpolationTechnique_Key, "Interpolation Technique", "Selected Interpolation Technique", 0, std::vector<std::string>{"Uniform", "Gaussian"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_KernelSize_Key, "Kernel Size", "Specifies the kernel size", std::vector<float32>{1.0f, 1.0f, 1.0f}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_GaussianSigmas_Key, "Gaussian Sigmas", "Specifies the Gaussian sigmas", std::vector<float32>{1.0f, 1.0f, 1.0f},
                                                         std::vector<std::string>{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedVertexGeometryPath_Key, "Vertex Geometry to Interpolate", "DataPath to geometry to interpolate", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Interpolated Image Geometry", "DataPath to interpolated geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VoxelIndicesPath_Key, "Voxel Indices", "DataPath to voxel indices", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint64},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputMaskPath_Key, "Mask", "DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InterpolateArrays_Key, "Attribute Arrays to Interpolate", "DataPaths to interpolate", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllNumericTypes(),
                                                               MultiArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyArrays_Key, "Attribute Arrays to Copy", "DataPaths to copy", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllDataTypes(),
                                                               MultiArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_InterpolatedGroupName_Key, "Interpolated Group", "DataPath to created DataGroup for interpolated data", "InterpolatedData"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_KernelDistancesArrayName_Key, "Kernel Distances Group", "DataPath to created DataGroup for kernel distances data", "KernelDistances"));

  params.linkParameters(k_UseMask_Key, k_InputMaskPath_Key, std::make_any<bool>(true));
  params.linkParameters(k_StoreKernelDistances_Key, k_KernelDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_InterpolationTechnique_Key, k_GaussianSigmas_Key, std::make_any<uint64>(k_Gaussian));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InterpolatePointCloudToRegularGridFilter::clone() const
{
  return std::make_unique<InterpolatePointCloudToRegularGridFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InterpolatePointCloudToRegularGridFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel) const
{
  auto useMask = args.value<bool>(k_UseMask_Key);
  auto storeKernelDistances = args.value<bool>(k_StoreKernelDistances_Key);
  auto interpolationTechnique = args.value<uint64>(k_InterpolationTechnique_Key);
  auto vertexGeomPath = args.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  auto imageGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto interpolatedGroupName = args.value<std::string>(k_InterpolatedGroupName_Key);
  auto voxelIndicesPath = args.value<DataPath>(k_VoxelIndicesPath_Key);
  auto interpolatedDataPaths = args.value<std::vector<DataPath>>(k_InterpolateArrays_Key);
  auto copyDataPaths = args.value<std::vector<DataPath>>(k_CopyArrays_Key);
  auto kernelSize = args.value<std::vector<float32>>(k_KernelSize_Key);
  auto sigmas = args.value<std::vector<float32>>(k_GaussianSigmas_Key);

  OutputActions actions;

  if(interpolationTechnique != k_Uniform && interpolationTechnique != k_Gaussian)
  {
    return MakePreflightErrorResult(-11000, fmt::format("Interpolation Technique must be 0 [Uniform] or 1 [Gaussian] "));
  }

  if(kernelSize[0] < 0 || kernelSize[1] < 0 || kernelSize[2] < 0)
  {
    return MakePreflightErrorResult(-11000, fmt::format("All kernel dimensions must be positive.\n "
                                                        "Current kernel dimensions:\n x = %1\n y = %2\n z = %3\n",
                                                        kernelSize[0], kernelSize[1], kernelSize[2]));
  }

  if(sigmas[0] <= 0 || sigmas[1] <= 0 || sigmas[2] <= 0)
  {
    return MakePreflightErrorResult(-11000, fmt::format("All sigmas must be positive.\n "
                                                        "Current sigmas:\n x = %1\n y = %2\n z = %3\n",
                                                        sigmas[0], sigmas[1], sigmas[2]));
  }

  const DataPath interpolatedGroupPath = imageGeomPath.createChildPath(interpolatedGroupName);
  auto vertexGeom = dataStructure.getDataAs<VertexGeom>(vertexGeomPath);
  auto image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  const SizeVec3 imageDims = image->getDimensions();
  std::vector<usize> tupleDims = {imageDims[2], imageDims[1], imageDims[0]};
  const usize numTuples = std::accumulate(tupleDims.cbegin(), tupleDims.cend(), static_cast<usize>(1), std::multiplies<>());

  // Create the attribute matrix for storing the interpolated/copied arrays
  {
    auto createGroupAction = std::make_unique<CreateAttributeMatrixAction>(interpolatedGroupPath, tupleDims);
    actions.appendAction(std::move(createGroupAction));
  }

  std::vector<DataPath> dataArrays = {vertexGeomPath.createChildPath(vertexGeom->getVertices()->getName()), voxelIndicesPath};

  // Create the neighbor list arrays for storing the interpolated array data
  for(const auto& interpolatePath : interpolatedDataPaths)
  {
    dataArrays.push_back(interpolatePath);

    auto targetArray = dataStructure.getDataAs<IDataArray>(interpolatePath);
    auto targetPath = interpolatedGroupPath.createChildPath(targetArray->getName());
    if(targetArray->getNumberOfComponents() != 1)
    {
      return MakePreflightErrorResult(-11002, fmt::format("Attribute Arrays selected for copying must be scalar arrays"));
    }
    auto dataType = targetArray->getDataType();
    if(dataType != DataType::boolean)
    {
      auto neighborPath = interpolatedGroupPath.createChildPath(targetArray->getName());
      auto neighborAction = std::make_unique<CreateNeighborListAction>(dataType, numTuples, neighborPath);
      actions.appendAction(std::move(neighborAction));
    }
  }

  // Create the neighbor list arrays for storing the copied array data
  for(const auto& copyPath : copyDataPaths)
  {
    dataArrays.push_back(copyPath);

    auto targetArray = dataStructure.getDataAs<IDataArray>(copyPath);
    auto targetPath = interpolatedGroupPath.createChildPath(targetArray->getName());
    if(targetArray->getNumberOfComponents() != 1)
    {
      return MakePreflightErrorResult(-11002, fmt::format("Attribute Arrays selected for copying must be scalar arrays"));
    }
    auto dataType = targetArray->getDataType();
    if(dataType != DataType::boolean)
    {
      auto neighborPath = interpolatedGroupPath.createChildPath(targetArray->getName());
      auto neighborAction = std::make_unique<CreateNeighborListAction>(dataType, numTuples, neighborPath);
      actions.appendAction(std::move(neighborAction));
    }
  }

  // validate the input arrays have matching tuples (i.e. it should all come from the input vertex geometry's vertex data)
  if(useMask)
  {
    dataArrays.push_back(args.value<DataPath>(k_InputMaskPath_Key));
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrays);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-11003, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Create the neighbor list array for storing the kernel distances
  if(storeKernelDistances)
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, numTuples, interpolatedGroupPath.createChildPath(args.value<std::string>(k_KernelDistancesArrayName_Key)));
    actions.appendAction(std::move(action));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> InterpolatePointCloudToRegularGridFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto useMask = args.value<bool>(k_UseMask_Key);
  auto storeKernelDistances = args.value<bool>(k_StoreKernelDistances_Key);
  auto interpolationTechnique = args.value<uint64>(k_InterpolationTechnique_Key);
  auto vertexGeomPath = args.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  auto imageGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto interpolatedGroupName = args.value<std::string>(k_InterpolatedGroupName_Key);
  auto interpolatedDataPaths = args.value<std::vector<DataPath>>(k_InterpolateArrays_Key);
  auto copyDataPaths = args.value<std::vector<DataPath>>(k_CopyArrays_Key);
  auto voxelIndicesPath = args.value<DataPath>(k_VoxelIndicesPath_Key);
  auto kernelSize = args.value<std::vector<float32>>(k_KernelSize_Key);

  const DataPath interpolatedGroupPath = imageGeomPath.createChildPath(interpolatedGroupName);
  const auto sigmas = args.value<std::vector<float32>>(k_GaussianSigmas_Key);

  auto vertices = dataStructure.getDataAs<VertexGeom>(vertexGeomPath);
  auto image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  SizeVec3 dims = image->getDimensions();
  FloatVec3 res = image->getSpacing();
  int64 kernelNumVoxels[3] = {0, 0, 0};

  auto numVerts = vertices->getNumberOfVertices();
  usize index = 0;
  usize x = 0;
  usize y = 0;
  usize z = 0;

  std::vector<float32> kernel;

  BoolArray::store_type* mask = nullptr;
  if(useMask)
  {
    mask = dataStructure.getDataAs<BoolArray>(args.value<DataPath>(k_InputMaskPath_Key))->getDataStore();
  }

  auto& voxelIndices = dataStructure.getDataRefAs<UInt64Array>(voxelIndicesPath);

  // Make sure the NeighborList's outer most vector is resized to the number of tuples and initialized to non null values (empty vectors)
  for(const auto& interpolatedDataPath : interpolatedDataPaths)
  {
    InitializeNeighborList(dataStructure, interpolatedGroupPath.createChildPath(interpolatedDataPath.getTargetName()));
  }
  for(const auto& copyDataPath : copyDataPaths)
  {
    InitializeNeighborList(dataStructure, interpolatedGroupPath.createChildPath(copyDataPath.getTargetName()));
  }

  usize maxImageIndex = ((dims[2] - 1) * dims[0] * dims[1]) + ((dims[1] - 1) * dims[0]) + (dims[0] - 1);

  kernelNumVoxels[0] = static_cast<int64>(std::ceil((kernelSize[0] / res[0]) * 0.5f));
  kernelNumVoxels[1] = static_cast<int64>(std::ceil((kernelSize[1] / res[1]) * 0.5f));
  kernelNumVoxels[2] = static_cast<int64>(std::ceil((kernelSize[2] / res[2]) * 0.5f));

  if(kernelSize[0] < res[0])
  {
    kernelNumVoxels[0] = 0;
  }
  if(kernelSize[1] < res[1])
  {
    kernelNumVoxels[1] = 0;
  }
  if(kernelSize[2] < res[2])
  {
    kernelNumVoxels[2] = 0;
  }

  int64 tmpKernelSize[3] = {1, 1, 1};
  int64 totalKernel = 0;

  for(usize i = 0; i < 3; i++)
  {
    tmpKernelSize[i] *= (kernelNumVoxels[i] * 2) + 1;
  }

  totalKernel = tmpKernelSize[0] * tmpKernelSize[1] * tmpKernelSize[2];

  kernel.resize(totalKernel);
  std::fill(kernel.begin(), kernel.end(), 0.0f);
  determineKernel(interpolationTechnique, sigmas, kernel, kernelNumVoxels);

  std::vector<float32> uniformKernel(totalKernel, 1.0f);

  std::vector<float32> kernelValDistances;
  if(storeKernelDistances)
  {
    kernelValDistances.resize(totalKernel);
    std::fill(kernelValDistances.begin(), kernelValDistances.end(), 0.0f);
    determineKernelDistances(kernelValDistances, kernelNumVoxels, res);
  }

  usize progIncrement = numVerts / 100;
  usize prog = 1;
  usize progressInt = 0;

  for(usize i = 0; i < numVerts; i++)
  {
    if(useMask)
    {
      if(!mask->getValue(i))
      {
        continue;
      }
    }
    index = voxelIndices[i];
    if(index > maxImageIndex)
    {
      return MakeErrorResult(-11004,
                             fmt::format("Index present in the selected Voxel Indices array that falls outside the selected Image Geometry for interpolation.\n Index = %1\n Max Image Index = %2\n",
                                         index, maxImageIndex));
    }
    x = index % dims[0];
    y = (index / dims[0]) % dims[1];
    z = index / (dims[0] * dims[1]);

    for(const auto& interpolatedDataPathItem : interpolatedDataPaths)
    {
      const auto dynamicArrayPath = interpolatedGroupPath.createChildPath(interpolatedDataPathItem.getTargetName());
      auto* dynamicArrayToInterpolate = dataStructure.getDataAs<INeighborList>(dynamicArrayPath);
      auto* sourceArray = dataStructure.getDataAs<IDataArray>(interpolatedDataPathItem);

      const auto& type = sourceArray->getDataType();
      if(type == DataType::boolean) // Can't be executed will throw error
      {
        continue;
      }

      // NO BOOL
      ExecuteNeighborFunction(MapPointCloudDataByKernelFunctor{}, type, sourceArray, dynamicArrayToInterpolate, kernel, kernelNumVoxels, dims.data(), x, y, z, i);
    }

    for(const auto& copyDataPath : copyDataPaths)
    {
      auto dynamicArrayPath = interpolatedGroupPath.createChildPath(copyDataPath.getTargetName());
      auto* dynamicArrayToCopy = dataStructure.getDataAs<INeighborList>(dynamicArrayPath);
      auto* sourceArray = dataStructure.getDataAs<IDataArray>(copyDataPath);

      const auto& type = sourceArray->getDataType();
      if(type == DataType::boolean) // Can't be executed will throw error
      {
        continue;
      }

      // NO BOOL
      ExecuteNeighborFunction(MapPointCloudDataByKernelFunctor{}, type, sourceArray, dynamicArrayToCopy, uniformKernel, kernelNumVoxels, dims.data(), x, y, z, i);
    }

    if(storeKernelDistances)
    {
      const DataPath kernelDistPath = interpolatedGroupPath.createChildPath(args.value<std::string>(k_KernelDistancesArrayName_Key));
      InitializeNeighborList(dataStructure, kernelDistPath);
      auto* kernelDistances = dataStructure.getDataAs<Float32NeighborList>(kernelDistPath);
      mapKernelDistances(kernelDistances, kernelValDistances, kernel, kernelNumVoxels, dims.data(), x, y, z);
    }

    if(i > prog)
    {
      progressInt = static_cast<int64>((static_cast<float>(i) / numVerts) * 100.0f);
      messageHandler(fmt::format("Interpolating Point Cloud || {}% Completed", progressInt));
      prog = prog + progIncrement;
    }
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_UseMaskKey = "UseMask";
constexpr StringLiteral k_StoreKernelDistancesKey = "StoreKernelDistances";
constexpr StringLiteral k_InterpolationTechniqueKey = "InterpolationTechnique";
constexpr StringLiteral k_KernelSizeKey = "KernelSize";
constexpr StringLiteral k_SigmasKey = "Sigmas";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_InterpolatedDataContainerNameKey = "InterpolatedDataContainerName";
constexpr StringLiteral k_VoxelIndicesArrayPathKey = "VoxelIndicesArrayPath";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_ArraysToInterpolateKey = "ArraysToInterpolate";
constexpr StringLiteral k_ArraysToCopyKey = "ArraysToCopy";
constexpr StringLiteral k_InterpolatedAttributeMatrixNameKey = "InterpolatedAttributeMatrixName";
constexpr StringLiteral k_KernelDistancesArrayNameKey = "KernelDistancesArrayName";
constexpr StringLiteral k_InterpolatedSuffixKey = "InterpolatedSuffix";
constexpr StringLiteral k_CopySuffixKey = "CopySuffix";
} // namespace SIMPL
} // namespace

Result<Arguments> InterpolatePointCloudToRegularGridFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = InterpolatePointCloudToRegularGridFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseMaskKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_StoreKernelDistancesKey, k_StoreKernelDistances_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_InterpolationTechniqueKey, k_InterpolationTechnique_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_KernelSizeKey, k_KernelSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_SigmasKey, k_GaussianSigmas_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, "@SIMPLNX_PARAMETER_KEY@"));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_InterpolatedDataContainerNameKey, k_InterpolatedGroupName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_VoxelIndicesArrayPathKey, k_VoxelIndicesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_InputMaskPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ArraysToInterpolateKey, k_InterpolateArrays_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ArraysToCopyKey, k_CopyArrays_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_InterpolatedAttributeMatrixNameKey, k_InterpolatedGroupName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_KernelDistancesArrayNameKey, k_KernelDistancesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_InterpolatedSuffixKey, "@SIMPLNX_PARAMETER_KEY@"));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_CopySuffixKey, "@SIMPLNX_PARAMETER_KEY@"));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core

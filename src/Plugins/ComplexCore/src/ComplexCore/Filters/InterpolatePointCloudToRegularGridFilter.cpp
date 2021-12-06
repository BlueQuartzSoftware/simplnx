#include "InterpolatePointCloudToRegularGridFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

namespace complex
{
namespace
{
template <typename T>
void createCompatibleNeighborList(IFilter* filter, const DataPath& path, const std::vector<usize>& cDims, std::vector<IDataArray*>& dynamicArrays)
{
  IDataArray::WeakPointer ptr = filter->getDataContainerArray()->createNonPrereqArrayFromPath<NeighborList<T>>(filter, path, 0, cDims);
  dynamicArrays.push_back(ptr);
}

template <typename T>
void mapPointCloudDataByKernel(IDataArray* source, IDataArray* dynamic, std::vector<float>& kernelVals, int64 kernel[3], usize dims[3], usize curX, usize curY, usize curZ,
                               usize vertIdx)
{
  DataArray<T>* inputDataPtr = dynamic_cast<DataArray<T>*>(source);
  auto inputData = inputDataPtr->getDataStore();
  NeighborList<T>* interpolatedDataPtr = dynamic_cast<NeighborList<T>*>(dynamic);

  usize index = 0;
  int64 startKernel[3] = {0, 0, 0};
  int64 endKernel[3] = {0, 0, 0};
  usize counter = 0;

  kernel[0] > int64(curX) ? startKernel[0] = 0 : startKernel[0] = curX - kernel[0];
  kernel[1] > int64(curY) ? startKernel[1] = 0 : startKernel[1] = curY - kernel[1];
  kernel[2] > int64(curZ) ? startKernel[2] = 0 : startKernel[2] = curZ - kernel[2];

  int64(curX) + kernel[0] >= int64(dims[0]) ? endKernel[0] = dims[0] - 1 : endKernel[0] = curX + kernel[0];
  int64(curY) + kernel[1] >= int64(dims[1]) ? endKernel[1] = dims[1] - 1 : endKernel[1] = curY + kernel[1];
  endKernel[2] = int64(curZ);

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

void determineKernel(int32 interpolationTechnique, const FloatVec3& sigmas, int64 kernelNumVoxels[3])
{
  usize counter = 0;

  for(int64 z = -kernelNumVoxels[2]; z <= kernelNumVoxels[2]; z++)
  {
    for(int64 y = -kernelNumVoxels[1]; y <= kernelNumVoxels[1]; y++)
    {
      for(int64 x = -kernelNumVoxels[0]; x <= kernelNumVoxels[0]; x++)
      {
        if(interpolationTechnique == 0)
        {
          kernel[counter] = 1.0f;
        }
        else if(interpolationTechnique == 1)
        {
          kernel[counter] = expf(-((x * x) / (2 * sigmas[0] * sigmas[0]) + (y * y) / (2 * sigmas[1] * sigmas[1]) + (z * z) / (2 * sigmas[2] * sigmas[2])));
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
        kernelValDistances[counter] = sqrtf(kernelValDistances[counter]);
        counter++;
      }
    }
  }
}

void mapKernelDistances(NeighborList<float32>* kernelDistances, int64 kernel[3], usize dims[3], usize curX, usize curY, usize curZ)
{
  usize index = 0;
  int64 startKernel[3] = {0, 0, 0};
  int64 endKernel[3] = {0, 0, 0};
  usize counter = 0;

  kernel[0] > int64(curX) ? startKernel[0] = 0 : startKernel[0] = curX - kernel[0];
  kernel[1] > int64(curY) ? startKernel[1] = 0 : startKernel[1] = curY - kernel[1];
  kernel[2] > int64(curZ) ? startKernel[2] = 0 : startKernel[2] = curZ - kernel[2];

  int64(curX) + kernel[0] >= int64(dims[0]) ? endKernel[0] = dims[0] - 1 : endKernel[0] = curX + kernel[0];
  int64(curY) + kernel[1] >= int64(dims[1]) ? endKernel[1] = dims[1] - 1 : endKernel[1] = curY + kernel[1];
  endKernel[2] = int64(curZ);

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

std::string InterpolatePointCloudToRegularGridFilter::name() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::name;
}

std::string InterpolatePointCloudToRegularGridFilter::className() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::className;
}

Uuid InterpolatePointCloudToRegularGridFilter::uuid() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::uuid;
}

std::string InterpolatePointCloudToRegularGridFilter::humanName() const
{
  return "Interpolate Point Cloud to Regular Grid";
}

Parameters InterpolatePointCloudToRegularGridFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<NumericTypeParameter>(k_NumericType_Key, "Numeric Type", "Numeric Type of data to create", NumericType::int32));
  params.insert(std::make_unique<UInt64Parameter>(k_NumComps_Key, "Number of Components", "Number of components", 1));
  params.insert(std::make_unique<UInt64Parameter>(k_NumTuples_Key, "Number of Tuples", "Number of tuples", 0));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataPath_Key, "Created Array", "Array storing the file data", DataPath{}));
  return params;
}

IFilter::UniquePointer InterpolatePointCloudToRegularGridFilter::clone() const
{
  return std::make_unique<InterpolatePointCloudToRegularGridFilter>();
}

IFilter::PreflightResult InterpolatePointCloudToRegularGridFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto vertexGeomPath = args.value<DataPath>(k_InterpolationTechnique_Key);
  auto interpolatedDataPath = args.value<DataPath>(k_InterpolationTechnique_Key);

  auto interpolationTechnique = args.value<int32>(k_InterpolationTechnique_Key);
  auto kernelSize = args.value<FloatVec3>(k_InterpolationTechnique_Key);
  auto sigmas = args.value<FloatVec3>(k_InterpolationTechnique_Key);
  //
  std::vector<const IDataArray*> dataArrays;
  OutputActions actions;

  auto vertexGeom = data.getDataAs<VertexGeom>(vertexGeomPath);
  auto image = data.getDataAs<ImageGeom>(interpolatedDataPath);

  if(vertexGeom == nullptr)
  {
    std::string ss = fmt::format("Vertex Geometry cannot be found");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertexGeom, ss}})};
  }
  if(image == nullptr)
  {
    std::string ss = fmt::format("Image Geometry cannot be found");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingImageGeom, ss}})};
  }

  dataArrays.push_back(vertexGeom->getVertices());

  if(interpolationTechnique < 0 || interpolationTechnique > 1)
  {
    std::string ss = fmt::format("Invalid selection for interpolation technique");
    return {nonstd::make_unexpected(std::vector<Error>{Error{-11000, ss}})};
  }

  if(kernelSize[0] < 0 || kernelSize[1] < 0 || kernelSize[2] < 0)
  {
    std::string ss = fmt::format("All kernel dimensions must be positive.\n "
                                 "Current kernel dimensions:\n x = %1\n y = %2\n z = %3\n",
                                 kernelSize[0], kernelSize[1], kernelSize[2]);
    return {nonstd::make_unexpected(std::vector<Error>{Error{-11000, ss}})};
  }

  if(sigmas[0] <= 0 || sigmas[1] <= 0 || sigmas[2] <= 0)
  {
    std::string ss = fmt::format("All sigmas must be positive.\n "
                                 "Current sigmas:\n x = %1\n y = %2\n z = %3\n",
                                 kernelSize[0], kernelSize[1], kernelSize[2]);
    return {nonstd::make_unexpected(std::vector<Error>{Error{-11000, ss}})};
  }

  // Get the list of all attribute matrices in the input data container and add them to the regular grid data container
  auto m = data.getDataAs<DataGroup>(dataGroupPath);
  auto interpolatedDC = data.getDataAs<DataGroup>(interpolatedDataPath);
  m_AttrMatList = m->getAttributeMatrixNames();
  SizeVec3 dims = image->getDimensions();
  std::vector<usize> tDims = {dims[0], dims[1], dims[2]};
  std::list<std::string> tempDataArrayList;
  DataPath tempPath;
  std::string tempAttrMatName;

  // All vertex data in the original point cloud will be interpolated to the regular grid's cells
  // Feature/Ensemble attribute matrices will remain unchanged and are deep copied to the new data container below
  // Create the attribute matrix where all the interpolated data will be stored
  
  //auto action = std::make_unique<CreateArrayAction>(NumericType::float32, path, std::vector<usize>{0}, cDims, dataArrayPath);
  interpolatedDC->createNonPrereqAttributeMatrix(this, getInterpolatedAttributeMatrixName(), tDims, AttributeMatrix::Type::Cell);

  std::vector<DataPath> interpolatePaths = getArraysToInterpolate();
  std::vector<DataPath> copyPaths = getArraysToCopy();

  #if 0
  if(!DataPath::ValidateVector(interpolatePaths) || !DataPath::ValidateVector(copyPaths))
  {
    std::string ss = fmt::format("There are Attribute Arrays selected that are not contained in the same Attribute Matrix. All selected Attribute Arrays must belong to the same Attribute Matrix");
    setErrorCondition(-11002, ss);
  }

  DataPath path(getDataContainerName().getDataContainerName(), "", "");

  if(!interpolatePaths.empty())
  {
    if(!interpolatePaths[0].hasSameDataContainer(path))
    {
      std::string ss = fmt::format("Attribute Arrays selected for interpolating must belong to the same Data Container selected for interpolation");
      setErrorCondition(-11002, ss);
    }
  }

  if(!copyPaths.empty())
  {
    if(!copyPaths[0].hasSameDataContainer(path))
    {
      std::string ss = fmt::format("Attribute Arrays selected for copying must belong to the same Data Container selected for interpolation");
      setErrorCondition(-11002, ss);
    }
  }

  if(!interpolatePaths.empty() && !copyPaths.empty())
  {
    if(!interpolatePaths[0].hasSameAttributeMatrixPath(copyPaths[0]))
    {
      std::string ss = fmt::format("Attribute Arrays selected for interpolation and copying must belong to the same Data Container and Attribute Matrix");
      setErrorCondition(-11002, ss);
    }
  }
  #endif

  std::string attrMatName;

  if(attrMatName.empty() && !interpolatePaths.empty())
  {
    attrMatName = interpolatePaths[0].getAttributeMatrixName();
  }

  if(attrMatName.empty() && !copyPaths.empty())
  {
    attrMatName = copyPaths[0].getAttributeMatrixName();
  }

  // Loop through all the attribute matrices in the original data container
  // If we are in a vertex attribute matrix, create data arrays for all in the new interpolated data attribute matrix
  // Else, we are in a feature/ensemble attribute matrix, and just deep copy it into the new data container

  std::vector<usize> cDims(1, 1);

  if(!getDataContainerName().empty())
  {
    for(auto it = m_AttrMatList.begin(); it != m_AttrMatList.end(); ++it)
    {
      AttributeMatrix* tmpAttrMat = m->getPrereqAttributeMatrix(this, *it, -301);
      if(getErrorCode() >= 0)
      {
        tempAttrMatName = tmpAttrMat->getName();
        if(tempAttrMatName != attrMatName)
        {
          AttributeMatrix* attrMat = tmpAttrMat->deepCopy();
          interpolatedDC->addOrReplaceAttributeMatrix(attrMat);
        }
        else
        {
          for(int32 i = 0; i < interpolatePaths.size(); i++)
          {
            tempPath.update(getInterpolatedDataContainerName().getDataContainerName(), getInterpolatedAttributeMatrixName(), interpolatePaths[i].getDataArrayName() + "Interpolation");
            IDataArray::Pointer tmpDataArray = tmpAttrMat->getPrereqIDataArray(this, interpolatePaths[i].getDataArrayName(), -90002);
            if(getErrorCode() >= 0)
            {
              sourceArraysToInterpolate.push_back(tmpDataArray);
              dataArrays.push_back(tmpDataArray);
              std::vector<usize> tmpDims = tmpDataArray->getComponentDimensions();
              if(tmpDims != cDims)
              {
                std::string ss = fmt::format("Attribute Arrays selected for interpolation must be scalar arrays");
                return {nonstd::make_unexpected(std::vector<Error>{Error{-11002, ss}})};
              }
              createCompatibleNeighborList(tmpDataArray, this, tempPath, cDims, m_DynamicArraysToInterpolate);
            }
          }

          for(int32 i = 0; i < copyPaths.size(); i++)
          {
            tempPath.update(getInterpolatedDataContainerName().getDataContainerName(), getInterpolatedAttributeMatrixName(), copyPaths[i].getDataArrayName() + "Copy");
            IDataArray::Pointer tmpDataArray = tmpAttrMat->getPrereqIDataArray(this, copyPaths[i].getDataArrayName(), -90002);
            if(getErrorCode() >= 0)
            {
              sourceArraysToCopy.push_back(tmpDataArray);
              dataArrays.push_back(tmpDataArray);
              std::vector<usize> tmpDims = tmpDataArray->getComponentDimensions();
              if(tmpDims != cDims)
              {
                std::string ss = fmt::format("Attribute Arrays selected for copying must be scalar arrays");
                return {nonstd::make_unexpected(std::vector<Error>{Error{-11002, ss}})};
              }
              createCompatibleNeighborList(tmpDataArray, this, tempPath, cDims, m_DynamicArraysToCopy);
            }
          }
        }
      }
    }
  }

  auto voxelIndicesPtr = data.getDataAs<USizeArray>(voxelIndicesArrayPath);
  if(nullptr != voxelIndicesPtr)
  {
    voxelIndices = voxelIndicesPtr->getDataStore();
    /* Now assign the raw pointer to data from the DataArray<T> object */
    dataArrays.push_back(voxelIndicesPtr);
  }

  if(useMask)
  {
    auto maskPtr = data.getDataAs<BoolArray>(maskArrayPath);
    if(nullptr != maskPtr)
    {
      mask = maskPtr->getDataStore();
      /* Now assign the raw pointer to data from the DataArray<T> object */
      dataArrays.push_back(maskPtr);
    }
  }

  path.update(getInterpolatedDataContainerName().getDataContainerName(), getInterpolatedAttributeMatrixName(), getKernelDistancesArrayName());

  if(storeKernelDistances)
  {
    auto action = std::make_unique<CreateArrayAction>(NumericType::float32, path, std::vector<usize>{0}, cDims, dataArrayPath);
    // kernelDistances = getDataContainerArray()->createNonPrereqArrayFromPath<NeighborList<float>>(this, path, 0, cDims);
    actions.actions.push_back(std::move(action));
  }

  data.validateNumberOfTuples(dataArrays);

  return {std::move(actions)};
}

Result<> InterpolatePointCloudToRegularGridFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_NumericType_Key);
  auto components = args.value<uint64>(k_NumComps_Key);
  auto numTuples = args.value<uint64>(k_NumTuples_Key);
  auto path = args.value<DataPath>(k_DataPath_Key);

  //
  auto vertices = data.getDataAs<VertexGeom>(vertexGeomPath);
  auto image = data.getDataAs<ImageGeom>(interpolatedImagePath); // Interpolated group
  SizeVec3 dims = image->getDimensions();
  FloatVec3 res = image->getSpacing();
  int64 kernelNumVoxels[3] = {0, 0, 0};

  auto numVerts = vertices->getNumberOfVertices();
  usize index = 0;
  usize x = 0;
  usize y = 0;
  usize z = 0;

  usize maxImageIndex = ((dims[2] - 1) * dims[0] * dims[1]) + ((dims[1] - 1) * dims[0]) + (dims[0] - 1);

  kernelNumVoxels[0] = int64(ceil((kernelSize[0] / res[0]) * 0.5f));
  kernelNumVoxels[1] = int64(ceil((kernelSize[1] / res[1]) * 0.5f));
  kernelNumVoxels[2] = int64(ceil((kernelSize[2] / res[2]) * 0.5f));

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

  int64 kernelSize[3] = {1, 1, 1};
  int64 totalKernel = 0;

  for(usize i = 0; i < 3; i++)
  {
    kernelSize[i] *= (kernelNumVoxels[i] * 2) + 1;
  }

  totalKernel = kernelSize[0] * kernelSize[1] * kernelSize[2];

  kernel.resize(totalKernel);
  std::fill(kernel.begin(), kernel.end(), 0.0f);
  determineKernel(kernelNumVoxels);

  std::vector<float> uniformKernel(totalKernel, 1.0f);

  if(storeKernelDistances)
  {
    kernelValDistances.resize(totalKernel);
    std::fill(kernelValDistances.begin(), kernelValDistances.end(), 0.0f);
    determineKernelDistances(kernelNumVoxels, res);
  }

  usize progIncrement = numVerts / 100;
  usize prog = 1;
  usize progressInt = 0;

  for(usize i = 0; i < numVerts; i++)
  {
    if(getCancel())
    {
      break;
    }
    if(useMask)
    {
      if(!mask[i])
      {
        continue;
      }
    }
    index = voxelIndices[i];
    if(index > maxImageIndex)
    {
      std::string ss = fmt::format("Index present in the selected Voxel Indices array that falls outside the selected Image Geometry for interpolation.\n Index = %1\n Max Image Index = %2\n", index,
                                   maxImageIndex);
      setErrorCondition(-1, ss);
    }
    x = index % dims[0];
    y = (index / dims[0]) % dims[1];
    z = index / (dims[0] * dims[1]);

    if(!sourceArraysToInterpolate.empty())
    {
      for(usize j = 0; sourceArraysToInterpolate.size(); j++)
      {
        mapPointCloudDataByKernel(sourceArraysToInterpolate[j].lock(), sourceArraysToInterpolate[j].lock(), dynamicArraysToInterpolate[j].lock(), kernel, kernelNumVoxels, dims.data(), x, y, z, i);
      }
    }

    if(sourceArraysToCopy.size() > 0)
    {
      for(usize j = 0; j < sourceArraysToCopy.size(); j++)
      {
        mapPointCloudDataByKernel(sourceArraysToCopy[j].lock(), sourceArraysToCopy[j].lock(), dynamicArraysToCopy[j].lock(), uniformKernel, kernelNumVoxels, dims.data(), x, y, z, i);
      }
    }

    if(storeKernelDistances)
    {
      mapKernelDistances(kernelNumVoxels, dims.data(), x, y, z);
    }

    if(i > prog)
    {
      progressInt = static_cast<int64>((static_cast<float>(i) / numVerts) * 100.0f);
      std::string ss = fmt::format("Interpolating Point Cloud || {}% Completed", progressInt);
      notifyStatusMessage(ss);
      prog = prog + progIncrement;
    }
  }

  return {};
}
} // namespace complex

#include "ErodeDilateCoordinationNumber.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;
namespace
{
struct DataArrayCopyTupleFunctor
{
  template <typename T>
  void operator()(IDataArray& outputIDataArray, size_t sourceIndex, size_t targetIndex)
  {
    using DataArrayType = DataArray<T>;
    DataArrayType outputArray = dynamic_cast<DataArrayType&>(outputIDataArray);
    outputArray.copyTuple(sourceIndex, targetIndex);
  }
};
} // namespace

// -----------------------------------------------------------------------------
ErodeDilateCoordinationNumber::ErodeDilateCoordinationNumber(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             ErodeDilateCoordinationNumberInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ErodeDilateCoordinationNumber::~ErodeDilateCoordinationNumber() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ErodeDilateCoordinationNumber::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumber::operator()()
{

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const size_t totalPoints = featureIds.getNumberOfTuples();

  std::vector<int64> neighbors(totalPoints, -1);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  size_t numFeatures = 0;

  for(size_t i = 0; i < totalPoints; i++)
  {
    const int32 featureName = featureIds[i];
    if(featureName > numFeatures)
    {
      numFeatures = featureName;
    }
  }

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  const std::string attrMatName = m_InputValues->FeatureIdsArrayPath.getTargetName();
  const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

  std::vector<int32> featureCount(numFeatures + 1, 0);
  std::vector<int32> coordinationNumber(totalPoints, 0);
  bool keepGoing = true;
  int32 counter = 1;

  while(counter > 0 && keepGoing)
  {
    counter = 0;
    if(!m_InputValues->Loop)
    {
      keepGoing = false;
    }

    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      const int64 zStride = dims[0] * dims[1] * zIdx;
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        const int64 yStride = dims[0] * yIdx;
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 voxelIndex = zStride + yStride + xIdx;
          const int32 featureName = featureIds[voxelIndex];
          int32 coordination = 0;
          int32 most = 0;
          // Loop over the 6 face neighbors of the voxel
          std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            const int32 feature = featureIds[neighborPoint];
            if((featureName > 0 && feature == 0) || (featureName == 0 && feature > 0))
            {
              coordination = coordination + 1;
              featureCount[feature]++;
              const int32 current = featureCount[feature];
              if(current > most)
              {
                most = current;
                neighbors[voxelIndex] = neighborPoint;
              }
            }
          }
          coordinationNumber[voxelIndex] = coordination;
          const int64 neighbor = neighbors[voxelIndex];
          if(coordinationNumber[voxelIndex] >= m_InputValues->CoordinationNumber && coordinationNumber[voxelIndex] > 0)
          {
            // TODO: update to use IDataArray->copyTuple() function
            /******************************************************************
             * If this section is slow it is because we are having to use the
             * ExecuteDataFunction<T>() in order to call "copyTuple()" because
             * "copyTuple()" isn't in the IArray API set. Oh well.
             */
            for(const auto& voxelArray : voxelArrays)
            {
              ExecuteDataFunction(DataArrayCopyTupleFunctor{}, voxelArray->getDataType(), *voxelArray, neighbor, voxelIndex);
            }
          }
          // Loop over the 6 face neighbors of the voxel
          isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
            const int32 feature = featureIds[neighborPoint];
            if(feature > 0)
            {
              featureCount[feature] = 0;
            }
          }
        }
      }
    }
    for(int64 zIndex = 0; zIndex < dims[2]; zIndex++)
    {
      const auto zStride = static_cast<int64>(dims[0] * dims[1] * zIndex);
      for(int64 yIndex = 0; yIndex < dims[1]; yIndex++)
      {
        const auto yStride = static_cast<int64>(dims[0] * yIndex);
        for(int64 xIndex = 0; xIndex < dims[0]; xIndex++)
        {
          const int64 voxelIndex = zStride + yStride + xIndex;
          if(coordinationNumber[voxelIndex] >= m_InputValues->CoordinationNumber)
          {
            counter++;
          }
        }
      }
    }
  }

  return {};
}

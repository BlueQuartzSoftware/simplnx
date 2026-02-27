#include "ComputeFeatureNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

namespace
{
template <bool ProcessSurfaceFeaturesV, bool ProcessBoundaryCellsV>
struct ComputeFeatureNeighborsFunctor
{
  template <bool is3DV>
  Result<> operator()(BoolAbstractDataStore* surfaceFeatures, Int8AbstractDataStore* boundaryCells, Float32NeighborList& sharedSurfaceAreaList, Int32NeighborList& neighborsList,
                      Int32AbstractDataStore& numNeighbors, const Int32AbstractDataStore& featureIds, usize totalFeatures, const std::array<int64, 3>& dims, const std::array<float64, 3> spacing,
                      const std::array<int64, 6>& neighborVoxelIndexOffsets, const std::array<FaceNeighborType, 6>& faceNeighborInternalIdx, ThrottledMessenger& throttledMessenger,
                      const std::atomic_bool& shouldCancel) const
  {
    if(ProcessSurfaceFeaturesV)
    {
      if(surfaceFeatures == nullptr)
      {
        return MakeErrorResult(-789620, "Process Surface Features selected, but the supplied Surface Features Array invalid.");
      }
    }

    if(ProcessBoundaryCellsV)
    {
      if(boundaryCells == nullptr)
      {
        return MakeErrorResult(-789621, "Process Boundary Cells selected, but the supplied Boundary Cells Array invalid.");
      }
    }

    const usize totalPoints = featureIds.getNumberOfTuples();

    const std::array<float64, 6> precomputedFaceAreas = computeFaceSurfaceAreas(spacing);
    std::vector<std::map<usize, float64>> neighborSurfaceAreas(totalFeatures);
    std::vector<std::set<int32>> neighborVector(totalFeatures);

    // Loop over all points to generate the neighbor lists
    for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
    {
      throttledMessenger.sendThrottledMessage([&] { return fmt::format("Determining Neighbor Lists || {:.2f}% Complete", CalculatePercentComplete(voxelIndex, totalPoints)); });

      if(shouldCancel)
      {
        return {};
      }

      // This value tracks the number of neighboring cells that have feature ids different from itself
      int8 numDiffNeighbors = 0;
      int32 feature = featureIds.getValue(voxelIndex);
      if(feature > 0 && feature < neighborVector.size())
      {
        const int64 xIdx = voxelIndex % dims[0];
        const int64 yIdx = (voxelIndex / dims[0]) % dims[1];
        const int64 zIdx = voxelIndex / (dims[0] * dims[1]);

        std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
        if constexpr(ProcessSurfaceFeaturesV)
        {
          if constexpr(is3DV)
          {
            // For a face neighbor to be valid it must exist thus if there is a false in the array it is a boundary
            if(!isValidFaceNeighbor[k_NegativeZNeighbor] || !isValidFaceNeighbor[k_NegativeYNeighbor] || !isValidFaceNeighbor[k_NegativeXNeighbor] || !isValidFaceNeighbor[k_PositiveXNeighbor] ||
               !isValidFaceNeighbor[k_PositiveYNeighbor] || !isValidFaceNeighbor[k_PositiveZNeighbor])
            {
              surfaceFeatures->setValue(feature, true);
            }
          }
          else
          {
            /**
             * TODO:
             * - Fix 2D case to account for empty dimesnions other than Z
             * - Potentially add a Constexpr template variable to cut down the need to check empty dims every time
             */
            if((xIdx == 0 || xIdx == dims[0] - 1 || yIdx == 0 || yIdx == dims[1] - 1) && dims[2] == 1)
            {
              surfaceFeatures->setValue(feature, true);
            }
          }
        }

        // Loop over the 6 face neighbors of the voxel
        for(const auto faceIndex : faceNeighborInternalIdx) // ref more expensive than trivial copy for scalar types
        {
          if(!isValidFaceNeighbor[faceIndex])
          {
            continue;
          }

          const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

          if(featureIds[neighborPoint] != feature && featureIds[neighborPoint] > 0)
          {
            numDiffNeighbors++;
            const int32 neighborFeatureId = featureIds.getValue(neighborPoint);
            neighborVector[feature].insert(neighborFeatureId);
            neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex];
          }
        }
      }
      if constexpr(ProcessBoundaryCellsV)
      {
        boundaryCells->setValue(voxelIndex, numDiffNeighbors);
      }
    }

    for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
    {
      numNeighbors.setValue(featureIdx, static_cast<int32>(neighborVector[featureIdx].size()));

      // Set the vector for each list into the NeighborList Object
      auto sharedNeiLst = std::make_shared<NeighborList<int32>::VectorType>();
      sharedNeiLst->assign(neighborVector[featureIdx].begin(), neighborVector[featureIdx].end());
      neighborsList.setList(static_cast<int32>(featureIdx), sharedNeiLst);

      auto sharedSAL = std::make_shared<NeighborList<float32>::VectorType>();
      sharedSAL->resize(totalFeatures);
      for(const auto& [featureId, surfaceArea] : neighborSurfaceAreas[featureIdx])
      {
        sharedSAL->operator[](featureId) = static_cast<float32>(surfaceArea);
      }
      sharedSurfaceAreaList.setList(static_cast<int32>(featureIdx), sharedSAL);
    }

    return {};
  }
};

template <class FunctorT, class... ArgsT>
Result<> ProcessVoxels(const FunctorT& functor, const ImageGeom& imageGeom, ArgsT&&... args)
{
  const usize xDimSize = imageGeom.getNumXCells();
  const usize yDimSize = imageGeom.getNumYCells();
  const usize zDimSize = imageGeom.getNumZCells();

  // Treat dimensions of 1 as flat for image geom
  if(xDimSize == 1 || yDimSize == 1 || zDimSize == 1)
  {
    return functor.template operator()<false>(std::forward<ArgsT>(args)...);
  }

  return functor.template operator()<true>(std::forward<ArgsT>(args)...);
}
} // namespace

// -----------------------------------------------------------------------------
ComputeFeatureNeighbors::ComputeFeatureNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeFeatureNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureNeighbors::~ComputeFeatureNeighbors() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureNeighbors::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto& numNeighbors = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumberOfNeighborsPath)->getDataStoreRef();

  auto& neighborsList = m_DataStructure.getDataRefAs<Int32NeighborList>(m_InputValues->NeighborListPath);
  auto& sharedSurfaceAreaList = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->SharedSurfaceAreaListPath);

  usize totalFeatures = numNeighbors.getNumberOfTuples();

  /* Ensure that we will be able to work with the user selected featureId Array */
  const auto [minFeatureId, maxFeatureId] = std::minmax_element(featureIds.begin(), featureIds.end());
  if(static_cast<usize>(*maxFeatureId) >= totalFeatures)
  {
    std::stringstream out;
    out << "Data Array " << m_InputValues->FeatureIdsPath.getTargetName() << " has a maximum value of " << *maxFeatureId << " which is greater than the "
        << " number of features from array " << m_InputValues->NumberOfNeighborsPath.getTargetName() << " which has " << totalFeatures << ". Did you select the "
        << " incorrect array for the 'FeatureIds' array?";
    return MakeErrorResult(-24500, out.str());
  }

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  SizeVec3 uDims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {static_cast<int64>(uDims[0]), static_cast<int64>(uDims[1]), static_cast<int64>(uDims[2])};

  FloatVec3 spacing32 = imageGeom.getSpacing();

  std::array<float64, 3> spacing64 = {static_cast<float64>(spacing32[0]), static_cast<float64>(spacing32[1]), static_cast<float64>(spacing32[2])};

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  Result<> result;
  if(m_InputValues->StoreSurfaceFeatures && m_InputValues->StoreBoundaryCells)
  {
    // Surface Features filled with `false` by default during creation in preflight
    auto* surfaceFeatures = m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore();
    auto* boundaryCells = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore();
    result = ProcessVoxels(::ComputeFeatureNeighborsFunctor<true, true>{}, imageGeom, surfaceFeatures, boundaryCells, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures,
                           dims, spacing64, neighborVoxelIndexOffsets, faceNeighborInternalIdx, throttledMessenger, m_ShouldCancel);
  }
  else if(m_InputValues->StoreSurfaceFeatures)
  {
    // Surface Features filled with `false` by default during creation in preflight
    auto* surfaceFeatures = m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore();
    result = ProcessVoxels(::ComputeFeatureNeighborsFunctor<true, false>{}, imageGeom, surfaceFeatures, nullptr, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                           spacing64, neighborVoxelIndexOffsets, faceNeighborInternalIdx, throttledMessenger, m_ShouldCancel);
  }
  else if(m_InputValues->StoreBoundaryCells)
  {
    auto* boundaryCells = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore();
    result = ProcessVoxels(::ComputeFeatureNeighborsFunctor<false, true>{}, imageGeom, nullptr, boundaryCells, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                           spacing64, neighborVoxelIndexOffsets, faceNeighborInternalIdx, throttledMessenger, m_ShouldCancel);
  }
  else
  {
    result = ProcessVoxels(::ComputeFeatureNeighborsFunctor<false, false>{}, imageGeom, nullptr, nullptr, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                           spacing64, neighborVoxelIndexOffsets, faceNeighborInternalIdx, throttledMessenger, m_ShouldCancel);
  }

  if(result.invalid())
  {
    return result;
  }

  return {};
}

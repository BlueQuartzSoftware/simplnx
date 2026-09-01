#include "ComputeFeatureNeighborsDirect.hpp"

#include "ComputeFeatureNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

namespace
{
/**
 * @struct ComputeFeatureNeighborsFunctor
 * @brief Computes direct feature-neighbor output for selected optional arrays.
 * @tparam ProcessSurfaceFeaturesV True to mark surface features.
 * @tparam ProcessBoundaryCellsV True to write BoundaryCells values.
 *
 * The normal dispatcher uses this functor with resident Feature IDs. It performs
 * serial direct store access and does not establish generic DataArray or DataStore thread safety.
 */
template <bool ProcessSurfaceFeaturesV, bool ProcessBoundaryCellsV>
struct ComputeFeatureNeighborsFunctor
{
  /**
   * @brief Computes selected neighbor output for one image dimensionality.
   * @tparam ImageDimensionStateT Specifies ImageGeom dimensionality behavior.
   * @param surfaceFeatures Receives optional surface-feature flags.
   * @param boundaryCells Receives optional boundary-cell counts.
   * @param sharedSurfaceAreaList Receives shared surface areas.
   * @param neighborsList Receives neighboring Feature IDs.
   * @param numNeighbors Receives neighbor counts.
   * @param featureIds Supplies Feature IDs.
   * @param totalFeatures Identifies the feature output count.
   * @param dims Supplies image dimensions.
   * @param spacing Supplies image spacing.
   * @param throttledMessenger Supplies interior progress messages.
   * @param shouldCancel Signals cancellation in the 3D interior sweep.
   * @return Success, or an optional-output error.
   *
   * Boundary processing completes before cancellation checks begin. A cancellation
   * return can preserve partial BoundaryCells output.
   */
  template <detail::ImageDimensionality ImageDimensionStateT>
  Result<> operator()(BoolAbstractDataStore* surfaceFeatures, Int8AbstractDataStore* boundaryCells, Float32NeighborList& sharedSurfaceAreaList, Int32NeighborList& neighborsList,
                      Int32AbstractDataStore& numNeighbors, const Int32AbstractDataStore& featureIds, usize totalFeatures, const std::array<int64, 3>& dims, const std::array<float64, 3> spacing,
                      ThrottledMessenger& throttledMessenger, const std::atomic_bool& shouldCancel) const
  {
    constexpr FaceNeighborType k_NeighborCount = VoxelNeighbors<ImageDimensionStateT>::k_FaceNeighborCount;

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
    const std::array<int64, k_NeighborCount> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets<ImageDimensionStateT>(dims);

    const std::array<float64, k_NeighborCount> precomputedFaceAreas = computeFaceSurfaceAreas<ImageDimensionStateT>(spacing);
    std::vector<std::map<usize, float64>> neighborSurfaceAreas(totalFeatures);

    // Boundary cells validate only faces that exist before the 3D interior sweep.
    constexpr std::array<FaceNeighborType, k_NeighborCount> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx<ImageDimensionStateT>();

    // Process corner cells.
    {
      const auto processCornerCell = [&](const int64 zIndex, const int64 yIndex, const int64 xIndex) -> void {
        int8 numDiffNeighbors = 0;

        const int64 voxelIndex = (dims[0] * dims[1] * zIndex) + (dims[0] * yIndex) + xIndex;
        const int32 feature = featureIds.getValue(voxelIndex);
        if(feature > 0)
        {
          if constexpr(ProcessSurfaceFeaturesV)
          {
            surfaceFeatures->setValue(feature, true);
          }

          std::array<bool, k_NeighborCount> isValidFaceNeighbor = computeValidFaceNeighbors<ImageDimensionStateT>(xIndex, yIndex, zIndex, dims);
          for(const auto faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            const int32 neighborFeatureId = featureIds.getValue(neighborPoint);
            if(neighborFeatureId != feature && neighborFeatureId > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex];
            }
          }
        }
        if constexpr(ProcessBoundaryCellsV)
        {
          boundaryCells->setValue(voxelIndex, numDiffNeighbors);
        }
      };

      ImageDimensionalUtilities::ProcessCorners<ImageDimensionStateT>(processCornerCell, dims);
    }

    // Process edge cells.
    if constexpr(!std::is_same_v<ImageDimensionStateT, SingleVoxelImage>)
    {
      const auto processEdgeCell = [&](const int64 zIndex, const int64 yIndex, const int64 xIndex) -> void {
        int8 numDiffNeighbors = 0;

        const int64 voxelIndex = (dims[0] * dims[1] * zIndex) + (dims[0] * yIndex) + xIndex;
        const int32 feature = featureIds.getValue(voxelIndex);
        if(feature > 0)
        {
          if constexpr(ProcessSurfaceFeaturesV && !ImageDimensionStateT::Is1DImageDimsState())
          {
            surfaceFeatures->setValue(feature, true);
          }

          std::array<bool, k_NeighborCount> isValidFaceNeighbor = computeValidFaceNeighbors<ImageDimensionStateT>(xIndex, yIndex, zIndex, dims);
          for(const auto faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            const int32 neighborFeatureId = featureIds.getValue(neighborPoint);
            if(neighborFeatureId != feature && neighborFeatureId > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex];
            }
          }
        }
        if constexpr(ProcessBoundaryCellsV)
        {
          boundaryCells->setValue(voxelIndex, numDiffNeighbors);
        }
      };

      ImageDimensionalUtilities::ProcessEdges<ImageDimensionStateT>(processEdgeCell, dims);
    }

    // Process non-degenerate face cells.
    if constexpr(!ImageDimensionStateT::Is1DImageDimsState() && !std::is_same_v<ImageDimensionStateT, SingleVoxelImage>)
    {
      const auto processFaceCell = [&](const int64 zIndex, const int64 yIndex, const int64 xIndex, const std::vector<FaceNeighborType>& validFaces) -> void {
        int8 numDiffNeighbors = 0;

        const int64 voxelIndex = (dims[0] * dims[1] * zIndex) + (dims[0] * yIndex) + xIndex;
        const int32 feature = featureIds.getValue(voxelIndex);
        if(feature > 0)
        {
          if constexpr(ProcessSurfaceFeaturesV && std::is_same_v<ImageDimensionStateT, Image3D>)
          {
            surfaceFeatures->setValue(feature, true);
          }

          for(const auto faceIndex : validFaces)
          {
            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

            const int32 neighborFeatureId = featureIds.getValue(neighborPoint);
            if(neighborFeatureId != feature && neighborFeatureId > 0)
            {
              numDiffNeighbors++;
              neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex];
            }
          }
        }
        if constexpr(ProcessBoundaryCellsV)
        {
          boundaryCells->setValue(voxelIndex, numDiffNeighbors);
        }
      };

      ImageDimensionalUtilities::ProcessFaces<ImageDimensionStateT>(processFaceCell, dims);
    }

    // The 3D interior has six valid face neighbors and needs no validity checks.
    if constexpr(std::is_same_v<ImageDimensionStateT, Image3D>)
    {
      const usize totalPoints = featureIds.getNumberOfTuples();

      for(int64 zIndex = 1; zIndex < dims[2] - 1; zIndex++)
      {
        const int64 zStride = dims[0] * dims[1] * zIndex;
        for(int64 yIndex = 1; yIndex < dims[1] - 1; yIndex++)
        {
          const int64 yStride = dims[0] * yIndex;
          throttledMessenger.sendThrottledMessage([&] { return fmt::format("Determining Neighbor Lists || {:.2f}% Complete", CalculatePercentComplete(zStride + yStride, totalPoints)); });

          if(shouldCancel)
          {
            return {};
          }
          for(int64 xIndex = 1; xIndex < dims[0] - 1; xIndex++)
          {
            int64 voxelIndex = zStride + yStride + xIndex;

            int8 numDiffNeighbors = 0;
            int32 feature = featureIds.getValue(voxelIndex);
            if(feature > 0)
            {
              for(const auto faceIndex : faceNeighborInternalIdx)
              {
                const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

                const int32 neighborFeatureId = featureIds.getValue(neighborPoint);
                if(neighborFeatureId != feature && neighborFeatureId > 0)
                {
                  numDiffNeighbors++;
                  neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex];
                }
              }
            }
            if constexpr(ProcessBoundaryCellsV)
            {
              boundaryCells->setValue(voxelIndex, numDiffNeighbors);
            }
          }
        }
      }
    }

    for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
    {
      const usize neighborCount = neighborSurfaceAreas[featureIdx].size();
      numNeighbors.setValue(featureIdx, static_cast<int32>(neighborCount));

      auto sharedNeiLst = std::make_shared<NeighborList<int32>::VectorType>();
      sharedNeiLst->reserve(neighborCount);
      auto sharedSAL = std::make_shared<NeighborList<float32>::VectorType>();
      sharedSAL->reserve(neighborCount);
      for(const auto& [featureId, surfaceArea] : neighborSurfaceAreas[featureIdx])
      {
        sharedNeiLst->push_back(static_cast<int32>(featureId));
        sharedSAL->push_back(static_cast<float32>(surfaceArea));
      }
      neighborsList.setList(static_cast<int32>(featureIdx), sharedNeiLst);
      sharedSurfaceAreaList.setList(static_cast<int32>(featureIdx), sharedSAL);
    }

    return {};
  }
};

/**
 * @brief Selects dimensionality behavior for direct neighbor processing.
 * @tparam FunctorT Specifies the direct processing functor.
 * @tparam ArgsT Specifies forwarded processing argument types.
 * @param functor Supplies the direct processing implementation.
 * @param imageGeom Supplies image dimensions.
 * @param args Forwards arguments to the selected dimensionality specialization.
 * @return Result from the selected specialization.
 *
 * Unit dimensions select lower-dimensional behavior before voxel processing starts.
 */
template <class FunctorT, class... ArgsT>
Result<> ProcessVoxels(const FunctorT& functor, const ImageGeom& imageGeom, ArgsT&&... args)
{
  const bool xDimEmpty = imageGeom.getNumXCells() == 1;
  const bool yDimEmpty = imageGeom.getNumYCells() == 1;
  const bool zDimEmpty = imageGeom.getNumZCells() == 1;
  const uint8 emptyDimCount = static_cast<uint8>(xDimEmpty) + static_cast<uint8>(yDimEmpty) + static_cast<uint8>(zDimEmpty);

  // A unit dimension selects lower-dimensional ImageGeom behavior.
  if(emptyDimCount == 0)
  {
    return functor.template operator()<Image3D>(std::forward<ArgsT>(args)...);
  }
  if(emptyDimCount == 1)
  {
    if(zDimEmpty)
    {
      return functor.template operator()<EmptyZImage2D>(std::forward<ArgsT>(args)...);
    }
    if(yDimEmpty)
    {
      return functor.template operator()<EmptyYImage2D>(std::forward<ArgsT>(args)...);
    }
    if(xDimEmpty)
    {
      return functor.template operator()<EmptyXImage2D>(std::forward<ArgsT>(args)...);
    }
  }
  if(emptyDimCount == 2)
  {
    if(xDimEmpty && yDimEmpty)
    {
      return functor.template operator()<ZImage1D>(std::forward<ArgsT>(args)...);
    }
    if(xDimEmpty && zDimEmpty)
    {
      return functor.template operator()<YImage1D>(std::forward<ArgsT>(args)...);
    }
    if(yDimEmpty && zDimEmpty)
    {
      return functor.template operator()<XImage1D>(std::forward<ArgsT>(args)...);
    }
  }
  if(emptyDimCount == 3)
  {
    return functor.template operator()<SingleVoxelImage>(std::forward<ArgsT>(args)...);
  }

  return {};
}
} // namespace

ComputeFeatureNeighborsDirect::ComputeFeatureNeighborsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             const ComputeFeatureNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureNeighborsDirect::~ComputeFeatureNeighborsDirect() noexcept = default;

Result<> ComputeFeatureNeighborsDirect::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto& numNeighbors = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumberOfNeighborsPath)->getDataStoreRef();

  auto& neighborsList = m_DataStructure.getDataRefAs<Int32NeighborList>(m_InputValues->NeighborListPath);
  auto& sharedSurfaceAreaList = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->SharedSurfaceAreaListPath);

  usize totalFeatures = numNeighbors.getNumberOfTuples();

  // Validate Feature ID range before indexing feature output.
  const int32 maxFeatureId = *std::max_element(featureIds.cbegin(), featureIds.cend());
  if(static_cast<usize>(maxFeatureId) >= totalFeatures)
  {
    std::stringstream out;
    out << "Data Array " << m_InputValues->FeatureIdsPath.getTargetName() << " has a maximum value of " << maxFeatureId << " which is greater than the "
        << " number of features from array " << m_InputValues->NumberOfNeighborsPath.getTargetName() << " which has " << totalFeatures << ". Did you select the "
        << " incorrect array for the 'FeatureIds' array?";
    return MakeErrorResult(-24500, out.str());
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  SizeVec3 uDims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {static_cast<int64>(uDims[0]), static_cast<int64>(uDims[1]), static_cast<int64>(uDims[2])};

  FloatVec3 spacing32 = imageGeom.getSpacing();

  std::array<float64, 3> spacing64 = {static_cast<float64>(spacing32[0]), static_cast<float64>(spacing32[1]), static_cast<float64>(spacing32[2])};

  if(m_InputValues->StoreSurfaceFeatures && m_InputValues->StoreBoundaryCells)
  {
    // Preflight initializes surface flags to false. This path marks only geometry-face features.
    auto* surfaceFeatures = m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore();
    auto* boundaryCells = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore();
    return ProcessVoxels(::ComputeFeatureNeighborsFunctor<true, true>{}, imageGeom, surfaceFeatures, boundaryCells, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                         spacing64, throttledMessenger, m_ShouldCancel);
  }
  if(m_InputValues->StoreSurfaceFeatures)
  {
    // Preflight initializes surface flags to false. This path marks only geometry-face features.
    auto* surfaceFeatures = m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore();
    return ProcessVoxels(::ComputeFeatureNeighborsFunctor<true, false>{}, imageGeom, surfaceFeatures, nullptr, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                         spacing64, throttledMessenger, m_ShouldCancel);
  }
  if(m_InputValues->StoreBoundaryCells)
  {
    auto* boundaryCells = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore();
    return ProcessVoxels(::ComputeFeatureNeighborsFunctor<false, true>{}, imageGeom, nullptr, boundaryCells, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims,
                         spacing64, throttledMessenger, m_ShouldCancel);
  }

  return ProcessVoxels(::ComputeFeatureNeighborsFunctor<false, false>{}, imageGeom, nullptr, nullptr, sharedSurfaceAreaList, neighborsList, numNeighbors, featureIds, totalFeatures, dims, spacing64,
                       throttledMessenger, m_ShouldCancel);
}

#include "ComputeSurfaceAreaToVolumeScanline.hpp"

#include "ComputeSurfaceAreaToVolume.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

using namespace nx::core;

// ----------------------------------------------------------------------------
// ComputeSurfaceAreaToVolumeScanline -- Out-of-Core Algorithm
//
// Computes the surface-area-to-volume ratio (and optional sphericity) for each
// feature in an image geometry. Produces identical results to the Direct variant
// but uses a 3-slice rolling window for sequential bulk I/O.
//
// KEY DESIGN PRINCIPLE: ALL array access uses bulk I/O (copyIntoBuffer /
// copyFromBuffer). There are zero operator[] calls on any DataStore.
//
// The algorithm has three phases:
//
// Phase 1 -- Surface area accumulation via Z-slice rolling window:
//   Three std::vector<int32> buffers (prevSlice, curSlice, nextSlice) hold
//   adjacent Z-slices of FeatureIds. For each voxel in curSlice, the 6
//   neighbors are checked using these buffers:
//     - -Z / +Z: prevSlice[inSlice] / nextSlice[inSlice]
//     - -Y / +Y: curSlice[inSlice - xPoints] / curSlice[inSlice + xPoints]
//     - -X / +X: curSlice[inSlice - 1] / curSlice[inSlice + 1]
//   Shared face areas are accumulated into a local std::vector<float32>.
//
// Phase 2 -- Ratio computation with local caches:
//   The feature-level NumCells array is bulk-read into a local vector, the
//   SA/V ratio is computed locally, and the result is bulk-written back.
//
// Phase 3 -- Optional sphericity computation:
//   Same local-cache approach as Phase 2.
//
// MEMORY BUDGET:
//   - 3 Z-slice buffers: 3 * dimX * dimY * 4 bytes
//   - featureSurfaceArea: numFeatures * 4 bytes
//   - localNumCells: numFeatures * 4 bytes
//   - localSurfaceAreaVolumeRatio: numFeatures * 4 bytes
//   - localSphericity (if needed): numFeatures * 4 bytes
//   Total: ~12 * dimX * dimY + ~16 * numFeatures bytes
// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolumeScanline::ComputeSurfaceAreaToVolumeScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       const ComputeSurfaceAreaToVolumeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolumeScanline::~ComputeSurfaceAreaToVolumeScanline() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Computes surface-area-to-volume ratio (and optional sphericity) using a
 * 3-slice rolling window with sequential bulk I/O for out-of-core storage
 * compatibility.
 *
 * **Phase 1 -- Surface area accumulation**:
 *   - Initialize the rolling window by loading Z-slices 0 and 1.
 *   - For each Z-slice, iterate all voxels in Y-X order within curSlice.
 *   - For each voxel with FeatureId > 0, check 6 face neighbors:
 *     - -Z: prevSlice[inSlice], +Z: nextSlice[inSlice]
 *     - -Y: curSlice[inSlice - xPoints], +Y: curSlice[inSlice + xPoints]
 *     - -X: curSlice[inSlice - 1], +X: curSlice[inSlice + 1]
 *   - When a neighbor differs, add the appropriate face area.
 *   - After each Z-slice, rotate buffers and load the next slice.
 *
 * **Phase 2 -- Ratio computation**:
 *   - Bulk-read the feature-level NumCells array into a local vector.
 *   - Compute SA/V = featureSurfaceArea[i] / (voxelVol * numCells[i]).
 *   - Bulk-write the ratio array back to the OOC store.
 *
 * **Phase 3 -- Optional sphericity**:
 *   - Compute sphericity = (pi^(1/3) * (6V)^(2/3)) / SA in a local buffer.
 *   - Bulk-write the sphericity array back to the OOC store.
 *
 * @return Result<> indicating success, validation errors, or cancellation.
 */
Result<> ComputeSurfaceAreaToVolumeScanline::operator()()
{
  // -- Setup: Retrieve input arrays and geometry --

  // Cell-level FeatureIds: accessed via rolling window, never via operator[]
  auto featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& featureIdsStore = featureIdsArrayPtr->getDataStoreRef();

  // Feature-level NumCells: will be bulk-read into a local vector in Phase 2
  const auto& numCells = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumCellsArrayPath)->getDataStoreRef();

  // Output SA/V ratio: will be bulk-written from a local vector in Phase 2
  auto& surfaceAreaVolumeRatio = m_DataStructure.getDataAs<Float32Array>(m_InputValues->SurfaceAreaVolumeRatioArrayName)->getDataStoreRef();

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  // Validate that the max FeatureId does not exceed the feature AttributeMatrix size
  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->NumCellsArrayPath.getParent(), *featureIdsArrayPtr, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }
  auto numFeatures = static_cast<int32>(numCells.getNumberOfTuples());
  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 spacing = imageGeom.getSpacing();

  auto xPoints = static_cast<int64>(dims[0]);
  auto yPoints = static_cast<int64>(dims[1]);
  auto zPoints = static_cast<int64>(dims[2]);

  // Volume of a single voxel, used to convert numCells to physical volume
  float32 voxelVol = spacing[0] * spacing[1] * spacing[2];

  // Local accumulator for per-feature surface area
  std::vector<float32> featureSurfaceArea(static_cast<usize>(numFeatures), 0.0f);

  // Pre-compute face areas for each neighbor direction. These depend on the
  // voxel spacing and which axis the shared face is normal to:
  //   - Z-normal face (shared by +/-Z neighbors): area = spacing.x * spacing.y
  //   - X-normal face (shared by +/-X neighbors): area = spacing.y * spacing.z
  //   - Y-normal face (shared by +/-Y neighbors): area = spacing.z * spacing.x
  const float32 xyFaceArea = spacing[0] * spacing[1]; // Z-normal face area
  const float32 yzFaceArea = spacing[1] * spacing[2]; // X-normal face area
  const float32 zxFaceArea = spacing[2] * spacing[0]; // Y-normal face area

  // -- Phase 1: Surface area accumulation via Z-slice rolling window --

  // Each Z-slice has yPoints * xPoints voxels
  const usize sliceSize = static_cast<usize>(yPoints) * static_cast<usize>(xPoints);

  // Allocate the 3-slice rolling window
  std::vector<int32> prevSlice(sliceSize, 0);
  std::vector<int32> curSlice(sliceSize, 0);
  std::vector<int32> nextSlice(sliceSize, 0);

  // Load the first Z-slice (z=0) into curSlice
  featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
  // Pre-load the second Z-slice into nextSlice (if available)
  if(zPoints > 1)
  {
    featureIdsStore.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
  }

  for(int64 z = 0; z < zPoints; z++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    for(int64 y = 0; y < yPoints; y++)
    {
      for(int64 x = 0; x < xPoints; x++)
      {
        // Flat index within the current Z-slice buffer
        const usize inSlice = static_cast<usize>(y) * static_cast<usize>(xPoints) + static_cast<usize>(x);
        int32 currentFeatureId = curSlice[inSlice];
        // Skip background voxels (FeatureId <= 0)
        if(currentFeatureId < 1)
        {
          continue;
        }

        float32 onSurface = 0.0f;

        // -Z neighbor: same (y, x) position in the previous Z-slice buffer.
        // Only valid if z > 0 (not on the bottom face of the volume).
        if(z > 0)
        {
          if(prevSlice[inSlice] != currentFeatureId)
          {
            onSurface += xyFaceArea;
          }
        }

        // +Z neighbor: same (y, x) position in the next Z-slice buffer.
        // Only valid if z < zPoints-1 (not on the top face of the volume).
        if(z < zPoints - 1)
        {
          if(nextSlice[inSlice] != currentFeatureId)
          {
            onSurface += xyFaceArea;
          }
        }

        // -Y neighbor: one row back in the current slice (inSlice - xPoints).
        if(y > 0)
        {
          if(curSlice[inSlice - static_cast<usize>(xPoints)] != currentFeatureId)
          {
            onSurface += yzFaceArea;
          }
        }

        // +Y neighbor: one row forward in the current slice (inSlice + xPoints).
        if(y < yPoints - 1)
        {
          if(curSlice[inSlice + static_cast<usize>(xPoints)] != currentFeatureId)
          {
            onSurface += yzFaceArea;
          }
        }

        // -X neighbor: one element back in the current row (inSlice - 1).
        if(x > 0)
        {
          if(curSlice[inSlice - 1] != currentFeatureId)
          {
            onSurface += zxFaceArea;
          }
        }

        // +X neighbor: one element forward in the current row (inSlice + 1).
        if(x < xPoints - 1)
        {
          if(curSlice[inSlice + 1] != currentFeatureId)
          {
            onSurface += zxFaceArea;
          }
        }

        // Add this voxel's boundary face contributions to the feature total
        featureSurfaceArea[currentFeatureId] += onSurface;
      }
    }

    // Rotate the rolling window: prevSlice <- curSlice <- nextSlice.
    // std::swap is O(1) for vectors (pointer swap, no data copy).
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);
    // Load the next-next Z-slice into the freed buffer
    if(z + 2 < zPoints)
    {
      featureIdsStore.copyIntoBuffer(static_cast<usize>(z + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  // -- Phase 2: Ratio computation with local caches --

  // Bulk-read the feature-level NumCells array into a local vector to avoid
  // per-element OOC lookups during the ratio computation loop.
  const usize numFeaturesUSize = static_cast<usize>(numFeatures);
  std::vector<int32> localNumCells(numFeaturesUSize);
  numCells.copyIntoBuffer(0, nonstd::span<int32>(localNumCells.data(), numFeaturesUSize));

  // Compute SA/V ratio into a local buffer, then bulk-write to the OOC store.
  // This avoids numFeatures individual operator[] writes on the output array.
  std::vector<float32> localSurfaceAreaVolumeRatio(numFeaturesUSize, 0.0f);

  const float32 thirdRootPi = std::pow(nx::core::Constants::k_PiF, 0.333333f);
  for(usize i = 1; i < numFeaturesUSize; i++)
  {
    float32 featureVolume = voxelVol * localNumCells[i];
    localSurfaceAreaVolumeRatio[i] = featureSurfaceArea[i] / featureVolume;
  }
  // Single bulk write of the entire SA/V ratio array
  surfaceAreaVolumeRatio.copyFromBuffer(0, nonstd::span<const float32>(localSurfaceAreaVolumeRatio.data(), numFeaturesUSize));

  // -- Phase 3: Optional sphericity computation --

  if(m_InputValues->CalculateSphericity)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Computing Sphericity"));

    auto& sphericity = m_DataStructure.getDataAs<Float32Array>(m_InputValues->SphericityArrayName)->getDataStoreRef();
    // Compute sphericity into a local buffer, then bulk-write.
    // Sphericity = (pi^(1/3) * (6V)^(2/3)) / SA
    // A perfect sphere has sphericity = 1.0; irregular shapes have lower values.
    std::vector<float32> localSphericity(numFeaturesUSize, 0.0f);
    for(usize i = 1; i < numFeaturesUSize; i++)
    {
      float32 featureVolume = voxelVol * localNumCells[i];
      localSphericity[i] = (thirdRootPi * std::pow((6.0f * featureVolume), 0.66666f)) / featureSurfaceArea[i];
    }
    // Single bulk write of the entire sphericity array
    sphericity.copyFromBuffer(0, nonstd::span<const float32>(localSphericity.data(), numFeaturesUSize));
  }

  return {};
}

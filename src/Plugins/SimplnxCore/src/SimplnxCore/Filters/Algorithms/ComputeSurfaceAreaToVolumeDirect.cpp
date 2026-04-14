#include "ComputeSurfaceAreaToVolumeDirect.hpp"

#include "ComputeSurfaceAreaToVolume.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>

using namespace nx::core;

// ----------------------------------------------------------------------------
// ComputeSurfaceAreaToVolumeDirect -- In-Core Algorithm
//
// Computes the surface-area-to-volume ratio (and optional sphericity) for each
// feature in an image geometry. The algorithm has two phases:
//
// Phase 1 -- Surface area accumulation:
//   Iterate every voxel in Z-Y-X order. For each voxel with FeatureId > 0,
//   check its 6 face neighbors. When a neighbor belongs to a different feature,
//   the area of the shared face is added to the current feature's accumulator.
//   Face areas depend on the voxel spacing:
//     - Z-normal faces (shared by +/-Z neighbors): spacing[0] * spacing[1]
//     - Y-normal faces (shared by +/-Y neighbors): spacing[1] * spacing[2]
//     - X-normal faces (shared by +/-X neighbors): spacing[2] * spacing[0]
//
// Phase 2 -- Ratio and sphericity computation:
//   For each feature, divide accumulated surface area by feature volume
//   (numCells * voxelVolume). Optionally compute sphericity using:
//     sphericity = (pi^(1/3) * (6*V)^(2/3)) / SA
//
// Data access pattern: Uses operator[] on the FeatureIds DataStore with
// pre-computed flat-index offsets for the 6 face neighbors. This is efficient
// for in-memory data but would cause chunk thrashing on OOC storage.
// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolumeDirect::ComputeSurfaceAreaToVolumeDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   const ComputeSurfaceAreaToVolumeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolumeDirect::~ComputeSurfaceAreaToVolumeDirect() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Computes surface-area-to-volume ratio (and optional sphericity) using
 * direct in-memory array indexing.
 *
 * The algorithm proceeds in two phases:
 *
 * **Phase 1 -- Surface area accumulation** (voxel-level):
 *   For each voxel in Z-Y-X order, check 6 face neighbors via flat-index offsets.
 *   Boundary neighbors (outside the volume) are skipped. When a valid neighbor
 *   belongs to a different feature, the shared face area is added to the current
 *   feature's surface-area accumulator. The face area depends on which axis the
 *   face is normal to (Z-normal = spacing.x * spacing.y, etc.).
 *
 * **Phase 2 -- Ratio computation** (feature-level):
 *   For each feature (starting from feature 1, since feature 0 is background):
 *   - Compute volume = numCells * voxelVolume
 *   - SA/V ratio = surfaceArea / volume
 *   - Sphericity (optional) = (pi^(1/3) * (6*V)^(2/3)) / SA
 *
 * @return Result<> indicating success, validation errors, or cancellation.
 */
Result<> ComputeSurfaceAreaToVolumeDirect::operator()()
{
  // -- Setup: Retrieve input arrays and geometry --

  // Cell-level FeatureIds: one int32 per voxel identifying which feature owns it
  auto featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStoreRef = featureIdsArrayPtr->getDataStoreRef();

  // Feature-level NumCells: pre-computed count of how many voxels each feature has
  const auto& numCells = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumCellsArrayPath)->getDataStoreRef();

  // Output: SA/V ratio per feature
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

  // Local accumulator for per-feature surface area. Using a std::vector here
  // (rather than the output DataStore) because multiple voxels contribute to
  // the same feature and we need read-modify-write access during accumulation.
  std::vector<float32> featureSurfaceArea(static_cast<usize>(numFeatures), 0.0f);

  // Pre-compute flat-index offsets for the 6 face neighbors.
  // For a voxel at flat index i:
  //   -Z neighbor: i - (xPoints * yPoints)   (one full Z-slice back)
  //   -Y neighbor: i - xPoints               (one row back)
  //   -X neighbor: i - 1                     (one element back)
  //   +X neighbor: i + 1                     (one element forward)
  //   +Y neighbor: i + xPoints               (one row forward)
  //   +Z neighbor: i + (xPoints * yPoints)   (one full Z-slice forward)
  int64 neighborOffset[6] = {0, 0, 0, 0, 0, 0};
  neighborOffset[0] = -xPoints * yPoints; // -Z
  neighborOffset[1] = -xPoints;           // -Y
  neighborOffset[2] = -1;                 // -X
  neighborOffset[3] = 1;                  // +X
  neighborOffset[4] = xPoints;            // +Y
  neighborOffset[5] = xPoints * yPoints;  // +Z

  // -- Phase 1: Surface area accumulation --
  // Iterate every voxel, check 6 neighbors, accumulate shared face areas.

  for(int64 zIdx = 0; zIdx < zPoints; zIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    int64 zStride = zIdx * xPoints * yPoints;
    for(int64 yIdx = 0; yIdx < yPoints; yIdx++)
    {
      int64 yStride = yIdx * xPoints;
      for(int64 xIdx = 0; xIdx < xPoints; xIdx++)
      {
        float32 onSurface = 0.0f;
        int32 currentFeatureId = featureIdsStoreRef[zStride + yStride + xIdx];
        // Skip background voxels (FeatureId <= 0)
        if(currentFeatureId < 1)
        {
          continue;
        }

        // Check each of the 6 face neighbors. Skip neighbors that would be
        // outside the volume (boundary voxels have fewer valid neighbors).
        for(int32 neighborOffsetIndex = 0; neighborOffsetIndex < 6; neighborOffsetIndex++)
        {
          // Boundary guards: skip neighbor if it would be out of bounds
          if(neighborOffsetIndex == 0 && zIdx == 0)
          {
            continue;
          }
          if(neighborOffsetIndex == 5 && zIdx == (zPoints - 1))
          {
            continue;
          }
          if(neighborOffsetIndex == 1 && yIdx == 0)
          {
            continue;
          }
          if(neighborOffsetIndex == 4 && yIdx == (yPoints - 1))
          {
            continue;
          }
          if(neighborOffsetIndex == 2 && xIdx == 0)
          {
            continue;
          }
          if(neighborOffsetIndex == 3 && xIdx == (xPoints - 1))
          {
            continue;
          }

          int64 neighborIndex = zStride + yStride + xIdx + neighborOffset[neighborOffsetIndex];

          // If the neighbor belongs to a different feature, the shared face
          // contributes to this feature's surface area. The face area depends
          // on which axis the face is normal to.
          if(featureIdsStoreRef[neighborIndex] != currentFeatureId)
          {
            if(neighborOffsetIndex == 0 || neighborOffsetIndex == 5) // Z-normal face (XY plane)
            {
              onSurface = onSurface + spacing[0] * spacing[1];
            }
            if(neighborOffsetIndex == 1 || neighborOffsetIndex == 4) // Y-normal face (XZ plane)
            {
              onSurface = onSurface + spacing[1] * spacing[2];
            }
            if(neighborOffsetIndex == 2 || neighborOffsetIndex == 3) // X-normal face (YZ plane)
            {
              onSurface = onSurface + spacing[2] * spacing[0];
            }
          }
        }
        // Add this voxel's boundary face contributions to the feature total
        int32 featureId = featureIdsStoreRef[zStride + yStride + xIdx];
        featureSurfaceArea[featureId] = featureSurfaceArea[featureId] + onSurface;
      }
    }
  }

  // -- Phase 2: Ratio and sphericity computation --

  // Compute SA/V ratio for each feature (skip feature 0 = background)
  const float32 thirdRootPi = std::pow(nx::core::Constants::k_PiF, 0.333333f);
  for(usize i = 1; i < numFeatures; i++)
  {
    float32 featureVolume = voxelVol * numCells[i];
    surfaceAreaVolumeRatio[i] = featureSurfaceArea[i] / featureVolume;
  }

  // Optionally compute sphericity: how close each feature's shape is to a sphere.
  // Sphericity = (pi^(1/3) * (6V)^(2/3)) / SA
  // A perfect sphere has sphericity = 1.0; more irregular shapes have lower values.
  if(m_InputValues->CalculateSphericity)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Computing Sphericity"));

    auto& sphericity = m_DataStructure.getDataAs<Float32Array>(m_InputValues->SphericityArrayName)->getDataStoreRef();
    for(usize i = 1; i < static_cast<usize>(numFeatures); i++)
    {
      float32 featureVolume = voxelVol * numCells[i];
      sphericity[i] = (thirdRootPi * std::pow((6.0f * featureVolume), 0.66666f)) / featureSurfaceArea[i];
    }
  }

  return {};
}

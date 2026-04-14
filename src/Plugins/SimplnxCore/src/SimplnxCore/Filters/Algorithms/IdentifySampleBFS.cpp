// -----------------------------------------------------------------------------
// IdentifySampleBFS.cpp -- In-core BFS flood-fill for sample identification
// -----------------------------------------------------------------------------
//
// This file implements the BFS (breadth-first search) variant of the
// IdentifySample algorithm, optimized for in-core (contiguous memory) data
// access. The algorithm identifies the largest connected component of "good"
// voxels (mask == true) as the sample, removes satellite regions, and
// optionally fills interior holes.
//
// The BFS approach uses O(N) temporary bit vectors and relies on random access
// to the mask array via getValue(). This is efficient when data fits in RAM
// but causes chunk thrashing when data is stored out-of-core in compressed
// HDF5 chunks. For OOC data, IdentifySampleCCL should be used instead
// (selected automatically by the IdentifySample dispatcher).
//
// When slice-by-slice mode is enabled, this class delegates to the shared
// IdentifySampleSliceBySliceFunctor which performs BFS on individual 2D slices.
//
// See IdentifySampleBFS.hpp for detailed algorithm documentation.
// -----------------------------------------------------------------------------

#include "IdentifySampleBFS.hpp"

#include "IdentifySample.hpp"
#include "IdentifySampleCommon.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
// =============================================================================
// IdentifySampleBFSFunctor
// =============================================================================
// BFS flood-fill algorithm for identifying the largest connected component of
// "good" voxels in an image geometry, then optionally filling interior holes.
//
// The algorithm has two phases:
//
// Phase 1 (Find Largest Component):
//   BFS flood-fill discovers all connected components of good voxels
//   (goodVoxels == true). Each component is found by starting BFS from an
//   unchecked good voxel and expanding to all face-adjacent good neighbors.
//   The largest component by voxel count is tracked as "the sample". After
//   all components are found, any good voxels NOT in the largest component
//   are set to false (they are noise or satellite regions).
//   Uses O(N) memory: checked + sample vectors (std::vector<bool>, 1 bit each).
//
// Phase 2 (Hole Fill, optional):
//   If fillHoles is true, a second BFS pass runs on bad voxels
//   (goodVoxels == false). Each connected component of bad voxels is
//   discovered via BFS. During BFS, a `touchesBoundary` flag tracks whether
//   any voxel in the component lies on the domain boundary (x/y/z == 0 or
//   max). If the component does NOT touch the boundary, it is fully enclosed
//   by the sample and is an interior hole -- all its voxels are set to true.
//   If it touches the boundary, it is external empty space and left as-is.
//
// NOTE: Uses std::vector<bool> (1 bit per voxel) for minimal memory overhead.
// Fast for in-core data where random access is O(1), but causes chunk
// thrashing in OOC mode due to BFS visiting neighbors across chunk boundaries.
// Use IdentifySampleCCL for out-of-core compatible processing.
// =============================================================================
struct IdentifySampleBFSFunctor
{
  template <typename T>
  void operator()(const ImageGeom* imageGeom, IDataArray* goodVoxelsPtr, bool fillHoles, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    auto& goodVoxels = goodVoxelsPtr->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const auto totalPoints = static_cast<int64>(goodVoxelsPtr->getNumberOfTuples());

    SizeVec3 udims = imageGeom->getDimensions();

    std::array<int64, 3> dims = {
        static_cast<int64>(udims[0]),
        static_cast<int64>(udims[1]),
        static_cast<int64>(udims[2]),
    };

    int64 neighborPoint = 0;
    std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
    std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

    std::vector<int64> currentVList;
    std::vector<bool> checked(totalPoints, false); // O(N) bits: tracks visited voxels
    std::vector<bool> sample(totalPoints, false);  // O(N) bits: marks voxels in the largest component
    int64 biggestBlock = 0;

    MessageHelper messageHelper(messageHandler);

    // --- Phase 1: Find the largest contiguous set of good voxels ------------
    // BFS flood-fill from each unvisited good voxel. Track the largest
    // connected component found so far.
    messageHelper.sendMessage("Phase 1: Finding largest connected component of good voxels...");
    for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
    {
      if(shouldCancel)
      {
        return;
      }
      if(!checked[voxelIndex] && goodVoxels.getValue(voxelIndex))
      {
        // Start BFS from this seed voxel to discover one connected component
        currentVList.push_back(voxelIndex);
        usize count = 0;
        while(count < currentVList.size())
        {
          int64 index = currentVList[count];
          int64 xIdx = index % dims[0];
          int64 yIdx = (index / dims[0]) % dims[1];
          int64 zIdx = index / (dims[0] * dims[1]);
          std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }
            neighborPoint = index + neighborVoxelIndexOffsets[faceIndex];

            if(!checked[neighborPoint] && goodVoxels.getValue(neighborPoint))
            {
              currentVList.push_back(neighborPoint);
              checked[neighborPoint] = true;
            }
          }
          count++;
        }
        // If this component is the largest found so far, record it as the sample
        if(static_cast<int64>(currentVList.size()) >= biggestBlock)
        {
          biggestBlock = currentVList.size();
          sample.assign(totalPoints, false);
          for(int64 j = 0; j < biggestBlock; j++)
          {
            sample[currentVList[j]] = true;
          }
        }
        currentVList.clear();
      }
    }
    // Any good voxels NOT in the largest component are noise/satellites --
    // set them to false so only the primary sample remains.
    for(int64 i = 0; i < totalPoints; i++)
    {
      if(!sample[i] && goodVoxels.getValue(i))
      {
        goodVoxels.setValue(i, false);
      }
    }
    sample.clear();
    checked.assign(totalPoints, false);

    // --- Phase 2: Hole fill (optional) ----------------------------------------
    // BFS on bad voxels (goodVoxels == false). Each connected component of
    // bad voxels is checked: if any voxel in the component touches a domain
    // boundary face (x/y/z == 0 or max), the component is external empty
    // space and is left as-is. If the component is fully enclosed by the
    // sample (touchesBoundary == false), it is an interior hole and all
    // its voxels are set to true.
    if(fillHoles)
    {
      messageHelper.sendMessage("Phase 2: Filling holes in sample...");

      bool touchesBoundary = false;
      for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
      {
        if(shouldCancel)
        {
          return;
        }
        if(!checked[voxelIndex] && !goodVoxels.getValue(voxelIndex))
        {
          // BFS from this bad voxel to discover one connected component of
          // bad data. Track whether any voxel in the component is on a
          // domain boundary face.
          currentVList.push_back(voxelIndex);
          usize count = 0;
          touchesBoundary = false;
          while(count < currentVList.size())
          {
            int64 index = currentVList[count];
            int64 xIdx = index % dims[0];
            int64 yIdx = (index / dims[0]) % dims[1];
            int64 zIdx = index / (dims[0] * dims[1]);
            // Check if this voxel lies on any domain boundary face
            if(xIdx == 0 || xIdx == (dims[0] - 1) || yIdx == 0 || yIdx == (dims[1] - 1) || zIdx == 0 || zIdx == (dims[2] - 1))
            {
              touchesBoundary = true;
            }
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }
              neighborPoint = index + neighborVoxelIndexOffsets[faceIndex];

              if(!checked[neighborPoint] && !goodVoxels.getValue(neighborPoint))
              {
                currentVList.push_back(neighborPoint);
                checked[neighborPoint] = true;
              }
            }
            count++;
          }
          // If this bad-data component does not touch any boundary, it is
          // an interior hole -- fill it by setting all voxels to true.
          if(!touchesBoundary)
          {
            for(int64 j : currentVList)
            {
              goodVoxels.setValue(j, true);
            }
          }
          currentVList.clear();
        }
      }
    }
    checked.clear();
  }
};
} // namespace

// -----------------------------------------------------------------------------
IdentifySampleBFS::IdentifySampleBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const IdentifySampleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
IdentifySampleBFS::~IdentifySampleBFS() noexcept = default;

// -----------------------------------------------------------------------------
Result<> IdentifySampleBFS::operator()()
{
  auto* inputData = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
  const auto* imageGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometryPath);

  if(m_InputValues->SliceBySlice)
  {
    ExecuteDataFunction(IdentifySampleSliceBySliceFunctor{}, inputData->getDataType(), imageGeom, inputData, m_InputValues->FillHoles,
                        static_cast<IdentifySampleSliceBySliceFunctor::Plane>(m_InputValues->SliceBySlicePlaneIndex), m_MessageHandler, m_ShouldCancel);
  }
  else
  {
    ExecuteDataFunction(IdentifySampleBFSFunctor{}, inputData->getDataType(), imageGeom, inputData, m_InputValues->FillHoles, m_MessageHandler, m_ShouldCancel);
  }

  return {};
}

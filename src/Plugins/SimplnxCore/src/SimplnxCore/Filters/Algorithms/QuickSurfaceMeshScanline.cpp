#include "QuickSurfaceMeshScanline.hpp"

#include "QuickSurfaceMesh.hpp"
#include "TupleTransfer.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>
#include <random>
#include <set>

using namespace nx::core;

// -----------------------------------------------------------------------------
namespace
{
constexpr float64 k_RangeMin = 0.0;
constexpr float64 k_RangeMax = 1.0;
constexpr std::mt19937_64::result_type k_Seed = 3412341234123412;
std::mt19937_64 generator(k_Seed);
std::uniform_real_distribution<> distribution(k_RangeMin, k_RangeMax);

// Buffer-based flip functions that operate on raw int32 pointers
// -----------------------------------------------------------------------------
void FlipProblemVoxelCase1(int32* buf, QuickSurfaceMeshScanline::MeshIndexType v1, QuickSurfaceMeshScanline::MeshIndexType v2, QuickSurfaceMeshScanline::MeshIndexType v3,
                           QuickSurfaceMeshScanline::MeshIndexType v4, QuickSurfaceMeshScanline::MeshIndexType v5, QuickSurfaceMeshScanline::MeshIndexType v6)
{
  auto val = static_cast<float32>(distribution(generator));

  if(val < 0.25f)
  {
    buf[v6] = buf[v4];
  }
  else if(val < 0.5f)
  {
    buf[v6] = buf[v5];
  }
  else if(val < 0.75f)
  {
    buf[v1] = buf[v2];
  }
  else
  {
    buf[v1] = buf[v3];
  }
}

// -----------------------------------------------------------------------------
void FlipProblemVoxelCase2(int32* buf, QuickSurfaceMeshScanline::MeshIndexType v1, QuickSurfaceMeshScanline::MeshIndexType v2, QuickSurfaceMeshScanline::MeshIndexType v3,
                           QuickSurfaceMeshScanline::MeshIndexType v4)
{
  auto val = static_cast<float32>(distribution(generator));

  if(val < 0.125f)
  {
    buf[v1] = buf[v2];
  }
  else if(val < 0.25f)
  {
    buf[v1] = buf[v3];
  }
  else if(val < 0.375f)
  {
    buf[v2] = buf[v1];
  }
  if(val < 0.5f)
  {
    buf[v2] = buf[v4];
  }
  else if(val < 0.625f)
  {
    buf[v3] = buf[v1];
  }
  else if(val < 0.75f)
  {
    buf[v3] = buf[v4];
  }
  else if(val < 0.875f)
  {
    buf[v4] = buf[v2];
  }
  else
  {
    buf[v4] = buf[v3];
  }
}

// -----------------------------------------------------------------------------
void FlipProblemVoxelCase3(int32* buf, QuickSurfaceMeshScanline::MeshIndexType v1, QuickSurfaceMeshScanline::MeshIndexType v2, QuickSurfaceMeshScanline::MeshIndexType v3)
{
  auto val = static_cast<float32>(distribution(generator));

  if(val < 0.5f)
  {
    buf[v2] = buf[v1];
  }
  else
  {
    buf[v3] = buf[v1];
  }
}
} // namespace

// -----------------------------------------------------------------------------
QuickSurfaceMeshScanline::QuickSurfaceMeshScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   const QuickSurfaceMeshInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
  generator.seed(k_Seed);
}

// -----------------------------------------------------------------------------
QuickSurfaceMeshScanline::~QuickSurfaceMeshScanline() noexcept = default;

// -----------------------------------------------------------------------------
Result<> QuickSurfaceMeshScanline::operator()()
{
  auto& grid = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  SizeVec3 udims = grid.getDimensions();

  usize xP = udims[0];
  usize yP = udims[1];
  usize zP = udims[2];

  MeshIndexType nodeCount = 0;
  MeshIndexType triangleCount = 0;
  usize numFeatures = 0;

  if(m_InputValues->FixProblemVoxels)
  {
    correctProblemVoxels();
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  countActiveNodesAndTriangles(nodeCount, triangleCount, numFeatures);
  if(m_ShouldCancel)
  {
    return {};
  }

  ShapeType tupleShape = {triangleCount};
  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.resizeVertexList(nodeCount);
  triangleGeom.getFaceAttributeMatrix()->resizeTuples(tupleShape);
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({nodeCount});

  for(const auto& dataPath : m_InputValues->CreatedDataArrayPaths)
  {
    Result<> result = nx::core::ResizeAndReplaceDataArray(m_DataStructure, dataPath, tupleShape, nx::core::IDataAction::Mode::Execute);
  }

  createNodesAndTriangles(nodeCount, triangleCount, numFeatures);
  if(m_ShouldCancel)
  {
    return {};
  }

  Result<> windingResult = {};
  if(m_InputValues->RepairTriangleWinding)
  {
    triangleGeom.findElementNeighbors(true);
    const auto optionalId = triangleGeom.getElementNeighborsId();
    if(!optionalId.has_value())
    {
      return MakeErrorResult(-56341, fmt::format("Unable to generate the connectivity list for {} geometry.", triangleGeom.getName()));
    }
    const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());

    windingResult = MeshingUtilities::RepairTriangleWinding(triangleGeom.getFaces()->getDataStoreRef(), connectivity,
                                                            m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef(), m_ShouldCancel, m_MessageHandler);

    m_DataStructure.removeData(triangleGeom.getElementContainingVertId().value());
    m_DataStructure.removeData(triangleGeom.getElementNeighborsId().value());
  }

  return windingResult;
}

// -----------------------------------------------------------------------------
void QuickSurfaceMeshScanline::correctProblemVoxels()
{
  m_MessageHandler(IFilter::Message::Type::Info, "Correcting Problem Voxels");

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  const MeshIndexType sliceSize = xP * yP;

  // Buffer two consecutive z-slices at a time
  auto sliceA = std::make_unique<int32[]>(sliceSize);
  auto sliceB = std::make_unique<int32[]>(sliceSize);

  MeshIndexType count = 1;
  MeshIndexType iter = 0;
  while(count > 0 && iter < 20)
  {
    if(m_ShouldCancel)
    {
      return;
    }
    iter++;
    count = 0;

    for(MeshIndexType k = 1; k < zP; k++)
    {
      // Load slice (k-1) into sliceA and slice k into sliceB
      featureIdsStore.copyIntoBuffer((k - 1) * sliceSize, nonstd::span<int32>(sliceA.get(), sliceSize));
      featureIdsStore.copyIntoBuffer(k * sliceSize, nonstd::span<int32>(sliceB.get(), sliceSize));

      bool sliceADirty = false;
      bool sliceBDirty = false;

      for(MeshIndexType j = 1; j < yP; j++)
      {
        MeshIndexType row1 = (j - 1) * xP;
        MeshIndexType row2 = j * xP;
        for(MeshIndexType i = 1; i < xP; i++)
        {
          // v1-v4 are in slice (k-1) = sliceA, v5-v8 are in slice k = sliceB
          MeshIndexType v1Local = row1 + i - 1;
          MeshIndexType v2Local = row1 + i;
          MeshIndexType v3Local = row2 + i - 1;
          MeshIndexType v4Local = row2 + i;

          int32 f1 = sliceA[v1Local];
          int32 f2 = sliceA[v2Local];
          int32 f3 = sliceA[v3Local];
          int32 f4 = sliceA[v4Local];
          int32 f5 = sliceB[v1Local];
          int32 f6 = sliceB[v2Local];
          int32 f7 = sliceB[v3Local];
          int32 f8 = sliceB[v4Local];

          // For the flip functions, we need indices into a combined 2-slice buffer.
          // sliceA occupies [0, sliceSize), sliceB occupies [sliceSize, 2*sliceSize)
          // But since FlipProblemVoxelCase functions operate on voxels that may be
          // in either slice, we use a combined buffer approach:
          // We'll use local indices directly into the correct slice buffer.

          if(f1 == f8 && f1 != f2 && f1 != f3 && f1 != f4 && f1 != f5 && f1 != f6 && f1 != f7)
          {
            // v1=plane1+row1+i-1, v2=plane1+row1+i, v3=plane1+row2+i-1
            // v6=plane2+row1+i, v7=plane2+row2+i-1, v8=plane2+row2+i
            // FlipProblemVoxelCase1(featureIds, v1, v2, v3, v6, v7, v8)
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.25f)
            {
              sliceB[v4Local] = sliceB[v2Local]; // v8 = v6
              sliceBDirty = true;
            }
            else if(val < 0.5f)
            {
              sliceB[v4Local] = sliceB[v3Local]; // v8 = v7
              sliceBDirty = true;
            }
            else if(val < 0.75f)
            {
              sliceA[v1Local] = sliceA[v2Local]; // v1 = v2
              sliceADirty = true;
            }
            else
            {
              sliceA[v1Local] = sliceA[v3Local]; // v1 = v3
              sliceADirty = true;
            }
            count++;
          }
          if(f2 == f7 && f2 != f1 && f2 != f3 && f2 != f4 && f2 != f5 && f2 != f6 && f2 != f8)
          {
            // FlipProblemVoxelCase1(featureIds, v2, v1, v4, v5, v8, v7)
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.25f)
            {
              sliceB[v3Local] = sliceB[v1Local]; // v7 = v5
              sliceBDirty = true;
            }
            else if(val < 0.5f)
            {
              sliceB[v3Local] = sliceB[v4Local]; // v7 = v8
              sliceBDirty = true;
            }
            else if(val < 0.75f)
            {
              sliceA[v2Local] = sliceA[v1Local]; // v2 = v1
              sliceADirty = true;
            }
            else
            {
              sliceA[v2Local] = sliceA[v4Local]; // v2 = v4
              sliceADirty = true;
            }
            count++;
          }
          if(f3 == f6 && f3 != f1 && f3 != f2 && f3 != f4 && f3 != f5 && f3 != f7 && f3 != f8)
          {
            // FlipProblemVoxelCase1(featureIds, v3, v1, v4, v5, v8, v6)
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.25f)
            {
              sliceB[v2Local] = sliceB[v1Local]; // v6 = v5
              sliceBDirty = true;
            }
            else if(val < 0.5f)
            {
              sliceB[v2Local] = sliceB[v4Local]; // v6 = v8
              sliceBDirty = true;
            }
            else if(val < 0.75f)
            {
              sliceA[v3Local] = sliceA[v1Local]; // v3 = v1
              sliceADirty = true;
            }
            else
            {
              sliceA[v3Local] = sliceA[v4Local]; // v3 = v4
              sliceADirty = true;
            }
            count++;
          }
          if(f4 == f5 && f4 != f1 && f4 != f2 && f4 != f3 && f4 != f6 && f4 != f7 && f4 != f8)
          {
            // FlipProblemVoxelCase1(featureIds, v4, v2, v3, v6, v7, v5)
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.25f)
            {
              sliceB[v1Local] = sliceB[v2Local]; // v5 = v6
              sliceBDirty = true;
            }
            else if(val < 0.5f)
            {
              sliceB[v1Local] = sliceB[v3Local]; // v5 = v7
              sliceBDirty = true;
            }
            else if(val < 0.75f)
            {
              sliceA[v4Local] = sliceA[v2Local]; // v4 = v2
              sliceADirty = true;
            }
            else
            {
              sliceA[v4Local] = sliceA[v3Local]; // v4 = v3
              sliceADirty = true;
            }
            count++;
          }

          // Case2 variants - these use FlipProblemVoxelCase2 which operates on 4 voxels
          // We inline the RNG consumption but delegate to a helper for the actual mutation
          auto doCase2 = [&](int32* bufX, MeshIndexType ix1, int32* bufY, MeshIndexType iy1, int32* bufZ, MeshIndexType iz1, int32* bufW, MeshIndexType iw1, bool& dirtyX, bool& dirtyY, bool& dirtyZ,
                             bool& dirtyW) {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.125f)
            {
              bufX[ix1] = bufY[iy1];
              dirtyX = true;
            }
            else if(val < 0.25f)
            {
              bufX[ix1] = bufZ[iz1];
              dirtyX = true;
            }
            else if(val < 0.375f)
            {
              bufY[iy1] = bufX[ix1];
              dirtyY = true;
            }
            if(val < 0.5f)
            {
              bufY[iy1] = bufW[iw1];
              dirtyY = true;
            }
            else if(val < 0.625f)
            {
              bufZ[iz1] = bufX[ix1];
              dirtyZ = true;
            }
            else if(val < 0.75f)
            {
              bufZ[iz1] = bufW[iw1];
              dirtyZ = true;
            }
            else if(val < 0.875f)
            {
              bufW[iw1] = bufY[iy1];
              dirtyW = true;
            }
            else
            {
              bufW[iw1] = bufZ[iz1];
              dirtyW = true;
            }
          };

          // f1==f6: v1(A),v2(A),v5(B),v6(B)
          if(f1 == f6 && f1 != f2 && f1 != f5)
          {
            doCase2(sliceA.get(), v1Local, sliceA.get(), v2Local, sliceB.get(), v1Local, sliceB.get(), v2Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f2 == f5 && f2 != f1 && f2 != f6)
          {
            doCase2(sliceA.get(), v2Local, sliceA.get(), v1Local, sliceB.get(), v2Local, sliceB.get(), v1Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f3 == f8 && f3 != f4 && f3 != f7)
          {
            doCase2(sliceA.get(), v3Local, sliceA.get(), v4Local, sliceB.get(), v3Local, sliceB.get(), v4Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f4 == f7 && f4 != f3 && f4 != f8)
          {
            doCase2(sliceA.get(), v4Local, sliceA.get(), v3Local, sliceB.get(), v4Local, sliceB.get(), v3Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f1 == f7 && f1 != f3 && f1 != f5)
          {
            doCase2(sliceA.get(), v1Local, sliceA.get(), v3Local, sliceB.get(), v1Local, sliceB.get(), v3Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f3 == f5 && f3 != f1 && f3 != f7)
          {
            doCase2(sliceA.get(), v3Local, sliceA.get(), v1Local, sliceB.get(), v3Local, sliceB.get(), v1Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f2 == f8 && f2 != f4 && f2 != f6)
          {
            doCase2(sliceA.get(), v2Local, sliceA.get(), v4Local, sliceB.get(), v2Local, sliceB.get(), v4Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f4 == f6 && f4 != f2 && f4 != f8)
          {
            doCase2(sliceA.get(), v4Local, sliceA.get(), v2Local, sliceB.get(), v4Local, sliceB.get(), v2Local, sliceADirty, sliceADirty, sliceBDirty, sliceBDirty);
            count++;
          }
          // Same-plane Case2 variants (all in sliceA or all in sliceB)
          if(f1 == f4 && f1 != f2 && f1 != f3)
          {
            doCase2(sliceA.get(), v1Local, sliceA.get(), v2Local, sliceA.get(), v3Local, sliceA.get(), v4Local, sliceADirty, sliceADirty, sliceADirty, sliceADirty);
            count++;
          }
          if(f2 == f3 && f2 != f1 && f2 != f4)
          {
            doCase2(sliceA.get(), v2Local, sliceA.get(), v1Local, sliceA.get(), v4Local, sliceA.get(), v3Local, sliceADirty, sliceADirty, sliceADirty, sliceADirty);
            count++;
          }
          if(f5 == f8 && f5 != f6 && f5 != f7)
          {
            doCase2(sliceB.get(), v1Local, sliceB.get(), v2Local, sliceB.get(), v3Local, sliceB.get(), v4Local, sliceBDirty, sliceBDirty, sliceBDirty, sliceBDirty);
            count++;
          }
          if(f6 == f7 && f6 != f5 && f6 != f8)
          {
            doCase2(sliceB.get(), v2Local, sliceB.get(), v1Local, sliceB.get(), v4Local, sliceB.get(), v3Local, sliceBDirty, sliceBDirty, sliceBDirty, sliceBDirty);
            count++;
          }

          // Case3 variants
          if(f2 == f3 && f2 == f4 && f2 == f5 && f2 == f6 && f2 == f7 && f2 != f1 && f2 != f8)
          {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.5f)
            {
              sliceA[v1Local] = sliceA[v2Local]; // v1 = v2
              sliceADirty = true;
            }
            else
            {
              sliceB[v4Local] = sliceA[v2Local]; // v8 = v2
              sliceBDirty = true;
            }
            count++;
          }
          if(f1 == f3 && f1 == f4 && f1 == f5 && f1 == f7 && f2 == f8 && f1 != f2 && f1 != f7)
          {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.5f)
            {
              sliceA[v2Local] = sliceA[v1Local]; // v2 = v1
              sliceADirty = true;
            }
            else
            {
              sliceB[v3Local] = sliceA[v1Local]; // v7 = v1
              sliceBDirty = true;
            }
            count++;
          }
          if(f1 == f2 && f1 == f4 && f1 == f5 && f1 == f7 && f1 == f8 && f1 != f3 && f1 != f6)
          {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.5f)
            {
              sliceA[v3Local] = sliceA[v1Local]; // v3 = v1
              sliceADirty = true;
            }
            else
            {
              sliceB[v2Local] = sliceA[v1Local]; // v6 = v1
              sliceBDirty = true;
            }
            count++;
          }
          if(f1 == f2 && f1 == f3 && f1 == f6 && f1 == f7 && f1 == f8 && f1 != f4 && f1 != f5)
          {
            auto val = static_cast<float32>(distribution(generator));
            if(val < 0.5f)
            {
              sliceA[v4Local] = sliceA[v1Local]; // v4 = v1
              sliceADirty = true;
            }
            else
            {
              sliceB[v1Local] = sliceA[v1Local]; // v5 = v1
              sliceBDirty = true;
            }
            count++;
          }
        }
      }

      // Write back dirty slices
      if(sliceADirty)
      {
        featureIdsStore.copyFromBuffer((k - 1) * sliceSize, nonstd::span<const int32>(sliceA.get(), sliceSize));
      }
      if(sliceBDirty)
      {
        featureIdsStore.copyFromBuffer(k * sliceSize, nonstd::span<const int32>(sliceB.get(), sliceSize));
      }
    }

    std::string ss = fmt::format("Correcting Problem Voxels: Iteration - '{}'; Problem Voxels - '{}'", iter, count);
    m_MessageHandler(IFilter::Message::Type::Info, ss);
  }
}

// -----------------------------------------------------------------------------
void QuickSurfaceMeshScanline::countActiveNodesAndTriangles(MeshIndexType& nodeCount, MeshIndexType& triangleCount, usize& numFeatures)
{
  m_MessageHandler(IFilter::Message::Type::Info, "Counting active nodes and triangles");

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  const MeshIndexType sliceSize = xP * yP;
  const MeshIndexType nodePlaneSize = (xP + 1) * (yP + 1);
  constexpr auto kMax = std::numeric_limits<MeshIndexType>::max();

  // Rolling node-plane buffers: O(2 * nodePlaneSize) instead of O((xP+1)*(yP+1)*(zP+1))
  std::vector<MeshIndexType> nodePlane0(nodePlaneSize, kMax);
  std::vector<MeshIndexType> nodePlane1(nodePlaneSize, kMax);

  // Lambda to count a node: if not yet assigned, assign and increment
  auto countNode = [&](std::vector<MeshIndexType>& plane, MeshIndexType offset) {
    if(plane[offset] == kMax)
    {
      plane[offset] = nodeCount;
      nodeCount++;
    }
  };

  // Buffer current and next z-slices for featureIds
  auto curSlice = std::make_unique<int32[]>(sliceSize);
  auto nextSlice = std::make_unique<int32[]>(sliceSize);

  // Load first slice
  featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.get(), sliceSize));

  numFeatures = 0;

  for(MeshIndexType k = 0; k < zP; k++)
  {
    if(m_ShouldCancel)
    {
      return;
    }
    // Load next z-slice if available
    if(k < zP - 1)
    {
      featureIdsStore.copyIntoBuffer((k + 1) * sliceSize, nonstd::span<int32>(nextSlice.get(), sliceSize));
    }

    for(MeshIndexType j = 0; j < yP; j++)
    {
      for(MeshIndexType i = 0; i < xP; i++)
      {
        MeshIndexType localIdx = j * xP + i;
        int32 curFeature = curSlice[localIdx];

        // Track max featureId for numFeatures
        if(static_cast<usize>(curFeature) > numFeatures)
        {
          numFeatures = static_cast<usize>(curFeature);
        }

        // Node offsets within a plane for node grid position (ni, nj):
        //   offset = nj * (xP + 1) + ni
        // Plane 0 corresponds to z=k, Plane 1 corresponds to z=k+1

        if(i == 0)
        {
          countNode(nodePlane0, j * (xP + 1) + i);
          countNode(nodePlane0, (j + 1) * (xP + 1) + i);
          countNode(nodePlane1, j * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
        if(j == 0)
        {
          countNode(nodePlane0, j * (xP + 1) + i);
          countNode(nodePlane0, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + i);
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          triangleCount += 2;
        }
        if(k == 0)
        {
          countNode(nodePlane0, j * (xP + 1) + i);
          countNode(nodePlane0, j * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + i);
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          triangleCount += 2;
        }
        if(i == (xP - 1))
        {
          countNode(nodePlane0, j * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          triangleCount += 2;
        }
        else if(curFeature != curSlice[localIdx + 1]) // neigh1 = point + 1
        {
          countNode(nodePlane0, j * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          triangleCount += 2;
        }
        if(j == (yP - 1))
        {
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
        else if(curFeature != curSlice[localIdx + xP]) // neigh2 = point + xP
        {
          countNode(nodePlane0, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane0, (j + 1) * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
        if(k == (zP - 1))
        {
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
        else if(curFeature != nextSlice[localIdx]) // neigh3 = point + xP*yP
        {
          countNode(nodePlane1, j * (xP + 1) + (i + 1));
          countNode(nodePlane1, j * (xP + 1) + i);
          countNode(nodePlane1, (j + 1) * (xP + 1) + (i + 1));
          countNode(nodePlane1, (j + 1) * (xP + 1) + i);
          triangleCount += 2;
        }
      }
    }

    // Rotate planes: plane1 becomes plane0 for next z-step, reinitialize plane1
    std::swap(nodePlane0, nodePlane1);
    std::fill(nodePlane1.begin(), nodePlane1.end(), kMax);

    // Swap featureId buffers: current becomes the old "next"
    std::swap(curSlice, nextSlice);
  }
}

// -----------------------------------------------------------------------------
void QuickSurfaceMeshScanline::createNodesAndTriangles(MeshIndexType nodeCount, MeshIndexType triangleCount, usize numFeatures)
{
  if(m_ShouldCancel)
  {
    return;
  }
  m_MessageHandler(IFilter::Message::Type::Info, "Creating mesh");

  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  auto* grid = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->GridGeomDataPath);

  SizeVec3 udims = grid->getDimensions();

  MeshIndexType xP = udims[0];
  MeshIndexType yP = udims[1];
  MeshIndexType zP = udims[2];

  const MeshIndexType sliceSize = xP * yP;
  const MeshIndexType nodePlaneSize = (xP + 1) * (yP + 1);
  constexpr auto kMax = std::numeric_limits<MeshIndexType>::max();

  auto* triangleGeom = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  ShapeType tDims = {nodeCount};
  triangleGeom->resizeVertexList(nodeCount);
  triangleGeom->resizeFaceList(triangleCount);
  triangleGeom->getFaceAttributeMatrix()->resizeTuples({triangleCount});
  triangleGeom->getVertexAttributeMatrix()->resizeTuples(tDims);

  auto& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef();

  auto& nodeTypesStore = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  nodeTypesStore.resizeTuples({nodeCount});

  VertexStore& vertex = triangleGeom->getVertices()->getDataStoreRef();
  TriStore& triangle = triangleGeom->getFaces()->getDataStoreRef();

  std::vector<std::set<int32>> ownerLists(nodeCount);

  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(usize i = 0; i < m_InputValues->SelectedCellDataArrayPaths.size(); i++)
  {
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedCellDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  for(usize i = 0; i < m_InputValues->SelectedFeatureDataArrayPaths.size(); i++)
  {
    ::AddFeatureTupleTransferInstance(m_DataStructure, m_InputValues->SelectedFeatureDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i + m_InputValues->SelectedCellDataArrayPaths.size()],
                                      m_InputValues->FeatureIdsArrayPath, tupleTransferFunctions);
  }

  // Buffer current and next z-slices for featureIds
  auto curSlice = std::make_unique<int32[]>(sliceSize);
  auto nextSlice = std::make_unique<int32[]>(sliceSize);

  // Rolling node-plane buffers: O(2 * nodePlaneSize) instead of O((xP+1)*(yP+1)*(zP+1))
  std::vector<MeshIndexType> nodePlane0(nodePlaneSize, kMax);
  std::vector<MeshIndexType> nodePlane1(nodePlaneSize, kMax);

  // Buffer all vertex coordinates in memory (O(surface_area), not O(volume)),
  // flushed once at the end to avoid per-element OOC writes.
  auto vertCoordBuf = std::make_unique<VertexStore::value_type[]>(nodeCount * 3);

  // Lambda to assign a node: if not yet assigned, assign sequential ID.
  // Always write vertex coordinates into the local buffer (last-write-wins
  // matches original behavior where multiple calls may pass different coords).
  auto assignNode = [&](std::vector<MeshIndexType>& plane, MeshIndexType offset, MeshIndexType& assignedNodeCount, usize coordX, usize coordY, usize coordZ) {
    if(plane[offset] == kMax)
    {
      plane[offset] = assignedNodeCount;
      assignedNodeCount++;
    }
    nx::core::Point3D<float64> tmpCoords = grid->getPlaneCoords(coordX, coordY, coordZ);
    MeshIndexType vi = plane[offset] * 3;
    vertCoordBuf[vi] = static_cast<VertexStore::value_type>(tmpCoords[0]);
    vertCoordBuf[vi + 1] = static_cast<VertexStore::value_type>(tmpCoords[1]);
    vertCoordBuf[vi + 2] = static_cast<VertexStore::value_type>(tmpCoords[2]);
  };

  // Load first slice
  featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.get(), sliceSize));

  MeshIndexType triangleIndex = 0;
  MeshIndexType assignedNodeCount = 0;

  // Per-slice buffers declared outside the loop so vector capacity is reused across slices.
  std::vector<MeshIndexType> triBuffer;
  std::vector<int32> faceLabelBuf;
  std::vector<QuickSurfaceTransferData> ttArgsBuf;

  for(MeshIndexType k = 0; k < zP; k++)
  {
    if(m_ShouldCancel)
    {
      return;
    }
    // Load next z-slice if available
    if(k < zP - 1)
    {
      featureIdsStore.copyIntoBuffer((k + 1) * sliceSize, nonstd::span<int32>(nextSlice.get(), sliceSize));
    }

    triBuffer.clear();
    faceLabelBuf.clear();
    ttArgsBuf.clear();

    for(MeshIndexType j = 0; j < yP; j++)
    {
      for(MeshIndexType i = 0; i < xP; i++)
      {
        MeshIndexType localIdx = j * xP + i;
        MeshIndexType point = k * sliceSize + localIdx;
        int32 curFeature = curSlice[localIdx];

        // Node plane offsets: offset = nj * (xP+1) + ni
        // nodePlane0 = z=k plane, nodePlane1 = z=k+1 plane

        if(i == 0)
        {
          MeshIndexType n1Off = j * (xP + 1) + i;
          MeshIndexType n2Off = (j + 1) * (xP + 1) + i;
          MeshIndexType n3Off = j * (xP + 1) + i;
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane0, n1Off, assignedNodeCount, i, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i, j, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i + 1, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid2);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid4);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        if(j == 0)
        {
          MeshIndexType n1Off = j * (xP + 1) + i;
          MeshIndexType n2Off = j * (xP + 1) + (i + 1);
          MeshIndexType n3Off = j * (xP + 1) + i;
          MeshIndexType n4Off = j * (xP + 1) + (i + 1);
          assignNode(nodePlane0, n1Off, assignedNodeCount, i, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i + 1, j, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i, j, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i + 1, j, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid4);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        if(k == 0)
        {
          MeshIndexType n1Off = j * (xP + 1) + i;
          MeshIndexType n2Off = j * (xP + 1) + (i + 1);
          MeshIndexType n3Off = (j + 1) * (xP + 1) + i;
          MeshIndexType n4Off = (j + 1) * (xP + 1) + (i + 1);
          assignNode(nodePlane0, n1Off, assignedNodeCount, i, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i + 1, j, k);
          assignNode(nodePlane0, n3Off, assignedNodeCount, i, j + 1, k);
          assignNode(nodePlane0, n4Off, assignedNodeCount, i + 1, j + 1, k);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane0[n3Off];
          MeshIndexType nid4 = nodePlane0[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid2);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid4);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        if(i == (xP - 1))
        {
          MeshIndexType n1Off = j * (xP + 1) + (i + 1);
          MeshIndexType n2Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n3Off = j * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + (i + 1);
          assignNode(nodePlane0, n1Off, assignedNodeCount, i + 1, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i + 1, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i + 1, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid4);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        else if(curFeature != curSlice[localIdx + 1])
        {
          int32 neigh1Feature = curSlice[localIdx + 1];
          MeshIndexType neigh1 = point + 1;

          MeshIndexType n1Off = j * (xP + 1) + (i + 1);
          MeshIndexType n2Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n3Off = j * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + (i + 1);
          assignNode(nodePlane0, n1Off, assignedNodeCount, i + 1, j, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i + 1, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i + 1, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          if(curFeature < neigh1Feature)
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid2);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh1Feature);
            ttArgsBuf.push_back({triangleIndex, neigh1, point, curFeature, neigh1Feature});
          }
          else
          {
            triBuffer.push_back(nid2);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(neigh1Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh1, point, neigh1Feature, curFeature});
          }
          triangleIndex++;

          triBuffer.push_back(nid2);
          if(curFeature < neigh1Feature)
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid4);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh1Feature);
            ttArgsBuf.push_back({triangleIndex, neigh1, point, curFeature, neigh1Feature});
          }
          else
          {
            triBuffer.push_back(nid4);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(neigh1Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh1, point, neigh1Feature, curFeature});
          }
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(neigh1Feature);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(neigh1Feature);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(neigh1Feature);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(neigh1Feature);
        }
        if(j == (yP - 1))
        {
          MeshIndexType n1Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n2Off = (j + 1) * (xP + 1) + i;
          MeshIndexType n3Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane0, n1Off, assignedNodeCount, i + 1, j + 1, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j + 1, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid4);
          triBuffer.push_back(nid3);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        else if(curFeature != curSlice[localIdx + xP])
        {
          int32 neigh2Feature = curSlice[localIdx + xP];
          MeshIndexType neigh2 = point + xP;

          MeshIndexType n1Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n2Off = (j + 1) * (xP + 1) + i;
          MeshIndexType n3Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane0, n1Off, assignedNodeCount, i + 1, j + 1, k);
          assignNode(nodePlane0, n2Off, assignedNodeCount, i, j + 1, k);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j + 1, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane0[n1Off];
          MeshIndexType nid2 = nodePlane0[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          if(curFeature < neigh2Feature)
          {
            triBuffer.push_back(nid2);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh2Feature);
            ttArgsBuf.push_back({triangleIndex, neigh2, point, curFeature, neigh2Feature});
          }
          else
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid2);
            faceLabelBuf.push_back(neigh2Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh2, point, neigh2Feature, curFeature});
          }
          triangleIndex++;

          triBuffer.push_back(nid2);
          if(curFeature < neigh2Feature)
          {
            triBuffer.push_back(nid4);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh2Feature);
            ttArgsBuf.push_back({triangleIndex, neigh2, point, curFeature, neigh2Feature});
          }
          else
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid4);
            faceLabelBuf.push_back(neigh2Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh2, point, neigh2Feature, curFeature});
          }
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(neigh2Feature);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(neigh2Feature);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(neigh2Feature);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(neigh2Feature);
        }
        if(k == (zP - 1))
        {
          MeshIndexType n1Off = j * (xP + 1) + (i + 1);
          MeshIndexType n2Off = j * (xP + 1) + i;
          MeshIndexType n3Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane1, n1Off, assignedNodeCount, i + 1, j, k + 1);
          assignNode(nodePlane1, n2Off, assignedNodeCount, i, j, k + 1);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j + 1, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane1[n1Off];
          MeshIndexType nid2 = nodePlane1[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid2);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          triBuffer.push_back(nid2);
          triBuffer.push_back(nid3);
          triBuffer.push_back(nid4);
          faceLabelBuf.push_back(-1);
          faceLabelBuf.push_back(curFeature);
          ttArgsBuf.push_back({triangleIndex, point, point, -1, curFeature});
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(-1);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(-1);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(-1);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(-1);
        }
        else if(curFeature != nextSlice[localIdx])
        {
          int32 neigh3Feature = nextSlice[localIdx];
          MeshIndexType neigh3 = point + sliceSize;

          MeshIndexType n1Off = j * (xP + 1) + (i + 1);
          MeshIndexType n2Off = j * (xP + 1) + i;
          MeshIndexType n3Off = (j + 1) * (xP + 1) + (i + 1);
          MeshIndexType n4Off = (j + 1) * (xP + 1) + i;
          assignNode(nodePlane1, n1Off, assignedNodeCount, i + 1, j, k + 1);
          assignNode(nodePlane1, n2Off, assignedNodeCount, i, j, k + 1);
          assignNode(nodePlane1, n3Off, assignedNodeCount, i + 1, j + 1, k + 1);
          assignNode(nodePlane1, n4Off, assignedNodeCount, i, j + 1, k + 1);

          MeshIndexType nid1 = nodePlane1[n1Off];
          MeshIndexType nid2 = nodePlane1[n2Off];
          MeshIndexType nid3 = nodePlane1[n3Off];
          MeshIndexType nid4 = nodePlane1[n4Off];

          triBuffer.push_back(nid1);
          if(curFeature < neigh3Feature)
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid2);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh3Feature);
            ttArgsBuf.push_back({triangleIndex, neigh3, point, curFeature, neigh3Feature});
          }
          else
          {
            triBuffer.push_back(nid2);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(neigh3Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh3, point, neigh3Feature, curFeature});
          }
          triangleIndex++;

          triBuffer.push_back(nid2);
          if(curFeature < neigh3Feature)
          {
            triBuffer.push_back(nid3);
            triBuffer.push_back(nid4);
            faceLabelBuf.push_back(curFeature);
            faceLabelBuf.push_back(neigh3Feature);
            ttArgsBuf.push_back({triangleIndex, neigh3, point, curFeature, neigh3Feature});
          }
          else
          {
            triBuffer.push_back(nid4);
            triBuffer.push_back(nid3);
            faceLabelBuf.push_back(neigh3Feature);
            faceLabelBuf.push_back(curFeature);
            ttArgsBuf.push_back({triangleIndex, neigh3, point, neigh3Feature, curFeature});
          }
          triangleIndex++;

          ownerLists[nid1].insert(curFeature);
          ownerLists[nid1].insert(neigh3Feature);
          ownerLists[nid2].insert(curFeature);
          ownerLists[nid2].insert(neigh3Feature);
          ownerLists[nid3].insert(curFeature);
          ownerLists[nid3].insert(neigh3Feature);
          ownerLists[nid4].insert(curFeature);
          ownerLists[nid4].insert(neigh3Feature);
        }
      }
    }

    // Flush buffered triangle connectivity for this z-slice
    if(!triBuffer.empty())
    {
      MeshIndexType sliceTriStart = triangleIndex - (triBuffer.size() / 3);
      triangle.copyFromBuffer(sliceTriStart * 3, nonstd::span<const MeshIndexType>(triBuffer.data(), triBuffer.size()));

      // Flush buffered face labels for this z-slice
      faceLabelsStore.copyFromBuffer(sliceTriStart * 2, nonstd::span<const int32>(faceLabelBuf.data(), faceLabelBuf.size()));

      // Batch TupleTransfer calls with face labels embedded in the records
      for(const auto& tupleTransferFunction : tupleTransferFunctions)
      {
        tupleTransferFunction->quickSurfaceTransferBatch(nonstd::span<const QuickSurfaceTransferData>(ttArgsBuf.data(), ttArgsBuf.size()));
      }
    }

    // Rotate planes: plane1 becomes plane0 for next z-step, reinitialize plane1
    std::swap(nodePlane0, nodePlane1);
    std::fill(nodePlane1.begin(), nodePlane1.end(), kMax);

    // Swap featureId buffers
    std::swap(curSlice, nextSlice);
  }

  // Flush all buffered vertex coordinates in one bulk write
  vertex.copyFromBuffer(0, nonstd::span<const VertexStore::value_type>(vertCoordBuf.get(), nodeCount * 3));

  // Build node types in a local buffer to avoid per-element OOC writes
  auto nodeTypesBuf = std::make_unique<int8[]>(nodeCount);
  for(usize i = 0; i < nodeCount; i++)
  {
    if(m_ShouldCancel)
    {
      return;
    }

    auto& ownerList = ownerLists[i];

    int8 nodeType = static_cast<int8>(ownerList.size());
    if(nodeType > 4)
    {
      nodeType = 4;
    }
    if(ownerList.find(-1) != ownerList.end())
    {
      nodeType += 10;
    }
    nodeTypesBuf[i] = nodeType;
  }
  nodeTypesStore.copyFromBuffer(0, nonstd::span<const int8>(nodeTypesBuf.get(), nodeCount));
}

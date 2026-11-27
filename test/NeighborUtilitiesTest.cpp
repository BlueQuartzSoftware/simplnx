#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <set>
#include <tuple>
#include <vector>

using namespace nx::core;

namespace
{
// Row-major linear index for the standard (x, y, z) layout used by ImageGeom:
//   index = z * dims[0] * dims[1] + y * dims[0] + x
constexpr int64 LinearIndex(int64 x, int64 y, int64 z, const std::array<int64, 3>& dims)
{
  return z * dims[0] * dims[1] + y * dims[0] + x;
}
} // namespace

TEST_CASE("Simplnx::NeighborUtilities: VoxelNeighbors face counts", "[Simplnx][NeighborUtilities]")
{
  STATIC_REQUIRE(VoxelNeighbors<Image3DType>::k_FaceNeighborCount == 6);
  STATIC_REQUIRE(VoxelNeighbors<EmptyXImage2DType>::k_FaceNeighborCount == 4);
  STATIC_REQUIRE(VoxelNeighbors<EmptyYImage2DType>::k_FaceNeighborCount == 4);
  STATIC_REQUIRE(VoxelNeighbors<EmptyZImage2DType>::k_FaceNeighborCount == 4);
  STATIC_REQUIRE(VoxelNeighbors<XImage1DType>::k_FaceNeighborCount == 2);
  STATIC_REQUIRE(VoxelNeighbors<YImage1DType>::k_FaceNeighborCount == 2);
  STATIC_REQUIRE(VoxelNeighbors<ZImage1DType>::k_FaceNeighborCount == 2);
  STATIC_REQUIRE(VoxelNeighbors<SingleVoxelImageType>::k_FaceNeighborCount == 0);
}

TEST_CASE("Simplnx::NeighborUtilities: initializeFaceNeighborInternalIdx ordering", "[Simplnx][NeighborUtilities]")
{
  SECTION("Image3D")
  {
    constexpr auto k_Idx = InitializeFaceNeighborInternalIdx<Image3DType>();
    using N = VoxelNeighbors<Image3DType>;
    REQUIRE(k_Idx == std::array<FaceNeighborType, 6>{N::k_NegativeZNeighbor, N::k_NegativeYNeighbor, N::k_NegativeXNeighbor, N::k_PositiveXNeighbor, N::k_PositiveYNeighbor, N::k_PositiveZNeighbor});
  }
  SECTION("EmptyXImage2D")
  {
    constexpr auto k_Idx = InitializeFaceNeighborInternalIdx<EmptyXImage2DType>();
    using N = VoxelNeighbors<EmptyXImage2DType>;
    REQUIRE(k_Idx == std::array<FaceNeighborType, 4>{N::k_NegativeZNeighbor, N::k_NegativeYNeighbor, N::k_PositiveYNeighbor, N::k_PositiveZNeighbor});
  }
  SECTION("EmptyYImage2D")
  {
    constexpr auto k_Idx = InitializeFaceNeighborInternalIdx<EmptyYImage2DType>();
    using N = VoxelNeighbors<EmptyYImage2DType>;
    REQUIRE(k_Idx == std::array<FaceNeighborType, 4>{N::k_NegativeZNeighbor, N::k_NegativeXNeighbor, N::k_PositiveXNeighbor, N::k_PositiveZNeighbor});
  }
  SECTION("EmptyZImage2D")
  {
    constexpr auto k_Idx = InitializeFaceNeighborInternalIdx<EmptyZImage2DType>();
    using N = VoxelNeighbors<EmptyZImage2DType>;
    REQUIRE(k_Idx == std::array<FaceNeighborType, 4>{N::k_NegativeYNeighbor, N::k_NegativeXNeighbor, N::k_PositiveXNeighbor, N::k_PositiveYNeighbor});
  }
}

TEST_CASE("Simplnx::NeighborUtilities: initializeFaceNeighborOffsets strides for non-square dims", "[Simplnx][NeighborUtilities]")
{
  SECTION("Image3D {4, 7, 3} - unique non-square strides")
  {
    const std::array<int64, 3> dims = {4, 7, 3};
    const auto offsets = InitializeFaceNeighborOffsets<Image3DType>(dims);
    // Order: -Z, -Y, -X, +X, +Y, +Z
    REQUIRE(offsets == std::array<int64, 6>{-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]});
    // -Z and +Z strides must equal the X*Y plane size
    REQUIRE(offsets[0] == -28);
    REQUIRE(offsets[5] == 28);
  }
  SECTION("EmptyXImage2D {1, 5, 8} - index varies over (y, z)")
  {
    const std::array<int64, 3> dims = {1, 5, 8};
    const auto offsets = InitializeFaceNeighborOffsets<EmptyXImage2DType>(dims);
    // Linear index with x=0: z * dims[0] * dims[1] + y * dims[0] = z*5 + y (since dims[0]=1).
    // Y stride = 1, Z stride = dims[1] = 5.
    // Order: -Z, -Y, +Y, +Z
    REQUIRE(offsets == std::array<int64, 4>{-dims[1], -1, 1, dims[1]});
    REQUIRE(offsets[0] == -5);
    REQUIRE(offsets[3] == 5);

    // Verify arithmetic: moving y=0→1 at z=2 should shift the linear index by offsets[+Y].
    REQUIRE(LinearIndex(0, 1, 2, dims) - LinearIndex(0, 0, 2, dims) == offsets[2]);
    REQUIRE(LinearIndex(0, 3, 3, dims) - LinearIndex(0, 3, 2, dims) == offsets[3]);
  }
  SECTION("EmptyYImage2D {6, 1, 4} - index varies over (x, z)")
  {
    const std::array<int64, 3> dims = {6, 1, 4};
    const auto offsets = InitializeFaceNeighborOffsets<EmptyYImage2DType>(dims);
    // Linear index with y=0: z*dims[0]*1 + x = z*6 + x. X stride = 1, Z stride = dims[0] = 6.
    // Order: -Z, -X, +X, +Z
    REQUIRE(offsets == std::array<int64, 4>{-dims[0], -1, 1, dims[0]});
    REQUIRE(offsets[0] == -6);
    REQUIRE(offsets[3] == 6);

    REQUIRE(LinearIndex(3, 0, 1, dims) - LinearIndex(2, 0, 1, dims) == offsets[2]);
    REQUIRE(LinearIndex(2, 0, 2, dims) - LinearIndex(2, 0, 1, dims) == offsets[3]);
  }
  SECTION("EmptyZImage2D {7, 3, 1} - index varies over (x, y)")
  {
    const std::array<int64, 3> dims = {7, 3, 1};
    const auto offsets = InitializeFaceNeighborOffsets<EmptyZImage2DType>(dims);
    // Linear index with z=0: y*dims[0] + x = y*7 + x. X stride = 1, Y stride = dims[0] = 7.
    // Order: -Y, -X, +X, +Y
    REQUIRE(offsets == std::array<int64, 4>{-dims[0], -1, 1, dims[0]});
    REQUIRE(offsets[0] == -7);
    REQUIRE(offsets[3] == 7);

    REQUIRE(LinearIndex(4, 1, 0, dims) - LinearIndex(3, 1, 0, dims) == offsets[2]);
    REQUIRE(LinearIndex(3, 2, 0, dims) - LinearIndex(3, 1, 0, dims) == offsets[3]);
  }
  SECTION("1D image stride is ±1 for all three 1D shapes")
  {
    REQUIRE(InitializeFaceNeighborOffsets<XImage1DType>({5, 1, 1}) == std::array<int64, 2>{-1, 1});
    REQUIRE(InitializeFaceNeighborOffsets<YImage1DType>({1, 5, 1}) == std::array<int64, 2>{-1, 1});
    REQUIRE(InitializeFaceNeighborOffsets<ZImage1DType>({1, 1, 5}) == std::array<int64, 2>{-1, 1});
  }
  SECTION("SingleVoxelImage returns empty array")
  {
    const auto offsets = InitializeFaceNeighborOffsets<SingleVoxelImageType>({1, 1, 1});
    REQUIRE(offsets.empty());
  }
}

TEST_CASE("Simplnx::NeighborUtilities: computeValidFaceNeighbors bounds checks", "[Simplnx][NeighborUtilities]")
{
  SECTION("Image3D {4, 3, 2} - corners / edges / interior")
  {
    const std::array<int64, 3> dims = {4, 3, 2};
    // Order: -Z, -Y, -X, +X, +Y, +Z
    // Corner (0, 0, 0): no negative neighbors, all positive valid
    REQUIRE(ComputeValidFaceNeighbors<Image3DType>(0, 0, 0, dims) == std::array<bool, 6>{false, false, false, true, true, true});
    // Corner (3, 2, 1): all negative valid, no positive neighbors
    REQUIRE(ComputeValidFaceNeighbors<Image3DType>(3, 2, 1, dims) == std::array<bool, 6>{true, true, true, false, false, false});
    // Face centre of the X+ face (3, 1, 0): -Z false, -Y true, -X true, +X false, +Y true, +Z true
    REQUIRE(ComputeValidFaceNeighbors<Image3DType>(3, 1, 0, dims) == std::array<bool, 6>{false, true, true, false, true, true});
  }
  SECTION("EmptyZImage2D {5, 4, 1} - non-square 2D")
  {
    const std::array<int64, 3> dims = {5, 4, 1};
    // Order: -Y, -X, +X, +Y
    REQUIRE(ComputeValidFaceNeighbors<EmptyZImage2DType>(0, 0, 0, dims) == std::array<bool, 4>{false, false, true, true});
    REQUIRE(ComputeValidFaceNeighbors<EmptyZImage2DType>(4, 3, 0, dims) == std::array<bool, 4>{true, true, false, false});
    REQUIRE(ComputeValidFaceNeighbors<EmptyZImage2DType>(2, 2, 0, dims) == std::array<bool, 4>{true, true, true, true});
    // Last X column: -X true, +X false
    REQUIRE(ComputeValidFaceNeighbors<EmptyZImage2DType>(4, 1, 0, dims) == std::array<bool, 4>{true, true, false, true});
  }
  SECTION("EmptyXImage2D {1, 3, 5}")
  {
    const std::array<int64, 3> dims = {1, 3, 5};
    // Order: -Z, -Y, +Y, +Z
    REQUIRE(ComputeValidFaceNeighbors<EmptyXImage2DType>(0, 0, 0, dims) == std::array<bool, 4>{false, false, true, true});
    REQUIRE(ComputeValidFaceNeighbors<EmptyXImage2DType>(0, 2, 4, dims) == std::array<bool, 4>{true, true, false, false});
  }
  SECTION("EmptyYImage2D {4, 1, 3}")
  {
    const std::array<int64, 3> dims = {4, 1, 3};
    // Order: -Z, -X, +X, +Z
    REQUIRE(ComputeValidFaceNeighbors<EmptyYImage2DType>(0, 0, 0, dims) == std::array<bool, 4>{false, false, true, true});
    REQUIRE(ComputeValidFaceNeighbors<EmptyYImage2DType>(3, 0, 2, dims) == std::array<bool, 4>{true, true, false, false});
  }
  SECTION("1D shapes")
  {
    REQUIRE(ComputeValidFaceNeighbors<XImage1DType>(0, 0, 0, {5, 1, 1}) == std::array<bool, 2>{false, true});
    REQUIRE(ComputeValidFaceNeighbors<XImage1DType>(4, 0, 0, {5, 1, 1}) == std::array<bool, 2>{true, false});
    REQUIRE(ComputeValidFaceNeighbors<YImage1DType>(0, 2, 0, {1, 5, 1}) == std::array<bool, 2>{true, true});
    REQUIRE(ComputeValidFaceNeighbors<ZImage1DType>(0, 0, 4, {1, 1, 5}) == std::array<bool, 2>{true, false});
  }
}

TEST_CASE("Simplnx::NeighborUtilities: computeFaceSurfaceAreas precision and ordering", "[Simplnx][NeighborUtilities]")
{
  // Use values chosen so that float64 math is exactly representable but
  // float32 intermediates would visibly truncate. spacing = {0.1, 0.2, 0.3}.
  const std::array<float64, 3> spacing = {0.1, 0.2, 0.3};
  const float64 zFace = spacing[0] * spacing[1]; // 0.02
  const float64 yFace = spacing[0] * spacing[2]; // 0.03
  const float64 xFace = spacing[1] * spacing[2]; // 0.06

  SECTION("Image3D area ordering matches neighbor internal index order")
  {
    const auto areas = ComputeFaceSurfaceAreas<Image3DType>(spacing);
    // Order: -Z, -Y, -X, +X, +Y, +Z
    REQUIRE(areas == std::array<float64, 6>{zFace, yFace, xFace, xFace, yFace, zFace});
  }
  SECTION("EmptyZImage2D returns only Y and X faces")
  {
    const auto areas = ComputeFaceSurfaceAreas<EmptyZImage2DType>(spacing);
    // Order: -Y, -X, +X, +Y
    REQUIRE(areas == std::array<float64, 4>{yFace, xFace, xFace, yFace});
  }
  SECTION("Computations stay in float64 precision")
  {
    // spacing chosen so that the float32 round-trip loses precision in the 8th significant digit.
    const std::array<float64, 3> tight = {0.10000001, 0.20000002, 0.30000003};
    const auto areas = ComputeFaceSurfaceAreas<Image3DType>(tight);
    const float64 expectedZ = tight[0] * tight[1];
    REQUIRE(areas[0] == Approx(expectedZ).epsilon(1e-15));
    // If someone re-introduces a static_cast<float32> on the intermediate, the
    // relative error grows to ~1e-7, which this epsilon would flag.
  }
}

TEST_CASE("Simplnx::NeighborUtilities: ProcessCorners visits the expected indices", "[Simplnx][NeighborUtilities]")
{
  using VisitedType = std::set<std::tuple<int64, int64, int64>>;
  auto recorder = [](VisitedType& sink) { return [&](int64 z, int64 y, int64 x) { sink.insert({z, y, x}); }; };

  SECTION("Image3D 3x3x3 visits all 8 corners")
  {
    VisitedType sink;
    image_dimensional_utilities::ProcessCorners<Image3DType>(recorder(sink), {3, 3, 3});
    REQUIRE(sink == VisitedType{{0, 0, 0}, {2, 2, 2}, {0, 0, 2}, {0, 2, 0}, {2, 0, 0}, {0, 2, 2}, {2, 0, 2}, {2, 2, 0}});
  }
  SECTION("EmptyZImage2D 4x3x1 visits 4 corners in the XY plane")
  {
    VisitedType sink;
    image_dimensional_utilities::ProcessCorners<EmptyZImage2DType>(recorder(sink), {4, 3, 1});
    REQUIRE(sink == VisitedType{{0, 0, 0}, {0, 0, 3}, {0, 2, 0}, {0, 2, 3}});
  }
  SECTION("XImage1D 5x1x1 visits both endpoints")
  {
    VisitedType sink;
    image_dimensional_utilities::ProcessCorners<XImage1DType>(recorder(sink), {5, 1, 1});
    REQUIRE(sink == VisitedType{{0, 0, 0}, {0, 0, 4}});
  }
  SECTION("SingleVoxelImage visits only the origin")
  {
    VisitedType sink;
    image_dimensional_utilities::ProcessCorners<SingleVoxelImageType>(recorder(sink), {1, 1, 1});
    REQUIRE(sink == VisitedType{{0, 0, 0}});
  }
}

TEST_CASE("Simplnx::NeighborUtilities: ProcessEdges visits the expected indices", "[Simplnx][NeighborUtilities]")
{
  using VisitedType = std::vector<std::tuple<int64, int64, int64>>;
  auto recorder = [](VisitedType& sink) { return [&](int64 z, int64 y, int64 x) { sink.emplace_back(z, y, x); }; };

  SECTION("EmptyZImage2D 5x4x1 walks the X-edge (length 3) and Y-edge (length 2)")
  {
    VisitedType sink;
    image_dimensional_utilities::ProcessEdges<EmptyZImage2DType>(recorder(sink), {5, 4, 1});
    // Expected: along y=0 and y=Ny-1 for x=1..Nx-2 (X edges), then x=0 and x=Nx-1 for y=1..Ny-2 (Y edges).
    // X edges: (0,0,1),(0,0,2),(0,0,3),(0,3,1),(0,3,2),(0,3,3)
    // Y edges: (0,1,0),(0,1,4),(0,2,0),(0,2,4)
    std::set<std::tuple<int64, int64, int64>> actual(sink.begin(), sink.end());
    REQUIRE(actual == std::set<std::tuple<int64, int64, int64>>{{0, 0, 1}, {0, 0, 2}, {0, 0, 3}, {0, 3, 1}, {0, 3, 2}, {0, 3, 3}, {0, 1, 0}, {0, 1, 4}, {0, 2, 0}, {0, 2, 4}});
    REQUIRE(sink.size() == 10);
  }
  SECTION("XImage1D 5x1x1 walks only the X interior")
  {
    VisitedType sink;
    image_dimensional_utilities::ProcessEdges<XImage1DType>(recorder(sink), {5, 1, 1});
    REQUIRE(sink == VisitedType{{0, 0, 1}, {0, 0, 2}, {0, 0, 3}});
  }
}

TEST_CASE("Simplnx::NeighborUtilities: ProcessFaces visits the expected indices", "[Simplnx][NeighborUtilities]")
{
  using VisitedType = std::vector<std::tuple<int64, int64, int64>>;
  auto recorder = [](VisitedType& sink) { return [&](int64 z, int64 y, int64 x, const std::vector<FaceNeighborType>& /*faces*/) { sink.emplace_back(z, y, x); }; };

  SECTION("EmptyZImage2D 4x3x1 visits the XY interior (length 1)")
  {
    VisitedType sink;
    image_dimensional_utilities::ProcessFaces<EmptyZImage2DType>(recorder(sink), {4, 3, 1});
    // Interior: x in [1, 2], y = 1. Expected invocations:
    REQUIRE(sink == VisitedType{{0, 1, 1}, {0, 1, 2}});
  }
  SECTION("Image3D 3x3x3 visits all 6 faces of the cube")
  {
    VisitedType sink;
    image_dimensional_utilities::ProcessFaces<Image3DType>(recorder(sink), {3, 3, 3});
    // Each face has 1 interior voxel ((y=1,x=1) etc.); six faces total.
    REQUIRE(sink.size() == 6);
    std::set<std::tuple<int64, int64, int64>> uniq(sink.begin(), sink.end());
    REQUIRE(uniq == std::set<std::tuple<int64, int64, int64>>{{0, 1, 1}, {2, 1, 1}, {1, 0, 1}, {1, 2, 1}, {1, 1, 0}, {1, 1, 2}});
  }
}

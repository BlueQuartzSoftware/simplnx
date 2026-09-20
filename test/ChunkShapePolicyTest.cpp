#include "simplnx/Utilities/Parsing/HDF5/ChunkShapePolicy.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::HDF5;

// All expected shapes below are derived by hand from k_TargetChunkBytes = 1 MiB (1048576 bytes).
// suffixBytes[i] = (elementByteSize * numComponents) * product(dims[i+1..]); rowBytes for the
// PinSlowestDim regime is (elementByteSize * numComponents) * product(dims[firstRowDim..]).

TEST_CASE("ChunkShapePolicy: BundleOuterSlabs regime", "[ChunkShapePolicy]")
{
  const ChunkShapeOptions opts{k_TargetChunkBytes, ChunkShapeRegime::BundleOuterSlabs};

  SECTION("3D large slice (one slice >= target -> pin dim0, band dim1)")
  {
    // dims={64,2048,512}, elem=4, nc=1. suffix[0]=4*2048*512=4194304 >= 1MiB -> 1;
    // suffix[1]=4*512=2048 < 1MiB -> rows=1048576/2048=512, clamp 2048 -> 512.
    const ShapeType chunk = computeChunkShape({64, 2048, 512}, /*numComponents=*/1, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{1, 512, 512});
  }

  SECTION("3D small slice (whole slice < target -> bundle outer slabs)")
  {
    // dims={100,16,16}, elem=4, nc=1. suffix[0]=4*16*16=1024 < 1MiB ->
    // rows=1048576/1024=1024, clamp 100 -> 100. All 100 outer slabs bundled.
    const ShapeType chunk = computeChunkShape({100, 16, 16}, /*numComponents=*/1, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{100, 16, 16});
  }

  SECTION("2D outer row >= target (pin dim0, band inner)")
  {
    // dims={1000,524288}, elem=4, nc=1. suffix[0]=4*524288=2097152 >= 1MiB -> 1;
    // suffix[1]=4 < 1MiB -> rows=1048576/4=262144, clamp 524288 -> 262144.
    const ShapeType chunk = computeChunkShape({1000, 524288}, /*numComponents=*/1, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{1, 262144});
  }

  SECTION("1D (band the only dimension)")
  {
    // dims={1000000}, elem=4, nc=1. suffix[0]=4 < 1MiB ->
    // rows=1048576/4=262144, clamp 1000000 -> 262144.
    const ShapeType chunk = computeChunkShape({1000000}, /*numComponents=*/1, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{262144});
  }

  SECTION("4D full-dataspace dims with numComponents=1 (writer usage)")
  {
    // dims={64,512,512,3} (component dim folded into dims), elem=4, nc=1.
    // suffix[3]=4, suffix[2]=4*3=12, suffix[1]=12*512=6144, suffix[0]=6144*512=3145728 >= 1MiB -> 1;
    // suffix[1]=6144 < 1MiB -> rows=1048576/6144=170, clamp 512 -> 170.
    const ShapeType chunk = computeChunkShape({64, 512, 512, 3}, /*numComponents=*/1, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{1, 170, 512, 3});
  }

  SECTION("empty dims -> empty shape")
  {
    REQUIRE(computeChunkShape({}, /*numComponents=*/1, /*elementByteSize=*/4, opts).empty());
  }
}

TEST_CASE("ChunkShapePolicy: PinSlowestDim regime", "[ChunkShapePolicy]")
{
  const ChunkShapeOptions opts{k_TargetChunkBytes, ChunkShapeRegime::PinSlowestDim};

  SECTION("rank>=3 (pin dim0 to 1, band dim1)")
  {
    // dims={64,2048,512}, elem=4, nc=1. rowBytes(firstRowDim=2)=4*512=2048 ->
    // rows=1048576/2048=512, clamp dims[1]=2048 -> 512.
    const ShapeType chunk = computeChunkShape({64, 2048, 512}, /*numComponents=*/1, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{1, 512, 512});
  }

  SECTION("1D (band the slowest dimension)")
  {
    // dims={1000000}, elem=4, nc=1. rowBytes(firstRowDim=1)=4 ->
    // rows=1048576/4=262144, clamp dims[0]=1000000 -> 262144.
    const ShapeType chunk = computeChunkShape({1000000}, /*numComponents=*/1, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{262144});
  }

  SECTION("2D (band the slowest dimension, inner kept full)")
  {
    // dims={1000,512}, elem=4, nc=1. rowBytes(firstRowDim=1)=4*512=2048 ->
    // rows=1048576/2048=512, clamp dims[0]=1000 -> 512.
    const ShapeType chunk = computeChunkShape({1000, 512}, /*numComponents=*/1, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{512, 512});
  }

  SECTION("rank>=3 with numComponents>1 (component folding)")
  {
    // tupleShape={64,512,512}, componentShape product=3, elem=4 -> unitBytes=12.
    // rowBytes(firstRowDim=2)=12*512=6144 -> rows=1048576/6144=170, clamp 512 -> 170.
    const ShapeType chunk = computeChunkShape({64, 512, 512}, /*numComponents=*/3, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{1, 170, 512});
  }

  SECTION("2D with numComponents>1 (component folding)")
  {
    // tupleShape={1000,256}, componentShape product=4, elem=4 -> unitBytes=16.
    // rowBytes(firstRowDim=1)=16*256=4096 -> rows=1048576/4096=256, clamp 1000 -> 256.
    const ShapeType chunk = computeChunkShape({1000, 256}, /*numComponents=*/4, /*elementByteSize=*/4, opts);
    REQUIRE(chunk == ShapeType{256, 256});
  }

  SECTION("empty dims -> empty shape")
  {
    REQUIRE(computeChunkShape({}, /*numComponents=*/1, /*elementByteSize=*/4, opts).empty());
  }
}

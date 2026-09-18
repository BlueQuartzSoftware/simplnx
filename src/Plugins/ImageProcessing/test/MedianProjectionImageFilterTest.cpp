#include "ProjectionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MedianProjectionImageFilter.hpp"

#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKMedianProjectionImageFilter, created at runtime by UUID.
const Uuid k_LegacyMedianProjectionUuid = *Uuid::FromString("00e48f6b-8a00-414f-b3d9-49d48a3f9a00");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: the new filter must produce the EXACT output the legacy ITK Median Projection
//     filter produces, over ProjectionDimension in {0,1,2} x Perform-In-Place in {true,false}. A ramp
//     input (value == flat index) makes the per-axis median differ by axis, so a wrong axis mapping or a
//     wrong in-place/new-geometry output shape is caught. This is the correctness gate for the
//     axis-projection façade + engine + MedianReduce (the non-associative slab-staged path).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MedianProjectionImageFilter: Legacy parity grid (3D)", "[ImageProcessing][MedianProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MedianProjectionImageFilter, float32>(k_LegacyMedianProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<float32>(ds, 12, 0.0, 1.0); });
}

// -----------------------------------------------------------------------------
// (Type-dispatch breadth) uint8, int16, and uint16 input coverage for the projection façade's type dispatch (the grid
// above exercises only float32) through the non-associative slab-staged MedianReduce path. Bit-exact vs the
// legacy ITK Median filter; small dims keep the ramp within the integer type's range (uint8: 6^3 = 216 <= 255).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MedianProjectionImageFilter: Legacy parity grid (uint8 input)", "[ImageProcessing][MedianProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MedianProjectionImageFilter, uint8>(k_LegacyMedianProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<uint8>(ds, 6, 0.0, 1.0); });
}
TEST_CASE("ImageProcessing::MedianProjectionImageFilter: Legacy parity grid (int16 input)", "[ImageProcessing][MedianProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MedianProjectionImageFilter, int16>(k_LegacyMedianProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<int16>(ds, 8, 0.0, 1.0); });
}
TEST_CASE("ImageProcessing::MedianProjectionImageFilter: Legacy parity grid (uint16 input)", "[ImageProcessing][MedianProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MedianProjectionImageFilter, uint16>(k_LegacyMedianProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<uint16>(ds, 8, 0.0, 1.0); });
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden tests (plan Task 5) -- duplicate each ITKMedianProjectionImageTest.cpp case on
// OUR ITK-free filter: (A) output md5 == ITK's committed hash + (B) live-ITK bit-exact parity. Type-preserving;
// the projected median selects an existing sample, so it is bit-exact.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MedianProjectionImageFilter: ITK real-image golden (default in-place)", "[ImageProcessing][ItkGolden][MedianProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MedianProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/true, "86de48c070480cb9809e28715f6e70e1");
}
TEST_CASE("ImageProcessing::MedianProjectionImageFilter: ITK real-image golden (new geometry)", "[ImageProcessing][ItkGolden][MedianProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MedianProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/false, "86de48c070480cb9809e28715f6e70e1");
}
TEST_CASE("ImageProcessing::MedianProjectionImageFilter: ITK real-image golden (dimensional)", "[ImageProcessing][ItkGolden][MedianProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MedianProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/2, /*removeOriginalGeometry=*/true, "0990f0f6c63ea9d63b701ed7c2467de7");
}
TEST_CASE("ImageProcessing::MedianProjectionImageFilter: ITK real-image golden (short)", "[ImageProcessing][ItkGolden][MedianProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MedianProjectionImageFilter>("Ramp-Up-Short.nrrd", /*projDim=*/1, /*removeOriginalGeometry=*/true, "9fcc7164f3294811cbf2d875b0e494d1");
}

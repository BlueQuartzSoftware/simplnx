#include "ProjectionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MinimumProjectionImageFilter.hpp"

#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKMinimumProjectionImageFilter, created at runtime by UUID.
const Uuid k_LegacyMinimumProjectionUuid = *Uuid::FromString("86898336-8680-4c4e-b166-3f8de9e3d4f2");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: the new filter must produce the EXACT output the legacy ITK Minimum Projection
//     filter produces, over ProjectionDimension in {0,1,2} x Perform-In-Place in {true,false}. A ramp
//     input (value == flat index) makes the per-axis minimum differ by axis, so a wrong axis mapping or a
//     wrong in-place/new-geometry output shape is caught. This is the correctness gate for the
//     axis-projection façade + engine + MinReduce.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinimumProjectionImageFilter: Legacy parity grid (3D)", "[ImageProcessing][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MinimumProjectionImageFilter, float32>(k_LegacyMinimumProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<float32>(ds, 12, 0.0, 1.0); });
}

// -----------------------------------------------------------------------------
// (Type-dispatch breadth) uint8 and uint16 input coverage for the projection façade's type dispatch (the grid
// above exercises only float32). Bit-exact vs the legacy ITK Minimum filter; small dims keep the ramp within the
// integer type's range so values stay distinct (uint8: 6^3 = 216 <= 255).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinimumProjectionImageFilter: Legacy parity grid (uint8 input)", "[ImageProcessing][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MinimumProjectionImageFilter, uint8>(k_LegacyMinimumProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<uint8>(ds, 6, 0.0, 1.0); });
}
TEST_CASE("ImageProcessing::MinimumProjectionImageFilter: Legacy parity grid (uint16 input)", "[ImageProcessing][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MinimumProjectionImageFilter, uint16>(k_LegacyMinimumProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<uint16>(ds, 8, 0.0, 1.0); });
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden tests (plan Task 5) -- duplicate each ITKMinimumProjectionImageTest.cpp case on
// OUR ITK-free filter: (A) output md5 == ITK's committed hash + (B) live-ITK bit-exact parity. Type-preserving.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinimumProjectionImageFilter: ITK real-image golden (default in-place)", "[ImageProcessing][ItkGolden][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MinimumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/true, "5591e0307db733396e8cc8143e7f29f7");
}
TEST_CASE("ImageProcessing::MinimumProjectionImageFilter: ITK real-image golden (new geometry)", "[ImageProcessing][ItkGolden][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MinimumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/false, "5591e0307db733396e8cc8143e7f29f7");
}
TEST_CASE("ImageProcessing::MinimumProjectionImageFilter: ITK real-image golden (dimensional)", "[ImageProcessing][ItkGolden][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MinimumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/2, /*removeOriginalGeometry=*/true, "6c16b87a823ca190294ac8b678ba4300");
}
TEST_CASE("ImageProcessing::MinimumProjectionImageFilter: ITK real-image golden (short)", "[ImageProcessing][ItkGolden][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MinimumProjectionImageFilter>("Ramp-Up-Short.nrrd", /*projDim=*/1, /*removeOriginalGeometry=*/true, "c4d83f61ffd5cc3a163155bb5d6a0698");
}

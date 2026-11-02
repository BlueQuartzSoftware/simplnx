#include "ProjectionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/MinimumProjectionImageFilter.hpp"

#include <memory>

using namespace nx::core;

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden tests (plan Task 5) -- duplicate each ITKMinimumProjectionImageTest.cpp case on
// OUR ITK-free filter: the output md5 must equal ITK's committed hash. The filter is type-preserving.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::MinimumProjectionImageFilter: ITK real-image golden (default in-place)", "[NXImageProcessing][ItkGolden][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MinimumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/true, "5591e0307db733396e8cc8143e7f29f7");
}
TEST_CASE("NXImageProcessing::MinimumProjectionImageFilter: ITK real-image golden (new geometry)", "[NXImageProcessing][ItkGolden][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MinimumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/false, "5591e0307db733396e8cc8143e7f29f7");
}
TEST_CASE("NXImageProcessing::MinimumProjectionImageFilter: ITK real-image golden (dimensional)", "[NXImageProcessing][ItkGolden][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MinimumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/2, /*removeOriginalGeometry=*/true, "6c16b87a823ca190294ac8b678ba4300");
}
TEST_CASE("NXImageProcessing::MinimumProjectionImageFilter: ITK real-image golden (short)", "[NXImageProcessing][ItkGolden][MinimumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MinimumProjectionImageFilter>("Ramp-Up-Short.nrrd", /*projDim=*/1, /*removeOriginalGeometry=*/true, "c4d83f61ffd5cc3a163155bb5d6a0698");
}

#include "ProjectionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MedianProjectionImageFilter.hpp"

#include <memory>

using namespace nx::core;

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden tests (plan Task 5) -- duplicate each ITKMedianProjectionImageTest.cpp case on
// OUR ITK-free filter: the output md5 must equal ITK's committed hash. The filter is type-preserving and the
// projected median selects an existing sample, so the hash is bit-exact.
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

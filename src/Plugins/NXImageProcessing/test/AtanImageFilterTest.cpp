#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/AtanImageFilter.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAtanImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::AtanImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][AtanImageFilter]")
{
  ip_test::CheckItkRampBaseline(ip_test::RunPointwiseFilter<AtanImageFilter>, "BasicFilters_AtanImageFilter_defaults.nrrd", 0.01);
}

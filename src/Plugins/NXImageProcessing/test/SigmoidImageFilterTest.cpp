#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/SigmoidImageFilter.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKSigmoidImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR SigmoidImageFilter with the ITK case's params
// (defaults -- Alpha/Beta/OutputMin/OutputMax unset), then compare the output md5 with ITK's committed hash.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SigmoidImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][SigmoidImageFilter]")
{
  ip_test::CheckItkRampMd5(ip_test::RunPointwiseFilter<SigmoidImageFilter>, "c9222b9c9150e0d7a07e9de184c10167");
}

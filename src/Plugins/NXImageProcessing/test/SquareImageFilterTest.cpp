#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/SquareImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): the op is { double d = x; return static_cast<U>(d*d); }.
// For x = 1e20 (representable in float32), d*d = 1e40 which exceeds FLT_MAX (~3.4e38), so the narrowing cast to
// float32 yields +inf. ITK's Sqr functor squares the same way, so this pins the observed overflow to +inf.
TEST_CASE("NXImageProcessing::SquareImageFilter: square near sqrt(FLT_MAX) overflows to +inf (domain edge)", "[NXImageProcessing][SquareImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {1.0e20f});
  const DataPath output = ip_test::RunPointwiseFilter<SquareImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // (1e20)^2 = 1e40 > FLT_MAX -> +inf
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKSquareImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR SquareImageFilter (defaults), then our output's
// md5 must EQUAL ITK's committed hash. A match confirms the durable golden byte-for-byte (no re-freeze needed).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SquareImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][SquareImageFilter]")
{
  ip_test::CheckItkRampMd5(ip_test::RunPointwiseFilter<SquareImageFilter>, "314065b457b66e102b9cafd7c49be6b3");
}

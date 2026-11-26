#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ExpNegativeImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): the op is static_cast<U>(std::exp(-static_cast<double>(x))).
// For a large NEGATIVE input, -x is large positive, so exp(-x) overflows double (>~709.78) to +inf, and casting
// +inf to float32 stays +inf. ITK's ExpNegative functor is the same expression, so this pins the +inf overflow.
TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: exp(-x) of a large negative input overflows to +inf (domain edge)", "[NXImageProcessing][ExpNegativeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-1000.0f});
  const DataPath output = ip_test::RunPointwiseFilter<ExpNegativeImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // exp(-(-1000)) = exp(1000) = +inf
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKExpNegativeImageTest.cpp(defaults):
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR filter (defaults), then compare with the
// committed baseline @0.01. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][ExpNegativeImageFilter]")
{
  ip_test::CheckItkRampBaseline(ip_test::RunPointwiseFilter<ExpNegativeImageFilter>, "BasicFilters_ExpNegativeImageFilter_defaults.nrrd", 0.01);
}

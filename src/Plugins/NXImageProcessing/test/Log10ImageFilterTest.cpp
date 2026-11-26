#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/Log10ImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): the op is static_cast<U>(std::log10(static_cast<double>(x)))
// -- ITK's equally-unguarded Log10 functor. log10(x<0) is NaN and log10(0) is -inf (IEEE pole). This test pins both.
TEST_CASE("NXImageProcessing::Log10ImageFilter: log10 of negative is NaN and log10(0) is -inf (domain edge)", "[NXImageProcessing][Log10ImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-2.0f, 0.0f});
  const DataPath output = ip_test::RunPointwiseFilter<Log10ImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0))); // log10(-2) -> NaN
  REQUIRE(std::isinf(out.getValue(1))); // log10(0)  -> -inf
  REQUIRE(out.getValue(1) < 0.0f);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKLog10ImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// log10(0) at the ramp's zero produces -inf; ip_golden::CompareImages mirrors ITK's own max-abs-diff comparator, which
// tolerates the -inf tail. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::Log10ImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][Log10ImageFilter]")
{
  ip_test::CheckItkRampBaseline(ip_test::RunPointwiseFilter<Log10ImageFilter>, "BasicFilters_Log10ImageFilter_defaults.nrrd", 0.01);
}

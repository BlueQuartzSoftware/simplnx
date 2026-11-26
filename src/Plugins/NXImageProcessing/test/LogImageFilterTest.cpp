#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/LogImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): the op is static_cast<U>(std::log(static_cast<double>(x)))
// -- ITK's equally-unguarded Log functor. log(x<0) is NaN and log(0) is -inf (IEEE pole). This test pins both. (The
// golden test only tolerates the -inf tail, it does not assert it explicitly.)
TEST_CASE("NXImageProcessing::LogImageFilter: log of negative is NaN and log(0) is -inf (domain edge)", "[NXImageProcessing][LogImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-2.0f, 0.0f});
  const DataPath output = ip_test::RunPointwiseFilter<LogImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0))); // log(-2) -> NaN
  REQUIRE(std::isinf(out.getValue(1))); // log(0)  -> -inf
  REQUIRE(out.getValue(1) < 0.0f);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKLogImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// log(0) at the ramp's zero produces -inf; ip_golden::CompareImages mirrors ITK's own max-abs-diff comparator, which
// tolerates the -inf tail. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::LogImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][LogImageFilter]")
{
  ip_test::CheckItkRampBaseline(ip_test::RunPointwiseFilter<LogImageFilter>, "BasicFilters_LogImageFilter_defaults.nrrd", 0.01);
}

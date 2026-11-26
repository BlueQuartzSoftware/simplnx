#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/AsinImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): asin is undefined for |x|>1. The op is
// static_cast<U>(std::asin(static_cast<double>(x))) -- ITK's equally-unguarded Asin functor -- so asin(2) is NaN.
TEST_CASE("NXImageProcessing::AsinImageFilter: asin of |x|>1 is NaN (domain edge)", "[NXImageProcessing][AsinImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {2.0f});
  const DataPath output = ip_test::RunPointwiseFilter<AsinImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAsinImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01 via
// ip_golden::CompareImages. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::AsinImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][AsinImageFilter]")
{
  ip_test::CheckItkRampBaseline(ip_test::RunPointwiseFilter<AsinImageFilter>, "BasicFilters_AsinImageFilter_defaults.nrrd", 0.01);
}

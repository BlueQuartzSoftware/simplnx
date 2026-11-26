#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/SqrtImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): sqrt is undefined for x<0. The op is
// static_cast<U>(std::sqrt(static_cast<double>(x))) -- ITK's equally-unguarded Sqrt functor -- so sqrt(-4) is NaN.
TEST_CASE("NXImageProcessing::SqrtImageFilter: sqrt of a negative is NaN (domain edge)", "[NXImageProcessing][SqrtImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-4.0f});
  const DataPath output = ip_test::RunPointwiseFilter<SqrtImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKSqrtImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SqrtImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][SqrtImageFilter]")
{
  ip_test::CheckItkRampBaseline(ip_test::RunPointwiseFilter<SqrtImageFilter>, "BasicFilters_SqrtImageFilter_defaults.nrrd", 0.01);
}

#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ExpImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): the op is static_cast<U>(std::exp(static_cast<double>(x))).
// exp overflows double above ~709.78, so exp(1000) is +inf and casting +inf to float32 stays +inf. ITK's Exp
// functor is the same expression, so this pins the observed +inf overflow.
TEST_CASE("NXImageProcessing::ExpImageFilter: exp of a large input overflows to +inf (domain edge)", "[NXImageProcessing][ExpImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {1000.0f});
  const DataPath output = ip_test::RunPointwiseFilter<ExpImageFilter>(ds, input);

  const auto& out = ds.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // +inf
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKExpImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ExpImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][ExpImageFilter]")
{
  ip_test::CheckItkRampBaseline(ip_test::RunPointwiseFilter<ExpImageFilter>, "BasicFilters_ExpImageFilter_defaults.nrrd", 0.01);
}

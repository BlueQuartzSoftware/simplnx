#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/AsinImageFilter.hpp"

using namespace nx::core;

// PART A domain edge (documents ITK-faithful behavior): asin is undefined for |x|>1. The op is
// static_cast<U>(std::asin(static_cast<double>(x))) -- ITK's equally-unguarded Asin functor -- so asin(2) is NaN.
TEST_CASE("NXImageProcessing::AsinImageFilter: asin of |x|>1 is NaN (domain edge)", "[NXImageProcessing][AsinImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(dataStructure, {2.0f});
  const DataPath output = ip_test::RunPointwiseFilter<AsinImageFilter>(dataStructure, input);

  const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAsinImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01 via
// ip_golden::CompareImages. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::AsinImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][AsinImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure dataStructure;
  const auto readInput = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath("Ramp-Zero-One-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(AsinImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(AsinImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AsinImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  AsinImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath("BasicFilters_AsinImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(dataStructure, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

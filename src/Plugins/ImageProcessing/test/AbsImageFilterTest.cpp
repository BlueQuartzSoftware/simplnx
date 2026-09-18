#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/AbsImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>

using namespace nx::core;

TEST_CASE("ImageProcessing::AbsImageFilter: Legacy parity", "[ImageProcessing][AbsImageFilter]")
{
  ip_test::RunLegacyParity<AbsImageFilter, float32>(*Uuid::FromString("e9dd12bc-f7fa-4ba2-98b0-fec3326bf620"), [](Arguments&) {}, 12, -500.0, 1.0);
}

// PART B float64 dispatch coverage: mirror the float32 parity with a float64 ramp so the otherwise-uninstantiated
// float64 dispatch arm of AllNumeric is covered and matches live ITK. (Abs's float branch is
// static_cast<U>(std::abs(static_cast<double>(x))), same as ITK for a floating pixel type.)
TEST_CASE("ImageProcessing::AbsImageFilter: Legacy parity (float64)", "[ImageProcessing][AbsImageFilter]")
{
  ip_test::RunLegacyParity<AbsImageFilter, float64>(*Uuid::FromString("e9dd12bc-f7fa-4ba2-98b0-fec3326bf620"), [](Arguments&) {}, 12, -500.0, 1.0);
}

// PART C multi-component (vector) input: the pointwise filters call PreflightImageFilter with requireScalar=false,
// so a MULTI-COMPONENT input is ACCEPTED and every component is transformed independently -- ApplyPointwise streams
// the flat store whose length is tuples*components, applying the op to each value. This asserts that shared
// requireScalar=false multi-component path once via Abs; it is identical for ALL the pointwise filters.
TEST_CASE("ImageProcessing::AbsImageFilter: multi-component (vector) input is processed per-component", "[ImageProcessing][AbsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({2, 1, 1}); // 2 cells
  const ShapeType cellShape = {1, 1, 2};
  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, cellShape, {3});
  auto* inArray = DataArray<float32>::Create(ds, "Input", store, cellAM->getId());
  auto& inRef = inArray->getDataStoreRef();
  // 2 tuples x 3 components = 6 values, mixed signs so abs is non-trivial on every component.
  const std::array<float32, 6> inputValues = {-1.0f, 2.0f, -3.0f, 4.0f, -5.0f, 6.0f};
  for(usize i = 0; i < inputValues.size(); ++i)
  {
    inRef.setValue(i, inputValues[i]);
  }
  REQUIRE(inArray->getNumberOfComponents() == 3);

  const DataPath outputPath = ip_test::RunPointwiseFilter<AbsImageFilter>(ds, inputPath);

  const auto& outArray = ds.getDataRefAs<DataArray<float32>>(outputPath);
  REQUIRE(outArray.getNumberOfComponents() == 3); // component shape preserved
  const auto& outRef = outArray.getDataStoreRef();
  for(usize i = 0; i < inputValues.size(); ++i)
  {
    REQUIRE(outRef.getValue(i) == std::abs(inputValues[i]));
  }
}

// Regression for the int64 precision loss: -(2^53 + 1) is representable in int64 but NOT in float64 (it rounds
// to -(2^53)), so the pre-fix `static_cast<U>(std::abs(static_cast<double>(value)))` returned 2^53 (off by one).
// The integer-domain abs returns the exact 2^53 + 1.
TEST_CASE("ImageProcessing::AbsImageFilter: int64 abs is exact in the integer domain", "[ImageProcessing][AbsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr int64 k_Neg = -9007199254740993LL;     // -(2^53 + 1)
  constexpr int64 k_Expected = 9007199254740993LL; // 2^53 + 1 (unrepresentable in float64)

  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({2, 1, 1});
  const ShapeType cellShape = {1, 1, 2};
  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<int64>(ds, inputPath, cellShape, {1});
  auto* inArray = DataArray<int64>::Create(ds, "Input", store, cellAM->getId());
  inArray->getDataStoreRef().setValue(0, k_Neg);
  inArray->getDataStoreRef().setValue(1, int64{-5});

  AbsImageFilter filter;
  Arguments args;
  args.insertOrAssign(AbsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(AbsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(AbsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int64>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getValue(0) == k_Expected); // exact, NOT 2^53
  REQUIRE(outStore.getValue(1) == int64{5});
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the two ITK-sourced real-image baseline golden cases below. Mirrors ITKAbsImageTest.cpp:
// read the ITK case's input (float32 or int16) through OUR ITK-free reader, run OUR AbsImageFilter (SameAsInput,
// no params), then:
//   (A) DURABLE golden -- compare our output to ITK's committed baseline .nrrd (SAME reader) at tol 0.01 via
//       ip_golden::CompareImages (permanent, survives ITK removal).
//   (B) LIVE-ITK parity -- run the legacy ITK filter on the SAME input into the SAME ds and compare via
//       ip_golden::CompareImages at @p parityTol (0 for the integer case -> bit-exact; 1e-4 for the float case).
// Pinned ForceInCore: the legacy ITK filter bad_casts an OOC store (reading via OUR reader is OOC-safe).
// -----------------------------------------------------------------------------
void RunAbsItkGolden(const std::string& inputFile, const std::string& baselineFile, float64 parityTol)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(AbsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(AbsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AbsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  AbsImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds.
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<AbsImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(AbsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, parityTol);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

TEST_CASE("ImageProcessing::AbsImageFilter: ITK real-image golden (float)", "[ImageProcessing][ItkGolden][AbsImageFilter]")
{
  RunAbsItkGolden("RA-Slice-Float.nrrd", "BasicFilters_AbsImageFilter_float.nrrd", /*parityTol=*/1.0e-4);
}

TEST_CASE("ImageProcessing::AbsImageFilter: ITK real-image golden (short)", "[ImageProcessing][ItkGolden][AbsImageFilter]")
{
  RunAbsItkGolden("RA-Slice-Short.nrrd", "BasicFilters_AbsImageFilter_short.nrrd", /*parityTol=*/0.0);
}

// -----------------------------------------------------------------------------
// Representative exercise of the SHARED PreflightImageFilter geometry-mismatch branch (k_GeometryMismatch,
// -8001) used by EVERY ImageProcessing filter: when the input array's tuple count does not equal the Image
// Geometry's cell count, preflight must fail. Asserted once here via Abs (the simplest single-impl pointwise
// filter, whose preflightImpl calls the shared facade directly); the branch is identical for all filters.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::AbsImageFilter: Preflight rejects an array whose tuple count != geometry cell count", "[ImageProcessing][AbsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({4, 4, 4}); // 64 cells

  // Input array with a DELIBERATELY MISMATCHED tuple count (8 tuples != 64 cells), held in its own top-level
  // AttributeMatrix (NOT wired as the geometry's cell data), so the array's tuple count and the geometry's cell
  // count differ and the shared PreflightImageFilter geometry check is the sole violation.
  const ShapeType mismatchedShape = {8};
  auto* am = AttributeMatrix::Create(ds, "Data", mismatchedShape);
  const DataPath inputPath({"Data", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, mismatchedShape, {1});
  DataArray<float32>::Create(ds, "Input", store, am->getId());

  AbsImageFilter filter;
  Arguments args;
  args.insertOrAssign(AbsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(AbsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(AbsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_GeometryMismatch);
}

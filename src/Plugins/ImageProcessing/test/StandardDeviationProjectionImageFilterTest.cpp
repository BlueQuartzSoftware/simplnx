#include "ProjectionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/StandardDeviationProjectionImageFilter.hpp"

#include <nonstd/span.hpp>

#include <cmath>
#include <memory>

using namespace nx::core;

namespace
{
// Tolerance for the Float64 standard-deviation output. This reducer computes the standard deviation with
// the one-pass algebraic identity (sumSq - sum^2/N)/(N-1) instead of ITK's two-pass sum of squared
// deviations; the two are only ALGEBRAICALLY equal, so a slightly looser tolerance than the exact-integer
// Sum/Mean is used. It is still ~4 orders of magnitude tighter than the SAMPLE-vs-POPULATION difference
// (a factor of sqrt(N/(N-1))), so the test still fails hard if the (N-1) sample convention were wrong.
constexpr float64 k_StdDevAbsTol = 1.0e-4;
constexpr float64 k_StdDevRelTol = 1.0e-7;

// Closed-form SAMPLE standard deviation of the ramp value(x,y,z) = z*D^2 + y*D + x along the projected axis.
// Each column is an arithmetic sequence {a, a+d, ..., a+(D-1)d} whose sample standard deviation is
// |d| * sqrt(D*(D+1)/12), INDEPENDENT of the starting value a -- so it is constant across all slots on a
// given axis; only the common difference d (1, D, or D^2) changes between axes. The legacy ITK Standard
// Deviation Projection filter is NOT registered at runtime (commented out of the ITKImageProcessing
// FilterList), so this independent closed form is the correctness oracle instead of legacy parity.
float64 StdDevExpectedForSlot(uint32 projDim, usize /*slot*/, usize dim)
{
  const auto Dd = static_cast<float64>(dim);
  const float64 baseStdDev = std::sqrt(Dd * (Dd + 1.0) / 12.0);
  float64 commonDiff = 1.0; // projDim == 0 -> vary x -> stride 1
  if(projDim == 1)
  {
    commonDiff = Dd; // vary y -> stride D
  }
  else if(projDim == 2)
  {
    commonDiff = Dd * Dd; // vary z -> stride D^2
  }
  return commonDiff * baseStdDev;
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Computed-expected correctness grid (in-core). The legacy ITK Standard Deviation Projection filter is
//     not registered at runtime, so correctness is gated on the independent closed form (the sample standard
//     deviation of a ramp column), over ProjectionDimension in {0,1,2} x Perform-In-Place in {true,false}.
//     A ramp input makes each column an arithmetic sequence with a per-axis common difference, so the
//     per-axis standard deviation differs by axis, and BOTH output modes of the shared façade are exercised.
//     The output element type is Float64. This is the correctness gate for the axis-projection façade
//     (AllNumeric + AlwaysFloat64) + engine + StdDevReduce, including its SAMPLE (divide-by-(N-1)) variance
//     convention lifted from itkStandardDeviationProjectionImageFilter.h.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::StandardDeviationProjectionImageFilter: Computed-expected grid (3D)", "[ImageProcessing][StandardDeviationProjectionImageFilter]")
{
  constexpr usize D = 12;
  projection_test::RunProjectionComputedExpectedGrid<StandardDeviationProjectionImageFilter>([](DataStructure& ds) { return ip_test::BuildRampImage<float32>(ds, D, 0.0, 1.0); },
                                                                                             [](uint32 projDim, usize slot) { return StdDevExpectedForSlot(projDim, slot, D); }, k_StdDevAbsTol,
                                                                                             k_StdDevRelTol);
}

// -----------------------------------------------------------------------------
// (1b) Type-dispatch breadth: an INTEGER (int16) input case for the projection façade's type dispatch (the grid
//      above exercises only float32). value == flat index holds for any element type, so the same closed-form
//      sample-standard-deviation oracle applies; the reducer accumulates in Float64. Small dim keeps the ramp
//      within int16 range.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::StandardDeviationProjectionImageFilter: Computed-expected grid (int16 input)", "[ImageProcessing][StandardDeviationProjectionImageFilter]")
{
  constexpr usize D = 8;
  projection_test::RunProjectionComputedExpectedGrid<StandardDeviationProjectionImageFilter>([](DataStructure& ds) { return ip_test::BuildRampImage<int16>(ds, D, 0.0, 1.0); },
                                                                                             [](uint32 projDim, usize slot) { return StdDevExpectedForSlot(projDim, slot, D); }, k_StdDevAbsTol,
                                                                                             k_StdDevRelTol);
}

// -----------------------------------------------------------------------------
// (2) Reducer edges (StdDevReduce). Two branches the ramp grids never reach:
//     - the N <= 1 guard, when a projected column has a single sample (projected axis extent 1);
//     - the variance < 0 clamp, when a column is constant (catastrophic cancellation can make the algebraic
//       one-pass variance a tiny negative before the clamp).
//     Both must produce a finite, exact 0 standard deviation.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::StandardDeviationProjectionImageFilter: single-sample column (N<=1 guard) -> 0", "[ImageProcessing][StandardDeviationProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize D = 6;
  // Image dims are XYZ; make the projected axis (Z) have extent 1, so every projected column is a single sample.
  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({D, D, 1});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  const ShapeType cellShape = {1, D, D}; // tuple shape is ZYX
  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<float32>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();
  for(usize i = 0; i < ref.getSize(); ++i)
  {
    ref.setValue(i, static_cast<float32>(i + 1)); // arbitrary distinct values; single-sample columns -> stddev 0
  }

  StandardDeviationProjectionImageFilter filter;
  Arguments args;
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{2})); // project Z (extent 1)
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Projected Image", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<float64>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == D * D);
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(std::isfinite(outStore.getValue(i)));
    REQUIRE(outStore.getValue(i) == 0.0);
  }
}

TEST_CASE("ImageProcessing::StandardDeviationProjectionImageFilter: constant column (variance clamp) -> 0", "[ImageProcessing][StandardDeviationProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize D = 8;
  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, D, 42.0, 0.0); // step 0 -> globally constant image (every column constant)

  StandardDeviationProjectionImageFilter filter;
  Arguments args;
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{1}));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Projected Image", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<float64>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == D * D);
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(std::isfinite(outStore.getValue(i)));
    REQUIRE(std::abs(outStore.getValue(i)) <= 1.0e-9);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden test (plan Task 5) -- duplicates
// ITKStandardDeviationProjectionImageTest.cpp(z_projection): RA-Float.nrrd, projection dimension 2 (Z), compared
// against the stored Float64 baseline (double, 64x64x1) at tol 1e-4 (A). (A)-only: the legacy NX
// ITKStandardDeviationProjectionImageFilter is commented out of the ITKImageProcessing FilterList
// (createFilter -> null / -9002), so no live-ITK (B) parity is available -- recorded (not an error).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::StandardDeviationProjectionImageFilter: ITK real-image golden (z_projection)", "[ImageProcessing][ItkGolden][StandardDeviationProjectionImageFilter]")
{
  projection_test::RunProjectionBaselineItkGolden<StandardDeviationProjectionImageFilter>("RA-Float.nrrd", "BasicFilters_StandardDeviationProjectionImageFilter_z_projection.nrrd", /*projDim=*/2,
                                                                                          /*tolerance=*/0.0001, projection_test::LiveItkParity::OptionalRecordIfMissing);
}

#include "ProjectionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/StandardDeviationProjectionImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/AxisProjectionEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ProjectionReducers.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <cmath>
#include <memory>
#include <type_traits>

using namespace nx::core;

namespace
{
// Welford's update and the closed-form oracle can differ through rounding. These tolerances remain
// much smaller than the difference between sample and population standard deviation.
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

TEMPLATE_TEST_CASE("ImageProcessing::StandardDeviationProjectionImageFilter: large-offset sample deviation", "[ImageProcessing][StandardDeviationProjectionImageFilter][StdDevStability]", float64,
                   float32, int32)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const uint32 projDim = GENERATE(0u, 1u, 2u);
  CAPTURE(projDim);

  // For {a, a+d, a+2d}, the mean is a+d and sample variance is (d*d + 0 + d*d)/(3-1) = d*d.
  // Float32 spacing at 1e9 is 64; using increments of 1 would round all three inputs to the same value.
  constexpr float64 k_Step = std::is_same_v<TestType, float32> ? 64.0 : 1.0;
  const std::array<TestType, 3> values = {static_cast<TestType>(1.0e9), static_cast<TestType>(1.0e9 + k_Step), static_cast<TestType>(1.0e9 + 2.0 * k_Step)};
  SizeVec3 dims = {1, 1, 1};
  dims[projDim] = 3;
  const ShapeType cellShape = {dims[2], dims[1], dims[0]};
  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions(dims);
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<TestType>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<TestType>::Create(ds, "Input", store, cellAM->getId());
  REQUIRE(array != nullptr);
  const auto writeResult = store->copyFromBuffer(0, nonstd::span<const TestType>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  StandardDeviationProjectionImageFilter filter;
  Arguments args;
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(projDim));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  // ApplyAxisProjection selects by store type and has no AlgorithmTestScope force flags or path witness.
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Projected Image", "CellData", "Output"});
  REQUIRE_NOTHROW(ds.getDataRefAs<Float64Array>(outputPath));
  const auto& output = ds.getDataRefAs<Float64Array>(outputPath);
  REQUIRE(output.getNumberOfTuples() == 1);
  REQUIRE(output[0] == Approx(k_Step).margin(1.0e-12));
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("ImageProcessing::StandardDeviationProjectionImageFilter: large-offset staged and bounded deviation", "[ImageProcessing][StandardDeviationProjectionImageFilter][StdDevStability]")
{
  UnitTest::LoadPlugins();
  namespace projection_detail = nx::core::ImageProcessing::detail;
  using nx::core::ImageProcessing::StdDevReduce;
  const usize projDim = GENERATE(0, 1, 2);
  CAPTURE(projDim);
  SizeVec3 dims = {1, 1, 1};
  dims[projDim] = 3;
  DataStore<float64> input({dims[2], dims[1], dims[0]}, {1}, 0.0);
  DataStore<float64> output({1, 1, 1}, {1}, 0.0);
  const std::array<float64, 3> values = {1.0e9, 1.0e9 + 1.0, 1.0e9 + 2.0};
  const auto writeResult = input.copyFromBuffer(0, nonstd::span<const float64>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  const auto shape = projection_detail::ValidateAxisProjection(input, output, dims, projDim);
  REQUIRE(shape.valid());
  const std::atomic_bool shouldCancel = false;

  // Call the existing engine entry points directly: these routes cannot be forced by AlgorithmTestScope.
  // In-memory buffers exercise the algorithms here, not a real disk-backed store.
  SECTION("resident slab staging")
  {
    const auto result = projection_detail::ApplyAxisProjectionResident(input, output, shape.value(), projDim, StdDevReduce{}, shouldCancel, IFilter::MessageHandler{});
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(output[0] == Approx(1.0).margin(1.0e-12)); // deviations {-1, 0, 1}: sqrt(2/(3-1)) = 1
  }
  if(projDim < 2)
  {
    SECTION("bounded true-2-D rows")
    {
      const auto result = projection_detail::ApplyAxisProjection2D(input, output, shape.value(), projDim, StdDevReduce{}, shouldCancel, projection_detail::k_AxisProjection2DTargetBytes);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      REQUIRE(output[0] == Approx(1.0).margin(1.0e-12));
    }
  }
}

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
// (2) Reducer edges (StdDevReduce). Two cases the ramp grids never reach:
//     - the N <= 1 guard, when a projected column has a single sample (projected axis extent 1);
//     - zero variance, when every value in a column is the same.
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

TEST_CASE("ImageProcessing::StandardDeviationProjectionImageFilter: constant column -> 0", "[ImageProcessing][StandardDeviationProjectionImageFilter]")
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
                                                                                          /*tolerance=*/0.0001);
}

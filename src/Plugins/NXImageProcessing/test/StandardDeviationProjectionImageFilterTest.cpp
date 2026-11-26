#include "ProjectionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/StandardDeviationProjectionImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
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
  const auto dd = static_cast<float64>(dim);
  const float64 baseStdDev = std::sqrt(dd * (dd + 1.0) / 12.0);
  float64 commonDiff = 1.0; // projDim == 0 -> vary x -> stride 1
  if(projDim == 1)
  {
    commonDiff = dd; // vary y -> stride D
  }
  else if(projDim == 2)
  {
    commonDiff = dd * dd; // vary z -> stride D^2
  }
  return commonDiff * baseStdDev;
}
} // namespace

TEMPLATE_TEST_CASE("NXImageProcessing::StandardDeviationProjectionImageFilter: large-offset sample deviation", "[NXImageProcessing][StandardDeviationProjectionImageFilter][StdDevStability]", float64,
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
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions(dims);
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<TestType>(dataStructure, inputPath, cellShape, {1});
  auto* arrayPtr = DataArray<TestType>::Create(dataStructure, "Input", store, cellAmPtr->getId());
  REQUIRE(arrayPtr != nullptr);
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
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  // ApplyAxisProjection selects by store type and has no AlgorithmTestScope force flags or path witness.
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Projected Image", "CellData", "Output"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(outputPath));
  const auto& output = dataStructure.getDataRefAs<Float64Array>(outputPath);
  REQUIRE(output.getNumberOfTuples() == 1);
  REQUIRE(output[0] == Approx(k_Step).margin(1.0e-12));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::StandardDeviationProjectionImageFilter: large-offset staged and bounded deviation", "[NXImageProcessing][StandardDeviationProjectionImageFilter][StdDevStability]")
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
TEST_CASE("NXImageProcessing::StandardDeviationProjectionImageFilter: Computed-expected grid (3D)", "[NXImageProcessing][StandardDeviationProjectionImageFilter]")
{
  constexpr usize k_D = 12;
  projection_test::RunProjectionComputedExpectedGrid<StandardDeviationProjectionImageFilter>(
      [](DataStructure& dataStructure) { return ip_test::BuildRampImage<float32>(dataStructure, k_D, 0.0, 1.0); }, [](uint32 projDim, usize slot) { return StdDevExpectedForSlot(projDim, slot, k_D); },
      k_StdDevAbsTol, k_StdDevRelTol);
}

// -----------------------------------------------------------------------------
// (1b) Type-dispatch breadth: an INTEGER (int16) input case for the projection façade's type dispatch (the grid
//      above exercises only float32). value == flat index holds for any element type, so the same closed-form
//      sample-standard-deviation oracle applies; the reducer accumulates in Float64. Small dim keeps the ramp
//      within int16 range.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::StandardDeviationProjectionImageFilter: Computed-expected grid (int16 input)", "[NXImageProcessing][StandardDeviationProjectionImageFilter]")
{
  constexpr usize k_D = 8;
  projection_test::RunProjectionComputedExpectedGrid<StandardDeviationProjectionImageFilter>([](DataStructure& dataStructure) { return ip_test::BuildRampImage<int16>(dataStructure, k_D, 0.0, 1.0); },
                                                                                             [](uint32 projDim, usize slot) { return StdDevExpectedForSlot(projDim, slot, k_D); }, k_StdDevAbsTol,
                                                                                             k_StdDevRelTol);
}

// -----------------------------------------------------------------------------
// (2) Reducer edges (StdDevReduce). Two cases the ramp grids never reach:
//     - the N <= 1 guard, when a projected column has a single sample (projected axis extent 1);
//     - zero variance, when every value in a column is the same.
//     Both must produce a finite, exact 0 standard deviation.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::StandardDeviationProjectionImageFilter: single-sample column (N<=1 guard) -> 0", "[NXImageProcessing][StandardDeviationProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_D = 6;
  // Image dims are XYZ; make the projected axis (Z) have extent 1, so every projected column is a single sample.
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions({k_D, k_D, 1});
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());
  const ShapeType cellShape = {1, k_D, k_D}; // tuple shape is ZYX
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(dataStructure, inputPath, cellShape, {1});
  auto* arrayPtr = DataArray<float32>::Create(dataStructure, "Input", store, cellAmPtr->getId());
  auto& ref = arrayPtr->getDataStoreRef();
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
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Projected Image", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<float64>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == k_D * k_D);
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(std::isfinite(outStore.getValue(i)));
    REQUIRE(outStore.getValue(i) == 0.0);
  }
}

TEST_CASE("NXImageProcessing::StandardDeviationProjectionImageFilter: constant column (variance clamp) -> 0", "[NXImageProcessing][StandardDeviationProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_D = 8;
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(dataStructure, k_D, 42.0, 0.0); // step 0 -> globally constant image (every column constant)

  StandardDeviationProjectionImageFilter filter;
  Arguments args;
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{1}));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Projected Image", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<float64>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == k_D * k_D);
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
TEST_CASE("NXImageProcessing::StandardDeviationProjectionImageFilter: ITK real-image golden (z_projection)", "[NXImageProcessing][ItkGolden][StandardDeviationProjectionImageFilter]")
{
  projection_test::RunProjectionBaselineItkGolden<StandardDeviationProjectionImageFilter>("RA-Float.nrrd", "BasicFilters_StandardDeviationProjectionImageFilter_z_projection.nrrd", /*projDim=*/2,
                                                                                          /*tolerance=*/0.0001);
}

TEST_CASE("NXImageProcessing::StandardDeviationProjectionImageFilter: Warning names discarded objects", "[NXImageProcessing][StandardDeviationProjectionImageFilter]")
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  // No extra cell array, an extra one, and an old array with the output name (also discarded in place).
  const std::string extraName = GENERATE(std::string(""), std::string("Extra"), std::string("Output"));
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(dataStructure, 3, 0.0, 1.0);
  const DataPath geomPath({"Image Geometry"});
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  const DataPath featurePath = geomPath.createChildPath("Cell Feature Data");
  REQUIRE(AttributeMatrix::Create(dataStructure, "Cell Feature Data", {2}, imageGeom.getId()) != nullptr);
  const DataPath extraPath = inputPath.getParent().createChildPath(extraName.empty() ? "Extra" : extraName);
  if(!extraName.empty())
  {
    auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(inputPath.getParent());
    auto store = DataStoreUtilities::CreateDataStore<int16>(dataStructure, extraPath, cellAM.getShape(), {1});
    REQUIRE(Int16Array::Create(dataStructure, extraName, store, cellAM.getId()) != nullptr);
  }
  StandardDeviationProjectionImageFilter filter;
  Arguments args;
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(StandardDeviationProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  std::string discardedWarning;
  for(const auto& warning : preflightResult.outputActions.warnings())
  {
    if(warning.code == -8343)
    {
      discardedWarning += warning.message;
    }
  }
  // The geometry swap deletes the original children. The warning excludes the consumed input,
  // but must name the feature matrix even when it is the only other discarded object, using full paths.
  CHECK(discardedWarning.find(featurePath.toString()) != std::string::npos);
  CHECK(discardedWarning.find(inputPath.toString()) == std::string::npos);
  if(!extraName.empty())
  {
    CHECK(discardedWarning.find(extraPath.toString()) != std::string::npos);
  }
  // Axis projection dispatches by store type; it has no Direct/Scanline override.
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  CHECK(dataStructure.getData(featurePath) == nullptr);
  if(extraName != "Output")
  {
    CHECK(dataStructure.getData(extraPath) == nullptr);
  }
  REQUIRE(dataStructure.getDataRefAs<ImageGeom>(geomPath).getDimensions() == SizeVec3{1, 3, 3});
  const auto& output = dataStructure.getDataRefAs<Float64Array>(inputPath.getParent().createChildPath("Output"));
  REQUIRE(output.getNumberOfTuples() == 9);
  for(usize slot = 0; slot < 9; ++slot)
  {
    // X columns are {3*s, 3*s+1, 3*s+2}: sample variance = (1+0+1)/(3-1) = 1.
    CHECK(output[slot] == Approx(1.0).margin(1.0e-12));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

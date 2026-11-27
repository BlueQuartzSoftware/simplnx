#include "MorphologyFilterTestUtils.hpp"

#include "ObjectMorphologyFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/DilateObjectMorphologyImageFilter.hpp"

#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <array>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

using namespace nx::core;
namespace omt = object_morph_test;
using nx::core::ImageProcessing::KernelType;

namespace
{
// Fresh error/warning codes for this filter (grep-verified unused across src/: -8520 block).
constexpr int32 k_NonFiniteObjectValue = -8521;
constexpr int32 k_ObjectValueOutOfRange = -8522;
constexpr int32 k_ObjectValueTruncated = -8523;

// The standard live-parity configurations: 3D uniform + 3D non-cubic radius on a multi-object/edge-touching
// image, plus a genuinely 2D (Z=1) image. Annulus is excluded from live parity (empty legacy kernel).
const std::vector<omt::ObjCase> k_ParityConfigs = {{.label = "3D r{1,1,1}", .radius = {1, 1, 1}, .dim_x = 12, .dim_y = 12, .dim_z = 12},
                                                   {.label = "3D r{2,1,1}", .radius = {2, 1, 1}, .dim_x = 12, .dim_y = 12, .dim_z = 12},
                                                   {.label = "2D Z=1 r{2,2,0}", .radius = {2, 2, 0}, .dim_x = 20, .dim_y = 16, .dim_z = 1}};
} // namespace

// -----------------------------------------------------------------------------
// (1) Computed-expected correctness grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 2D Z=1 r{2,2,0}} on a
//     multi-object, edge-touching binary object image, for ObjectValue=1 (uint8) and ObjectValue=7 (int32). Each
//     output is compared to the INDEPENDENT scatter oracle over MakeStructuringElement(kernel, radius).
//
//     This was formerly a LIVE-ITK parity grid, but the legacy multi-threaded ITK ObjectMorphology filter is
//     NONDETERMINISTIC even for Dilate: each thread guard-copies input->output (`if(out != ObjectValue) out = in`)
//     and then paints boundary neighborhoods into the SHARED output with no cross-thread synchronization, so a
//     paint into a neighbor thread's region that lands between that thread's guarded load and store is clobbered
//     back to background -- observed dropping ~1 boundary voxel in ~8% of runs. The prior assumption that "Dilate
//     is immune" (the copy guard preserves ObjectValue paints) is FALSE: that guard is a non-atomic
//     read-modify-write, so the concurrent paint is a genuine data race. Our single-threaded scatter is
//     deterministic and matches ITK's intended single-threaded semantics, so -- as with Erode (see context-notes
//     7a-F) -- we validate against the computed-expected oracle rather than the unstable live-ITK golden.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: Computed-expected grid (Ball/Box/Cross)", "[NXImageProcessing][DilateObjectMorphologyImageFilter]")
{
  constexpr auto k_Dilate = ImageProcessing::ObjectMorphOp::Dilate;
  const std::vector<std::pair<const char*, KernelType>> kernels = {{"Ball", KernelType::Ball}, {"Box", KernelType::Box}, {"Cross", KernelType::Cross}};

  SECTION("ObjectValue=1 (uint8)")
  {
    const auto build = [](DataStructure& dataStructure, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<uint8>(dataStructure, dx, dy, dz, uint8{1}, uint8{0}); };
    const auto setParams = omt::DilateParamSetter<DilateObjectMorphologyImageFilter>(1.0);
    for(const auto& kernel : kernels)
    {
      for(const omt::ObjCase& cfg : k_ParityConfigs)
      {
        DYNAMIC_SECTION("kernel=" << kernel.first << " " << cfg.label)
        {
          const std::array<int32, 3> radius = {static_cast<int32>(cfg.radius[0]), static_cast<int32>(cfg.radius[1]), static_cast<int32>(cfg.radius[2])};
          omt::RunObjectMorphologyComputedExpected<DilateObjectMorphologyImageFilter, uint8>(build, cfg.dim_x, cfg.dim_y, cfg.dim_z, kernel.second, radius, k_Dilate, uint8{1}, uint8{0}, setParams);
        }
      }
    }
  }
  SECTION("ObjectValue=7 (non-default, int32 input)")
  {
    const auto build = [](DataStructure& dataStructure, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<int32>(dataStructure, dx, dy, dz, int32{7}, int32{0}); };
    const auto setParams = omt::DilateParamSetter<DilateObjectMorphologyImageFilter>(7.0);
    for(const auto& kernel : kernels)
    {
      for(const omt::ObjCase& cfg : k_ParityConfigs)
      {
        DYNAMIC_SECTION("kernel=" << kernel.first << " " << cfg.label)
        {
          const std::array<int32, 3> radius = {static_cast<int32>(cfg.radius[0]), static_cast<int32>(cfg.radius[1]), static_cast<int32>(cfg.radius[2])};
          omt::RunObjectMorphologyComputedExpected<DilateObjectMorphologyImageFilter, int32>(build, cfg.dim_x, cfg.dim_y, cfg.dim_z, kernel.second, radius, k_Dilate, int32{7}, int32{0}, setParams);
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
// (2) Annulus computed-expected (NOT live parity): this filter ships ITK's real thickness-1 Annulus shell,
//     whereas the legacy SimpleITK Annulus kernel was empty (a wrapper bug). Validated against the
//     independent scatter oracle over MakeStructuringElement(Annulus, radius).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: Annulus computed-expected", "[NXImageProcessing][DilateObjectMorphologyImageFilter]")
{
  const auto build = [](DataStructure& dataStructure, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<uint8>(dataStructure, dx, dy, dz, uint8{1}, uint8{0}); };
  const auto setParams = omt::DilateParamSetter<DilateObjectMorphologyImageFilter>(1.0);
  constexpr auto k_Dilate = ImageProcessing::ObjectMorphOp::Dilate;

  SECTION("Annulus 3D r{1,1,1}")
  {
    omt::RunObjectMorphologyComputedExpected<DilateObjectMorphologyImageFilter, uint8>(build, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, k_Dilate, uint8{1}, uint8{0}, setParams);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    omt::RunObjectMorphologyComputedExpected<DilateObjectMorphologyImageFilter, uint8>(build, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, k_Dilate, uint8{1}, uint8{0}, setParams);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    omt::RunObjectMorphologyComputedExpected<DilateObjectMorphologyImageFilter, uint8>(build, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, k_Dilate, uint8{1}, uint8{0}, setParams);
  }
}

// -----------------------------------------------------------------------------
// (3) float32 input: object morphology uses an EXACT-equality boundary test, so an exactly-representable
//     Object Value (3.0f) must behave identically to an integer image. Validated computed-expected (the
//     legacy float path is not the parity target here).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: float32 exact-equality object value", "[NXImageProcessing][DilateObjectMorphologyImageFilter]")
{
  const auto build = [](DataStructure& dataStructure, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<float32>(dataStructure, dx, dy, dz, 3.0f, 0.0f); };
  const auto setParams = omt::DilateParamSetter<DilateObjectMorphologyImageFilter>(3.0);
  omt::RunObjectMorphologyComputedExpected<DilateObjectMorphologyImageFilter, float32>(build, 12, 12, 12, KernelType::Ball, {2, 2, 2}, ImageProcessing::ObjectMorphOp::Dilate, 3.0f, 0.0f, setParams);
}

// -----------------------------------------------------------------------------
// (4) Both-algorithm-paths computed-expected on a constructed solid block. The object-morphology façade routes
//     through ApplyObjectMorphology's DispatchAlgorithm, so the AlgorithmTestScope runs the filter under BOTH the
//     ITK-faithful in-core Scatter path and the out-of-core streamed Gather path on in-memory stores (selected by
//     SIMPLNX_TEST_ALGORITHM_PATH). The legacy ITK filter rejects out-of-core arrays and is NOT run; instead the
//     output is compared to the independent scatter oracle computed over the actual block. Real-disk out-of-core
//     coverage is retired to the generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: Computed-expected (both algorithm paths)", "[NXImageProcessing][DilateObjectMorphologyImageFilter]")
{
  unit_test::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  constexpr usize k_D = 32;
  constexpr uint8 k_ObjectValue = 1;
  constexpr uint8 k_BackgroundValue = 0;
  constexpr usize k_Lo = 8;
  constexpr usize k_Hi = 23; // solid object cube [8,23]^3 (interior block, does not touch the image edge)

  // Build the block pattern once in memory (bulk-fill the array from it; keep it for the oracle).
  std::vector<uint8> input(k_D * k_D * k_D, k_BackgroundValue);
  for(usize z = k_Lo; z <= k_Hi; ++z)
  {
    for(usize y = k_Lo; y <= k_Hi; ++y)
    {
      for(usize x = k_Lo; x <= k_Hi; ++x)
      {
        input[omt::FlatIndex(x, y, z, k_D, k_D)] = k_ObjectValue;
      }
    }
  }

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions({k_D, k_D, k_D});
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());
  const ShapeType cellShape = {k_D, k_D, k_D};
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = data_store_utilities::CreateDataStore<uint8>(dataStructure, inputPath, cellShape, {1});
  auto* inputArrayPtr = DataArray<uint8>::Create(dataStructure, "Input", store, cellAmPtr->getId());
  {
    constexpr usize k_ChunkValues = 65536;
    auto& ref = inputArrayPtr->getDataStoreRef();
    for(usize start = 0; start < input.size(); start += k_ChunkValues)
    {
      const usize count = std::min(k_ChunkValues, input.size() - start);
      Result<> writeResult = ref.copyFromBuffer(start, nonstd::span<const uint8>(input.data() + start, count));
      SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    }
  }
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  DilateObjectMorphologyImageFilter filter;
  Arguments args;
  args.insertOrAssign(DilateObjectMorphologyImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(DilateObjectMorphologyImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(DilateObjectMorphologyImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(DilateObjectMorphologyImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(KernelType::Ball)));
  args.insertOrAssign(DilateObjectMorphologyImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(std::vector<uint32>{2, 2, 2}));
  args.insertOrAssign(DilateObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(static_cast<float64>(k_ObjectValue)));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<uint8>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = k_D * k_D * k_D;
  REQUIRE(outStore.getSize() == numSlots);

  const std::vector<uint8> expected = omt::ObjectMorphologyOracle<uint8>(input, k_D, k_D, k_D, KernelType::Ball, {2, 2, 2}, ImageProcessing::ObjectMorphOp::Dilate, k_ObjectValue, k_BackgroundValue);

  // Stream the output in bounded chunks and compare.
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<uint8[]>(k_ChunkValues);
  bool allMatch = true;
  usize badSlot = 0;
  for(usize start = 0; start < numSlots && allMatch; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, numSlots - start);
    Result<> copyResult = outStore.copyIntoBuffer(start, nonstd::span<uint8>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
    for(usize i = 0; i < count; ++i)
    {
      if(buffer[i] != expected[start + i])
      {
        allMatch = false;
        badSlot = start + i;
        break;
      }
    }
  }
  if(!allMatch)
  {
    UNSCOPED_INFO("slot=" << badSlot << " got=" << static_cast<int>(buffer[badSlot % k_ChunkValues]) << " expected=" << static_cast<int>(expected[badSlot]));
  }
  REQUIRE(allMatch);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// (5) Preflight cast-guard: the Float64 Object Value is cast to the input element type. For an integer input a
//     non-finite value or an out-of-range value is an undefined conversion (error), and a fractional value is
//     truncated (warning). For a float input only a non-finite value is rejected. In-range values preflight
//     cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: Preflight cast-guard on Object Value", "[NXImageProcessing][DilateObjectMorphologyImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto makeArgs = [](const DataPath& inputPath, float64 objectValue) {
    Arguments args;
    args.insertOrAssign(DilateObjectMorphologyImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(DilateObjectMorphologyImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(DilateObjectMorphologyImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(DilateObjectMorphologyImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(KernelType::Ball)));
    args.insertOrAssign(DilateObjectMorphologyImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(std::vector<uint32>{1, 1, 1}));
    args.insertOrAssign(DilateObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(objectValue));
    return args;
  };

  DilateObjectMorphologyImageFilter filter;

  SECTION("integer input (uint8)")
  {
    DataStructure dataStructure;
    const DataPath inputPath = omt::BuildObjectImage<uint8>(dataStructure, 8, 8, 8, uint8{1}, uint8{0}); // range [0, 255]

    // Out of range (> 255) -> undefined cast -> error.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 99999.0));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_ObjectValueOutOfRange);
    }
    // Non-finite (NaN) -> error.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, std::numeric_limits<float64>::quiet_NaN()));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_NonFiniteObjectValue);
    }
    // Fractional (1.5) -> truncation warning, still valid.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 1.5));
      SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
      REQUIRE_FALSE(result.output_actions.warnings().empty());
      REQUIRE(result.output_actions.warnings().front().code == k_ObjectValueTruncated);
    }
    // In-range integer value -> clean.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 1.0));
      SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
      REQUIRE(result.output_actions.warnings().empty());
    }
  }

  SECTION("float input (float32)")
  {
    DataStructure dataStructure;
    const DataPath inputPath = omt::BuildObjectImage<float32>(dataStructure, 8, 8, 8, 1.0f, 0.0f);

    // Non-finite (Inf) -> error (would break the exact-equality object test).
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, std::numeric_limits<float64>::infinity()));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_NonFiniteObjectValue);
    }
    // Finite but out of float32 range (1e300 > FLT_MAX): the cast to float32 would overflow to +inf and the
    // exact-equality object test would then match nothing -> reject as out-of-range rather than run silently.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 1e300));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_ObjectValueOutOfRange);
    }
    // A fractional but finite value is representable in a float image -> clean, no truncation warning.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 3.5));
      SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
      REQUIRE(result.output_actions.warnings().empty());
    }
    // An exactly-representable value -> clean.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 3.0));
      SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
      REQUIRE(result.output_actions.warnings().empty());
    }
  }

  // Regression for the 64-bit boundary hole in ValidateOneValueInRange: (float64)INT64_MAX rounds UP to 2^63 and
  // (float64)UINT64_MAX to 2^64, so the old `truncated > (float64)max<T>` test ACCEPTED exactly 2^63 / 2^64 and
  // the downstream static_cast<T> was undefined behavior. The exclusive power-of-two bound must reject them.
  SECTION("int64 input: 2^63 boundary is rejected, in-range value is clean")
  {
    DataStructure dataStructure;
    const DataPath inputPath = omt::BuildObjectImage<int64>(dataStructure, 8, 8, 8, int64{1}, int64{0});

    // Exactly 2^63 (== (float64)INT64_MAX): NOT representable as int64 -> undefined cast -> must error.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 9223372036854775808.0));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_ObjectValueOutOfRange);
    }
    // A large but representable value (< 2^63) -> clean (no false rejection at the extreme).
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 9.0e18));
      SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
      REQUIRE(result.output_actions.warnings().empty());
    }
  }

  SECTION("uint64 input: 2^64 boundary is rejected, in-range value is clean")
  {
    DataStructure dataStructure;
    const DataPath inputPath = omt::BuildObjectImage<uint64>(dataStructure, 8, 8, 8, uint64{1}, uint64{0});

    // Exactly 2^64 (== (float64)UINT64_MAX): NOT representable as uint64 -> undefined cast -> must error.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 18446744073709551616.0));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_ObjectValueOutOfRange);
    }
    // A large but representable value (< 2^64) -> clean.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 1.8e19));
      SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
      REQUIRE(result.output_actions.warnings().empty());
    }
  }
}

// -----------------------------------------------------------------------------
// (6) ITK-sourced real-image golden -- duplicates ITKDilateObjectMorphologyImageTest.cpp(float / short):
//     RA-Slice-Float.nrrd / RA-Slice-Short.nrrd (64x64x1), Ball radius {1,1,1}, ITK default Object Value 1.
//     (B) DETERMINISTIC computed-expected (NOT live-ITK bit-exact -- itk::ObjectMorphologyImageFilter is a
//     nondeterministic multithreaded data race, so a fresh legacy run is not a stable oracle; see test (1)) +
//     (A) durable ITK-captured baseline compare @ 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: ITK real-image golden (float)", "[NXImageProcessing][ItkGolden][DilateObjectMorphologyImageFilter]")
{
  object_morph_test::RunObjectMorphologyItkGoldenBaseline<DilateObjectMorphologyImageFilter, float32>("RA-Slice-Float.nrrd", "BasicFilters_DilateObjectMorphologyImageFilter_float.nrrd",
                                                                                                      ImageProcessing::ObjectMorphOp::Dilate, 1.0f, 0.0f,
                                                                                                      object_morph_test::DilateParamSetter<DilateObjectMorphologyImageFilter>(1.0), 0.01);
}

TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: ITK real-image golden (short)", "[NXImageProcessing][ItkGolden][DilateObjectMorphologyImageFilter]")
{
  object_morph_test::RunObjectMorphologyItkGoldenBaseline<DilateObjectMorphologyImageFilter, int16>("RA-Slice-Short.nrrd", "BasicFilters_DilateObjectMorphologyImageFilter_short.nrrd",
                                                                                                    ImageProcessing::ObjectMorphOp::Dilate, int16{1}, int16{0},
                                                                                                    object_morph_test::DilateParamSetter<DilateObjectMorphologyImageFilter>(1.0), 0.01);
}

// -----------------------------------------------------------------------------
// The input selection requires a scalar component shape.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: Preflight rejects non-scalar (multi-component) input", "[NXImageProcessing][DilateObjectMorphologyImageFilter]")
{
  omt::RequirePreflightRejectsNonScalar<DilateObjectMorphologyImageFilter, uint8>(filter_parameter::constants::k_ValidateTupleShapeValue,
                                                                                  omt::DilateParamSetter<DilateObjectMorphologyImageFilter>(1.0));
}

TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: Kernel Radius representability", "[NXImageProcessing][DilateObjectMorphologyImageFilter]")
{
  morph_test::RequireKernelRadiusPreflight<DilateObjectMorphologyImageFilter>();
}

TEST_CASE("NXImageProcessing::DilateObjectMorphologyImageFilter: Ordinary Kernel Radius control", "[NXImageProcessing][DilateObjectMorphologyImageFilter]")
{
  morph_test::RequireOrdinaryKernelRadius<DilateObjectMorphologyImageFilter>({5, 5, 5});
}

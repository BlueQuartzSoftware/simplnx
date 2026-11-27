#include "MorphologyFilterTestUtils.hpp"

#include "ObjectMorphologyFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ErodeObjectMorphologyImageFilter.hpp"

#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace omt = object_morph_test;
using nx::core::ImageProcessing::KernelType;
using nx::core::ImageProcessing::ObjectMorphOp;

// =============================================================================
// WHY NO LIVE-LEGACY PARITY GRID FOR ERODE (unlike Dilate).
//
// The legacy ITK ErodeObjectMorphologyImageFilter is validated here against the INDEPENDENT scatter oracle
// (computed-expected), NOT against a live run of the ITK filter, because ITK's ObjectMorphology has a
// multi-threading artifact that makes its Erode output non-deterministic and divergent from the single-pass
// object-morphology semantics:
//
//   itkObjectMorphologyImageFilter::DynamicThreadedGenerateData() processes each threaded output sub-region
//   by (1) copying the input into the output for every pixel whose CURRENT output value != ObjectValue, then
//   (2) painting the boundary object pixels' structuring element into the output. A boundary pixel near a
//   region edge paints across into a NEIGHBORING region; if that neighbor region's copy step (1) runs AFTER
//   the paint, it REVERTS the painted pixel back to the input (because the painted value != ObjectValue).
//
//   - DILATE paints the ObjectValue, so the revert condition (value != ObjectValue) is FALSE for its paints
//     -> cross-region Dilate paints are preserved -> Dilate matches ITK exactly (see the Dilate parity grid).
//   - ERODE paints the BackgroundValue (!= ObjectValue), so cross-region Erode paints ARE reverted whenever
//     the sole painter of a pixel lies in another thread region. This drops paints in a thread-partition-
//     dependent way, empirically confirmed for both 3D and 2D inputs and for any BackgroundValue that differs
//     from the value the copy restores. It is therefore an ITK bug/artifact, not the intended semantics.
//
// The intended (single-pass) object-morphology semantics -- output = copy(input), then every boundary object
// pixel paints BackgroundValue over its structuring element -- are what this filter's engine implements. That
// is pinned deterministically by @ref object_morph_test::ObjectMorphologyOracle here (computed-expected) and
// additionally gated byte-for-byte between the in-core Scatter and OOC Gather engine paths in
// ObjectMorphologyEngineTest (including a BackgroundValue that collides with an existing label). Dilate's
// live-legacy parity (in DilateObjectMorphologyImageFilterTest) already confirms the shared boundary test +
// kernel rasterization against ITK, so the only Erode-specific behavior (the paint value) is fully covered by
// the deterministic oracle below.
// =============================================================================

namespace
{
// Fresh error/warning codes for this filter (grep-verified unused across src/: -8530 block).
constexpr int32 k_NonFiniteValue = -8531;
constexpr int32 k_ValueOutOfRange = -8532;
constexpr int32 k_ValueTruncated = -8533;

// The standard correctness configurations: 3D uniform + 3D non-cubic radius on a multi-object/edge-touching
// image, plus a genuinely 2D (Z=1) image.
const std::vector<omt::ObjCase> k_Configs = {{.label = "3D r{1,1,1}", .radius = {1, 1, 1}, .dim_x = 12, .dim_y = 12, .dim_z = 12},
                                             {.label = "3D r{2,1,1}", .radius = {2, 1, 1}, .dim_x = 12, .dim_y = 12, .dim_z = 12},
                                             {.label = "2D Z=1 r{2,2,0}", .radius = {2, 2, 0}, .dim_x = 20, .dim_y = 16, .dim_z = 1}};

const char* KernelName(KernelType kt)
{
  switch(kt)
  {
  case KernelType::Annulus:
    return "Annulus";
  case KernelType::Ball:
    return "Ball";
  case KernelType::Box:
    return "Box";
  case KernelType::Cross:
    return "Cross";
  }
  return "?";
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Computed-expected correctness grid: Ball/Box/Cross/Annulus x {3D r{1,1,1}, 3D r{2,1,1}, 2D Z=1 r{2,2,0}}
//     on a multi-object, edge-touching binary object image (ObjectValue=1, BackgroundValue=0). Each output is
//     compared to the INDEPENDENT scatter oracle over MakeStructuringElement(kernel, radius) -- the pinned
//     single-pass object-erosion semantics. All four kernels are covered (Annulus included, since it has no
//     live-legacy oracle anyway -- the legacy SimpleITK Annulus kernel is empty).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: Computed-expected grid (all kernels)", "[NXImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  const auto build = [](DataStructure& dataStructure, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<uint8>(dataStructure, dx, dy, dz, uint8{1}, uint8{0}); };
  const auto setParams = omt::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(1.0, 0.0);
  const std::vector<KernelType> kernels = {KernelType::Ball, KernelType::Box, KernelType::Cross, KernelType::Annulus};

  for(KernelType kt : kernels)
  {
    for(const omt::ObjCase& cfg : k_Configs)
    {
      DYNAMIC_SECTION("kernel=" << KernelName(kt) << " " << cfg.label)
      {
        const std::array<int32, 3> radius = {static_cast<int32>(cfg.radius[0]), static_cast<int32>(cfg.radius[1]), static_cast<int32>(cfg.radius[2])};
        omt::RunObjectMorphologyComputedExpected<ErodeObjectMorphologyImageFilter, uint8>(build, cfg.dim_x, cfg.dim_y, cfg.dim_z, kt, radius, ObjectMorphOp::Erode, uint8{1}, uint8{0}, setParams);
      }
    }
  }
}

// -----------------------------------------------------------------------------
// (2) Computed-expected varying BOTH ObjectValue and BackgroundValue, including a BackgroundValue that
//     COLLIDES with an existing non-object label. Object erosion must paint the Background Value over the
//     boundary shell regardless of whether that value already exists, and treat EVERY value != ObjectValue as
//     non-object. Input carries three labels {7 (object), 3 (existing label), 0 (background)}; eroded with
//     BackgroundValue=3 (colliding) and, on a two-label {7,0} image, BackgroundValue=9 (distinct).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: Computed-expected varying Object/Background Value", "[NXImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  const std::vector<KernelType> kernels = {KernelType::Ball, KernelType::Box, KernelType::Cross};
  const std::vector<std::array<int32, 3>> radii = {{1, 1, 1}, {2, 1, 1}};

  for(KernelType kt : kernels)
  {
    for(const std::array<int32, 3>& radius : radii)
    {
      DYNAMIC_SECTION("kernel=" << KernelName(kt) << " r{" << radius[0] << "," << radius[1] << "," << radius[2] << "}")
      {
        SECTION("ObjectValue=7 BackgroundValue=3 colliding with existing label 3")
        {
          const auto build = [](DataStructure& dataStructure, usize dx, usize dy, usize dz) {
            return omt::BuildObjectImageWithExtraLabel<int32>(dataStructure, dx, dy, dz, int32{7}, int32{3}, int32{0});
          };
          omt::RunObjectMorphologyComputedExpected<ErodeObjectMorphologyImageFilter, int32>(build, 12, 12, 12, kt, radius, ObjectMorphOp::Erode, int32{7}, int32{3},
                                                                                            omt::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(7.0, 3.0));
        }
        SECTION("ObjectValue=7 BackgroundValue=9 distinct")
        {
          const auto build = [](DataStructure& dataStructure, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<int32>(dataStructure, dx, dy, dz, int32{7}, int32{0}); };
          omt::RunObjectMorphologyComputedExpected<ErodeObjectMorphologyImageFilter, int32>(build, 12, 12, 12, kt, radius, ObjectMorphOp::Erode, int32{7}, int32{9},
                                                                                            omt::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(7.0, 9.0));
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
// (3) float32 input: object morphology uses an EXACT-equality boundary test, so an exactly-representable
//     Object Value (3.0f) must behave identically to an integer image. Validated computed-expected.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: float32 exact-equality object value", "[NXImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  const auto build = [](DataStructure& dataStructure, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<float32>(dataStructure, dx, dy, dz, 3.0f, 0.0f); };
  const auto setParams = omt::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(3.0, 0.0);
  omt::RunObjectMorphologyComputedExpected<ErodeObjectMorphologyImageFilter, float32>(build, 12, 12, 12, KernelType::Ball, {2, 2, 2}, ObjectMorphOp::Erode, 3.0f, 0.0f, setParams);
}

// -----------------------------------------------------------------------------
// (4) Both-algorithm-paths computed-expected on a constructed solid block. The object-morphology façade routes
//     through ApplyObjectMorphology's DispatchAlgorithm, so the AlgorithmTestScope runs the filter under BOTH the
//     ITK-faithful in-core Scatter path and the out-of-core streamed Gather path on in-memory stores (selected by
//     SIMPLNX_TEST_ALGORITHM_PATH). The output is compared to the independent scatter oracle over the actual
//     block. Real-disk out-of-core coverage is retired to the generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: Computed-expected (both algorithm paths)", "[NXImageProcessing][ErodeObjectMorphologyImageFilter]")
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

  ErodeObjectMorphologyImageFilter filter;
  Arguments args;
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(KernelType::Ball)));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(std::vector<uint32>{2, 2, 2}));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(static_cast<float64>(k_ObjectValue)));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_BackgroundValue_Key, std::make_any<float64>(static_cast<float64>(k_BackgroundValue)));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<uint8>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = k_D * k_D * k_D;
  REQUIRE(outStore.getSize() == numSlots);

  const std::vector<uint8> expected = omt::ObjectMorphologyOracle<uint8>(input, k_D, k_D, k_D, KernelType::Ball, {2, 2, 2}, ObjectMorphOp::Erode, k_ObjectValue, k_BackgroundValue);

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
// (5) Preflight cast-guard on BOTH Object Value and Background Value. Each Float64 value is cast to the input
//     element type: for an integer input a non-finite or out-of-range value is an undefined conversion
//     (error), and a fractional value is truncated (warning). For a float input a non-finite or magnitude
//     out-of-range value (e.g. 1e300 > FLT_MAX) is rejected; a finite representable value is clean. Preflight
//     validates Object Value first, returns early on the FIRST error, and accumulates truncation warnings.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: Preflight cast-guard on Object + Background Value", "[NXImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto makeArgs = [](const DataPath& inputPath, float64 objectValue, float64 backgroundValue) {
    Arguments args;
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(KernelType::Ball)));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(std::vector<uint32>{1, 1, 1}));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(objectValue));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
    return args;
  };

  ErodeObjectMorphologyImageFilter filter;

  SECTION("integer input (uint8)")
  {
    DataStructure dataStructure;
    const DataPath inputPath = omt::BuildObjectImage<uint8>(dataStructure, 8, 8, 8, uint8{1}, uint8{0}); // range [0, 255]

    // Object Value out of range -> error.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 99999.0, 0.0));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_ValueOutOfRange);
    }
    // Background Value out of range (negative) -> error (Object Value is in-range).
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 1.0, -5.0));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_ValueOutOfRange);
    }
    // Non-finite (NaN) Object Value -> error.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, std::numeric_limits<float64>::quiet_NaN(), 0.0));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_NonFiniteValue);
    }
    // Non-finite (Inf) Background Value -> error (Object Value is in-range).
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 1.0, std::numeric_limits<float64>::infinity()));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_NonFiniteValue);
    }
    // Both values fractional -> TWO truncation warnings, still valid (warnings from both are accumulated).
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 1.5, 0.5));
      SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
      REQUIRE(result.output_actions.warnings().size() == 2);
      REQUIRE(result.output_actions.warnings()[0].code == k_ValueTruncated);
      REQUIRE(result.output_actions.warnings()[1].code == k_ValueTruncated);
    }
    // Both in-range integer values -> clean.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 1.0, 0.0));
      SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
      REQUIRE(result.output_actions.warnings().empty());
    }
  }

  SECTION("float input (float32)")
  {
    DataStructure dataStructure;
    const DataPath inputPath = omt::BuildObjectImage<float32>(dataStructure, 8, 8, 8, 1.0f, 0.0f);

    // Object Value out of float32 range (1e300 > FLT_MAX) -> error.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 1e300, 0.0));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_ValueOutOfRange);
    }
    // Background Value out of float32 range -> error (Object Value is in-range).
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 3.0, 1e300));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_ValueOutOfRange);
    }
    // Non-finite (Inf) Object Value -> error.
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, std::numeric_limits<float64>::infinity(), 0.0));
      REQUIRE(result.output_actions.invalid());
      REQUIRE(result.output_actions.errors().front().code == k_NonFiniteValue);
    }
    // Both exactly-representable finite values -> clean (no truncation warning for float types).
    {
      auto result = filter.preflight(dataStructure, makeArgs(inputPath, 3.0, 0.0));
      SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
      REQUIRE(result.output_actions.warnings().empty());
    }
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKErodeObjectMorphologyImageTest.cpp(float / short):
// RA-Slice-Float.nrrd / RA-Slice-Short.nrrd (64x64x1), Ball radius {1,1,1}, ITK default Object Value 1 /
// Background Value 0. (B) DETERMINISTIC computed-expected (NOT live-ITK bit-exact -- itk::ObjectMorphologyImageFilter
// is a nondeterministic multithreaded data race, so a fresh legacy run is not a stable oracle) + (A) durable
// ITK-captured baseline compare @ 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: ITK real-image golden (float)", "[NXImageProcessing][ItkGolden][ErodeObjectMorphologyImageFilter]")
{
  object_morph_test::RunObjectMorphologyItkGoldenBaseline<ErodeObjectMorphologyImageFilter, float32>("RA-Slice-Float.nrrd", "BasicFilters_ErodeObjectMorphologyImageFilter_float.nrrd",
                                                                                                     ImageProcessing::ObjectMorphOp::Erode, 1.0f, 0.0f,
                                                                                                     object_morph_test::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(1.0, 0.0), 0.01);
}

TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: ITK real-image golden (short)", "[NXImageProcessing][ItkGolden][ErodeObjectMorphologyImageFilter]")
{
  object_morph_test::RunObjectMorphologyItkGoldenBaseline<ErodeObjectMorphologyImageFilter, int16>("RA-Slice-Short.nrrd", "BasicFilters_ErodeObjectMorphologyImageFilter_short.nrrd",
                                                                                                   ImageProcessing::ObjectMorphOp::Erode, int16{1}, int16{0},
                                                                                                   object_morph_test::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(1.0, 0.0), 0.01);
}

// -----------------------------------------------------------------------------
// The input selection requires a scalar component shape.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: Preflight rejects non-scalar (multi-component) input", "[NXImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  omt::RequirePreflightRejectsNonScalar<ErodeObjectMorphologyImageFilter, uint8>(filter_parameter::constants::k_ValidateTupleShapeValue,
                                                                                 omt::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(1.0, 0.0));
}

TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: Kernel Radius representability", "[NXImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  morph_test::RequireKernelRadiusPreflight<ErodeObjectMorphologyImageFilter>();
}

TEST_CASE("NXImageProcessing::ErodeObjectMorphologyImageFilter: Ordinary Kernel Radius control", "[NXImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  morph_test::RequireOrdinaryKernelRadius<ErodeObjectMorphologyImageFilter>({0, 0, 0});
}

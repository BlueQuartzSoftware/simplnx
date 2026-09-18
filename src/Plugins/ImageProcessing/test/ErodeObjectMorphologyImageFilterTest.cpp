#include "ObjectMorphologyFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ErodeObjectMorphologyImageFilter.hpp"

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
const std::vector<omt::ObjCase> k_Configs = {{"3D r{1,1,1}", {1, 1, 1}, 12, 12, 12}, {"3D r{2,1,1}", {2, 1, 1}, 12, 12, 12}, {"2D Z=1 r{2,2,0}", {2, 2, 0}, 20, 16, 1}};

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
TEST_CASE("ImageProcessing::ErodeObjectMorphologyImageFilter: Computed-expected grid (all kernels)", "[ImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<uint8>(ds, dx, dy, dz, uint8{1}, uint8{0}); };
  const auto setParams = omt::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(1.0, 0.0);
  const std::vector<KernelType> kernels = {KernelType::Ball, KernelType::Box, KernelType::Cross, KernelType::Annulus};

  for(KernelType kt : kernels)
  {
    for(const omt::ObjCase& cfg : k_Configs)
    {
      DYNAMIC_SECTION("kernel=" << KernelName(kt) << " " << cfg.label)
      {
        const std::array<int32, 3> radius = {static_cast<int32>(cfg.radius[0]), static_cast<int32>(cfg.radius[1]), static_cast<int32>(cfg.radius[2])};
        omt::RunObjectMorphologyComputedExpected<ErodeObjectMorphologyImageFilter, uint8>(build, cfg.dimX, cfg.dimY, cfg.dimZ, kt, radius, ObjectMorphOp::Erode, uint8{1}, uint8{0}, setParams);
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
TEST_CASE("ImageProcessing::ErodeObjectMorphologyImageFilter: Computed-expected varying Object/Background Value", "[ImageProcessing][ErodeObjectMorphologyImageFilter]")
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
          const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return omt::BuildObjectImageWithExtraLabel<int32>(ds, dx, dy, dz, int32{7}, int32{3}, int32{0}); };
          omt::RunObjectMorphologyComputedExpected<ErodeObjectMorphologyImageFilter, int32>(build, 12, 12, 12, kt, radius, ObjectMorphOp::Erode, int32{7}, int32{3},
                                                                                            omt::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(7.0, 3.0));
        }
        SECTION("ObjectValue=7 BackgroundValue=9 distinct")
        {
          const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<int32>(ds, dx, dy, dz, int32{7}, int32{0}); };
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
TEST_CASE("ImageProcessing::ErodeObjectMorphologyImageFilter: float32 exact-equality object value", "[ImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return omt::BuildObjectImage<float32>(ds, dx, dy, dz, 3.0f, 0.0f); };
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
TEST_CASE("ImageProcessing::ErodeObjectMorphologyImageFilter: Computed-expected (both algorithm paths)", "[ImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize D = 32;
  constexpr uint8 objectValue = 1;
  constexpr uint8 backgroundValue = 0;
  constexpr usize lo = 8;
  constexpr usize hi = 23; // solid object cube [8,23]^3 (interior block, does not touch the image edge)

  // Build the block pattern once in memory (bulk-fill the array from it; keep it for the oracle).
  std::vector<uint8> input(D * D * D, backgroundValue);
  for(usize z = lo; z <= hi; ++z)
  {
    for(usize y = lo; y <= hi; ++y)
    {
      for(usize x = lo; x <= hi; ++x)
      {
        input[omt::FlatIndex(x, y, z, D, D)] = objectValue;
      }
    }
  }

  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({D, D, D});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  const ShapeType cellShape = {D, D, D};
  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<uint8>(ds, inputPath, cellShape, {1});
  auto* inputArray = DataArray<uint8>::Create(ds, "Input", store, cellAM->getId());
  {
    constexpr usize k_ChunkValues = 65536;
    auto& ref = inputArray->getDataStoreRef();
    for(usize start = 0; start < input.size(); start += k_ChunkValues)
    {
      const usize count = std::min(k_ChunkValues, input.size() - start);
      Result<> writeResult = ref.copyFromBuffer(start, nonstd::span<const uint8>(input.data() + start, count));
      SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    }
  }
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  ErodeObjectMorphologyImageFilter filter;
  Arguments args;
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(KernelType::Ball)));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{2, 2, 2}));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(static_cast<float64>(objectValue)));
  args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_BackgroundValue_Key, std::make_any<float64>(static_cast<float64>(backgroundValue)));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<uint8>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = D * D * D;
  REQUIRE(outStore.getSize() == numSlots);

  const std::vector<uint8> expected = omt::ObjectMorphologyOracle<uint8>(input, D, D, D, KernelType::Ball, {2, 2, 2}, ObjectMorphOp::Erode, objectValue, backgroundValue);

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
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// (5) Preflight cast-guard on BOTH Object Value and Background Value. Each Float64 value is cast to the input
//     element type: for an integer input a non-finite or out-of-range value is an undefined conversion
//     (error), and a fractional value is truncated (warning). For a float input a non-finite or magnitude
//     out-of-range value (e.g. 1e300 > FLT_MAX) is rejected; a finite representable value is clean. Preflight
//     validates Object Value first, returns early on the FIRST error, and accumulates truncation warnings.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ErodeObjectMorphologyImageFilter: Preflight cast-guard on Object + Background Value", "[ImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto makeArgs = [](const DataPath& inputPath, float64 objectValue, float64 backgroundValue) {
    Arguments args;
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(KernelType::Ball)));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_ObjectValue_Key, std::make_any<float64>(objectValue));
    args.insertOrAssign(ErodeObjectMorphologyImageFilter::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
    return args;
  };

  ErodeObjectMorphologyImageFilter filter;

  SECTION("integer input (uint8)")
  {
    DataStructure ds;
    const DataPath inputPath = omt::BuildObjectImage<uint8>(ds, 8, 8, 8, uint8{1}, uint8{0}); // range [0, 255]

    // Object Value out of range -> error.
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, 99999.0, 0.0));
      REQUIRE(result.outputActions.invalid());
      REQUIRE(result.outputActions.errors().front().code == k_ValueOutOfRange);
    }
    // Background Value out of range (negative) -> error (Object Value is in-range).
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, 1.0, -5.0));
      REQUIRE(result.outputActions.invalid());
      REQUIRE(result.outputActions.errors().front().code == k_ValueOutOfRange);
    }
    // Non-finite (NaN) Object Value -> error.
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, std::numeric_limits<float64>::quiet_NaN(), 0.0));
      REQUIRE(result.outputActions.invalid());
      REQUIRE(result.outputActions.errors().front().code == k_NonFiniteValue);
    }
    // Non-finite (Inf) Background Value -> error (Object Value is in-range).
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, 1.0, std::numeric_limits<float64>::infinity()));
      REQUIRE(result.outputActions.invalid());
      REQUIRE(result.outputActions.errors().front().code == k_NonFiniteValue);
    }
    // Both values fractional -> TWO truncation warnings, still valid (warnings from both are accumulated).
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, 1.5, 0.5));
      SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
      REQUIRE(result.outputActions.warnings().size() == 2);
      REQUIRE(result.outputActions.warnings()[0].code == k_ValueTruncated);
      REQUIRE(result.outputActions.warnings()[1].code == k_ValueTruncated);
    }
    // Both in-range integer values -> clean.
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, 1.0, 0.0));
      SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
      REQUIRE(result.outputActions.warnings().empty());
    }
  }

  SECTION("float input (float32)")
  {
    DataStructure ds;
    const DataPath inputPath = omt::BuildObjectImage<float32>(ds, 8, 8, 8, 1.0f, 0.0f);

    // Object Value out of float32 range (1e300 > FLT_MAX) -> error.
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, 1e300, 0.0));
      REQUIRE(result.outputActions.invalid());
      REQUIRE(result.outputActions.errors().front().code == k_ValueOutOfRange);
    }
    // Background Value out of float32 range -> error (Object Value is in-range).
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, 3.0, 1e300));
      REQUIRE(result.outputActions.invalid());
      REQUIRE(result.outputActions.errors().front().code == k_ValueOutOfRange);
    }
    // Non-finite (Inf) Object Value -> error.
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, std::numeric_limits<float64>::infinity(), 0.0));
      REQUIRE(result.outputActions.invalid());
      REQUIRE(result.outputActions.errors().front().code == k_NonFiniteValue);
    }
    // Both exactly-representable finite values -> clean (no truncation warning for float types).
    {
      auto result = filter.preflight(ds, makeArgs(inputPath, 3.0, 0.0));
      SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
      REQUIRE(result.outputActions.warnings().empty());
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
TEST_CASE("ImageProcessing::ErodeObjectMorphologyImageFilter: ITK real-image golden (float)", "[ImageProcessing][ItkGolden][ErodeObjectMorphologyImageFilter]")
{
  object_morph_test::RunObjectMorphologyItkGoldenBaseline<ErodeObjectMorphologyImageFilter, float32>("RA-Slice-Float.nrrd", "BasicFilters_ErodeObjectMorphologyImageFilter_float.nrrd",
                                                                                                     ImageProcessing::ObjectMorphOp::Erode, 1.0f, 0.0f,
                                                                                                     object_morph_test::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(1.0, 0.0), 0.01);
}

TEST_CASE("ImageProcessing::ErodeObjectMorphologyImageFilter: ITK real-image golden (short)", "[ImageProcessing][ItkGolden][ErodeObjectMorphologyImageFilter]")
{
  object_morph_test::RunObjectMorphologyItkGoldenBaseline<ErodeObjectMorphologyImageFilter, int16>("RA-Slice-Short.nrrd", "BasicFilters_ErodeObjectMorphologyImageFilter_short.nrrd",
                                                                                                   ImageProcessing::ObjectMorphOp::Erode, int16{1}, int16{0},
                                                                                                   object_morph_test::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(1.0, 0.0), 0.01);
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (k_NonScalarInput == -8530) fires before the shared PreflightImageFilter, so a 3-component input is rejected.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ErodeObjectMorphologyImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][ErodeObjectMorphologyImageFilter]")
{
  omt::RequirePreflightRejectsNonScalar<ErodeObjectMorphologyImageFilter, uint8>(-8530, omt::ErodeParamSetter<ErodeObjectMorphologyImageFilter>(1.0, 0.0));
}

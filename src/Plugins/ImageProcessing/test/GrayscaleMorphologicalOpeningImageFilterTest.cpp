#include "MorphologyFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GrayscaleMorphologicalOpeningImageFilter.hpp"

#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKGrayscaleMorphologicalOpeningImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyGrayscaleOpeningUuid = *Uuid::FromString("54433e92-fb7d-40f4-95bf-eb3db76d5caa");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 3D r{2,1,0}, 2D Z=1 r{2,2,0}}. The new
//     filter must reproduce the legacy ITK Grayscale Morphological Opening output EXACTLY (erode then dilate;
//     both folds select existing values). Annulus is excluded (empty legacy kernel). The (Box, 3D r{2,1,0})
//     cell is excluded from LIVE parity -- the legacy decomposable-Box anchor path produces garbage for a
//     zero-radius axis on a 3D image -- and is instead gated by a Box r{2,1,0} computed-expected case in test
//     (2); Ball/Cross at r{2,1,0} DO match live legacy.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalOpeningImageFilter: Legacy parity grid (Ball/Box/Cross)", "[ImageProcessing][GrayscaleMorphologicalOpeningImageFilter]")
{
  morph_test::RunMorphologyParityGrid<GrayscaleMorphologicalOpeningImageFilter, float32>(
      k_LegacyGrayscaleOpeningUuid, [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); },
      morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalOpeningImageFilter>(/*safeBorder=*/true),
      [](std::string_view kernelName, const morph_test::MorphCase& cfg) { return kernelName == "Box" && std::string_view(cfg.label) == "3D r{2,1,0}"; });
}

// -----------------------------------------------------------------------------
// (2) Computed-expected (NOT legacy parity), independent oracle composed from the skip-OOB SE-gather folds
//     (opening == dilate(erode(in))). Driven with Safe Border == FALSE so the skip-OOB oracle applies; the
//     Safe Border == true (padded) convention is covered by the parity grid in (1). This validates:
//     - a standard non-degenerate kernel under Safe Border == false (Ball + Box), and
//     - Annulus: this filter ships ITK's real thickness-1 shell, whereas the legacy SimpleITK Annulus was empty.
//     - Box r{2,1,0}: the excluded parity cell from (1); the legacy ITK anchor path is broken for a zero-radius
//       axis on a 3D image, so the correct result is test-documented here rather than left silently absent.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalOpeningImageFilter: Computed-expected (safeBorder=false; Annulus + Box zero-radius)", "[ImageProcessing][GrayscaleMorphologicalOpeningImageFilter]")
{
  using morph_test::CompositeKind;
  using morph_test::KernelType;
  auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); };
  const auto setParams = morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalOpeningImageFilter>(/*safeBorder=*/false);
  constexpr auto kOpening = CompositeKind::Opening;

  SECTION("safeBorder=false Ball 3D r{1,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalOpeningImageFilter, float32>(build, 12, 12, 12, KernelType::Ball, {1, 1, 1}, kOpening, setParams);
  }
  SECTION("safeBorder=false Box 3D r{2,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalOpeningImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 1}, kOpening, setParams);
  }
  SECTION("Annulus 3D r{1,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalOpeningImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, kOpening, setParams);
  }
  SECTION("Annulus 3D r{2,2,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalOpeningImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {2, 2, 2}, kOpening, setParams);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalOpeningImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, kOpening, setParams);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalOpeningImageFilter, float32>(build, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, kOpening, setParams);
  }
  SECTION("Box 3D zero-radius r{2,1,0} (legacy anchor path broken)")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalOpeningImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 0}, kOpening, setParams);
  }
}

// -----------------------------------------------------------------------------
// (3) Computed-expected on a ramp (value == flat index) with a Box structuring element, run on in-memory stores
//     under BOTH algorithm paths (in-core Direct + OOC Scanline) via UnitTest::AlgorithmTestScope. This composite
//     routes through ApplyMorphology's DispatchAlgorithm, so it records an algorithm-path execution and the
//     scope's executeFilter witness is satisfied. The legacy ITK filter rejects out-of-core arrays and is NOT
//     run here; instead the closed form of a Box opening (erode then dilate) of a strictly-increasing ramp is
//     asserted directly. The erosion of a monotone ramp is again a monotone field whose value at each voxel is
//     the flat index of its -radius corner; the subsequent dilation reads that field at its +radius corner.
//     Composing the two (each clamped to the image) gives opening(x,y,z) = flat( max(min(x+rx,D-1)-rx,0), ... ),
//     i.e. the identity in the interior and a clamped shift near the boundary. (D^3 - 1 < 2^24, so float32 is
//     exact.) Real-disk out-of-core coverage is retired to the generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalOpeningImageFilter: Ramp opening computed-expected (both algorithm paths)", "[ImageProcessing][GrayscaleMorphologicalOpeningImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize D = 32;
  constexpr int64 rx = 2;
  constexpr int64 ry = 1;
  constexpr int64 rz = 1;

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, D, 0.0, 1.0);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  GrayscaleMorphologicalOpeningImageFilter filter;
  Arguments args;
  args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Box)));
  args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_KernelRadius_Key,
                      std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{static_cast<uint32>(rx), static_cast<uint32>(ry), static_cast<uint32>(rz)}));
  // Safe Border == false so the skip-OOB closed form below applies; this exercises the composite's OOC
  // streaming of the plain two-pass path on both algorithm iterations.
  args.insertOrAssign(GrayscaleMorphologicalOpeningImageFilter::k_SafeBorder_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<float32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = D * D * D;
  REQUIRE(outStore.getSize() == numSlots);

  auto expectedForSlot = [&](usize s) -> float32 {
    const int64 Di = static_cast<int64>(D);
    const int64 x = static_cast<int64>(s % D);
    const int64 y = static_cast<int64>((s / D) % D);
    const int64 z = static_cast<int64>(s / (D * D));
    // Opening = dilate(erode(ramp)): the erosion corner is the -radius corner, evaluated at the +radius
    // corner of the dilation (all clamped to the image).
    const int64 ox = std::max<int64>(std::min<int64>(x + rx, Di - 1) - rx, 0);
    const int64 oy = std::max<int64>(std::min<int64>(y + ry, Di - 1) - ry, 0);
    const int64 oz = std::max<int64>(std::min<int64>(z + rz, Di - 1) - rz, 0);
    return static_cast<float32>((oz * Di + oy) * Di + ox);
  };

  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<float32[]>(k_ChunkValues);
  bool allMatch = true;
  usize badSlot = 0;
  float32 badGot = 0.0f;
  float32 badExp = 0.0f;
  for(usize start = 0; start < numSlots && allMatch; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, numSlots - start);
    Result<> copyResult = outStore.copyIntoBuffer(start, nonstd::span<float32>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
    for(usize i = 0; i < count; ++i)
    {
      if(buffer[i] != expectedForSlot(start + i))
      {
        allMatch = false;
        badSlot = start + i;
        badGot = buffer[i];
        badExp = expectedForSlot(start + i);
        break;
      }
    }
  }
  if(!allMatch)
  {
    UNSCOPED_INFO("slot=" << badSlot << " got=" << badGot << " expected=" << badExp);
  }
  REQUIRE(allMatch);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden on STAPLE1.png (uint8, 200x150x1), Safe Border default (true). Duplicates the
// Box r{20,5,1}, Ball r{1,1,1}, and Cross r{20,5,2} cases of ITKGrayscaleMorphologicalOpeningImageTest. Each case
// does (A) md5 == committed ITK hash + (B) live-ITK bit-exact.
//
// The Ball/Cross cases are the regression for the composite Safe Border 2D size-1-axis pad defect (fixed by
// SafeBorderPadRadius in ImageProcessingFilterUtilities.hpp). Before the fix, padding the Z axis of this 2D image
// by the SE Z-radius gave Z real depth, so the 3D SE's out-of-plane offsets read materialized planes: Ball r{1,1,1}
// produced abf21fbe... (wrong) and Cross r{20,5,2} produced the unchanged-input identity 095f00a6... (no change).
// Clamping the pad to 0 on the size-1 axis (matching ITK's Z==1 => 2D collapse) makes all three reproduce ITK.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalOpeningImageFilter: ITK real-image golden (Box r{20,5,1})", "[ImageProcessing][ItkGolden][GrayscaleMorphologicalOpeningImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<GrayscaleMorphologicalOpeningImageFilter>("STAPLE1.png", "0a5ac0dbca31e1b92eb6d48e990582a7", static_cast<uint64>(morph_test::KernelType::Box), {20, 5, 1},
                                                                                  morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalOpeningImageFilter>(/*safeBorder=*/true));
}

TEST_CASE("ImageProcessing::GrayscaleMorphologicalOpeningImageFilter: ITK real-image golden (Ball r{1,1,1})", "[ImageProcessing][ItkGolden][GrayscaleMorphologicalOpeningImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<GrayscaleMorphologicalOpeningImageFilter>("STAPLE1.png", "867de5ed8cf49c4657e1545bd57f2c23", static_cast<uint64>(morph_test::KernelType::Ball), {1, 1, 1},
                                                                                  morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalOpeningImageFilter>(/*safeBorder=*/true));
}

TEST_CASE("ImageProcessing::GrayscaleMorphologicalOpeningImageFilter: ITK real-image golden (Cross r{20,5,2})", "[ImageProcessing][ItkGolden][GrayscaleMorphologicalOpeningImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<GrayscaleMorphologicalOpeningImageFilter>("STAPLE1.png", "5651a92320cfd9f01be4463131a4e573", static_cast<uint64>(morph_test::KernelType::Cross), {20, 5, 2},
                                                                                  morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalOpeningImageFilter>(/*safeBorder=*/true));
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (error -8390) fires before the shared PreflightImageFilter, so a 3-component input is rejected at preflight.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalOpeningImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][GrayscaleMorphologicalOpeningImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<GrayscaleMorphologicalOpeningImageFilter, float32>(
      -8390, morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalOpeningImageFilter>(/*safeBorder=*/true));
}

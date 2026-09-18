#include "MorphologyFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GrayscaleMorphologicalClosingImageFilter.hpp"

#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKGrayscaleMorphologicalClosingImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyGrayscaleClosingUuid = *Uuid::FromString("8b859b54-93d4-4341-8fbf-85e1e461d5b5");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 3D r{2,1,0}, 2D Z=1 r{2,2,0}}. The new
//     filter must reproduce the legacy ITK Grayscale Morphological Closing output EXACTLY (dilate then erode;
//     both folds select existing values). Annulus is excluded (empty legacy kernel). The (Box, 3D r{2,1,0})
//     cell is excluded from LIVE parity -- the legacy decomposable-Box anchor path produces garbage for a
//     zero-radius axis on a 3D image -- and is instead gated by a Box r{2,1,0} computed-expected case in test
//     (2); Ball/Cross at r{2,1,0} DO match live legacy.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalClosingImageFilter: Legacy parity grid (Ball/Box/Cross)", "[ImageProcessing][GrayscaleMorphologicalClosingImageFilter]")
{
  morph_test::RunMorphologyParityGrid<GrayscaleMorphologicalClosingImageFilter, float32>(
      k_LegacyGrayscaleClosingUuid, [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); },
      morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalClosingImageFilter>(/*safeBorder=*/true),
      [](std::string_view kernelName, const morph_test::MorphCase& cfg) { return kernelName == "Box" && std::string_view(cfg.label) == "3D r{2,1,0}"; });
}

// -----------------------------------------------------------------------------
// (2) Computed-expected (NOT legacy parity), independent oracle composed from the skip-OOB SE-gather folds
//     (closing == erode(dilate(in))). Driven with Safe Border == FALSE so the skip-OOB oracle applies; the
//     Safe Border == true (padded) convention is covered by the parity grid in (1). This validates:
//     - a standard non-degenerate kernel under Safe Border == false (Ball + Box), and
//     - Annulus: this filter ships ITK's real thickness-1 shell, whereas the legacy SimpleITK Annulus was empty.
//     - Box r{2,1,0}: the excluded parity cell from (1); the legacy ITK anchor path is broken for a zero-radius
//       axis on a 3D image, so the correct result is test-documented here rather than left silently absent.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalClosingImageFilter: Computed-expected (safeBorder=false; Annulus + Box zero-radius)", "[ImageProcessing][GrayscaleMorphologicalClosingImageFilter]")
{
  using morph_test::CompositeKind;
  using morph_test::KernelType;
  auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); };
  const auto setParams = morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalClosingImageFilter>(/*safeBorder=*/false);
  constexpr auto kClosing = CompositeKind::Closing;

  SECTION("safeBorder=false Ball 3D r{1,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalClosingImageFilter, float32>(build, 12, 12, 12, KernelType::Ball, {1, 1, 1}, kClosing, setParams);
  }
  SECTION("safeBorder=false Box 3D r{2,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalClosingImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 1}, kClosing, setParams);
  }
  SECTION("Annulus 3D r{1,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalClosingImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, kClosing, setParams);
  }
  SECTION("Annulus 3D r{2,2,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalClosingImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {2, 2, 2}, kClosing, setParams);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalClosingImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, kClosing, setParams);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalClosingImageFilter, float32>(build, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, kClosing, setParams);
  }
  SECTION("Box 3D zero-radius r{2,1,0} (legacy anchor path broken)")
  {
    morph_test::RunMorphologyCompositeComputedExpected<GrayscaleMorphologicalClosingImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 0}, kClosing, setParams);
  }
}

// -----------------------------------------------------------------------------
// (3) Both-algorithm-paths computed-expected on a ramp (value == flat index) with a Box structuring element. The
//     composite routes through ApplyMorphology's DispatchAlgorithm, so the AlgorithmTestScope runs the filter
//     under BOTH the in-core moving-histogram Direct path and the out-of-core Scanline path on in-memory stores
//     (selected by SIMPLNX_TEST_ALGORITHM_PATH). The legacy ITK filter rejects out-of-core arrays and is NOT run
//     here; instead the closed form of a Box closing (dilate then erode) of a strictly-increasing ramp is
//     asserted directly. The dilation of a monotone ramp is again a monotone field whose value at each voxel is
//     the flat index of its +radius corner; the subsequent erosion therefore reads that field at its -radius
//     corner. Composing the two (each clamped to the image) gives closing(x,y,z) = flat( min(max(x-rx,0)+rx,D-1),
//     ... ), i.e. the identity in the interior and a clamped shift near the boundary. (D^3 - 1 < 2^24, so float32
//     is exact.) Real-disk out-of-core coverage is retired to the generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalClosingImageFilter: Ramp computed-expected (both algorithm paths)", "[ImageProcessing][GrayscaleMorphologicalClosingImageFilter]")
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

  GrayscaleMorphologicalClosingImageFilter filter;
  Arguments args;
  args.insertOrAssign(GrayscaleMorphologicalClosingImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(GrayscaleMorphologicalClosingImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GrayscaleMorphologicalClosingImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(GrayscaleMorphologicalClosingImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Box)));
  args.insertOrAssign(GrayscaleMorphologicalClosingImageFilter::k_KernelRadius_Key,
                      std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{static_cast<uint32>(rx), static_cast<uint32>(ry), static_cast<uint32>(rz)}));
  // Safe Border == false so the skip-OOB closed form below applies; this exercises the composite's OOC
  // streaming of the plain two-pass path on both algorithm iterations.
  args.insertOrAssign(GrayscaleMorphologicalClosingImageFilter::k_SafeBorder_Key, std::make_any<bool>(false));

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
    // Closing = erode(dilate(ramp)): the dilation corner is the +radius corner, evaluated at the -radius
    // corner of the erosion (all clamped to the image).
    const int64 cx = std::min<int64>(std::max<int64>(x - rx, 0) + rx, Di - 1);
    const int64 cy = std::min<int64>(std::max<int64>(y - ry, 0) + ry, Di - 1);
    const int64 cz = std::min<int64>(std::max<int64>(z - rz, 0) + rz, Di - 1);
    return static_cast<float32>((cz * Di + cy) * Di + cx);
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
// ITK-sourced real-image md5 golden: duplicates the Ball r{1,1,1} case of ITKGrayscaleMorphologicalClosingImageTest
// on STAPLE1.png (uint8, 200x150x1), Safe Border default (true). (A) md5 == committed ITK hash + (B) live-ITK
// bit-exact. This is a regression for the composite Safe Border 2D size-1-axis pad defect (fixed by
// SafeBorderPadRadius in ImageProcessingFilterUtilities.hpp): before the fix, padding the Z axis of this 2D image by
// the SE Z-radius made the closing return the unchanged-input identity 095f00a68a84df4396914fa758f34dcc instead of
// the committed 103130cc4caf40d9fb252fbabc531e15; clamping the pad to 0 on the size-1 axis reproduces ITK exactly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalClosingImageFilter: ITK real-image golden (Ball r{1,1,1})", "[ImageProcessing][ItkGolden][GrayscaleMorphologicalClosingImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<GrayscaleMorphologicalClosingImageFilter>("STAPLE1.png", "103130cc4caf40d9fb252fbabc531e15", static_cast<uint64>(morph_test::KernelType::Ball), {1, 1, 1},
                                                                                  morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalClosingImageFilter>(/*safeBorder=*/true));
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (error -8400) fires before the shared PreflightImageFilter, so a 3-component input is rejected at preflight.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleMorphologicalClosingImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][GrayscaleMorphologicalClosingImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<GrayscaleMorphologicalClosingImageFilter, float32>(
      -8400, morph_test::CompositeKernelParamSetter<GrayscaleMorphologicalClosingImageFilter>(/*safeBorder=*/true));
}

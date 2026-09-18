#include "MorphologyFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GrayscaleDilateImageFilter.hpp"

#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKGrayscaleDilateImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyGrayscaleDilateUuid = *Uuid::FromString("944f4d1a-adb1-401a-9c0f-e2085ef2f6dc");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 3D r{2,1,0}, 2D Z=1 r{2,2,0}}. The new
//     filter must reproduce the legacy ITK Grayscale Dilate output EXACTLY. This is the correctness gate for
//     the morphology façade + MakeStructuringElement + the Dilate (max) fold. Annulus is excluded (empty
//     legacy kernel). The (Box, 3D r{2,1,0}) cell is excluded from LIVE parity -- the legacy decomposable-Box
//     anchor path produces garbage for a zero-radius axis on a 3D image -- and is instead gated by a Box
//     r{2,1,0} computed-expected case in test (2); Ball/Cross at r{2,1,0} DO match live legacy.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleDilateImageFilter: Legacy parity grid (Ball/Box/Cross)", "[ImageProcessing][GrayscaleDilateImageFilter]")
{
  morph_test::RunMorphologyParityGrid<GrayscaleDilateImageFilter, float32>(
      k_LegacyGrayscaleDilateUuid, [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); },
      morph_test::DefaultKernelParamSetter<GrayscaleDilateImageFilter>(),
      [](std::string_view kernelName, const morph_test::MorphCase& cfg) { return kernelName == "Box" && std::string_view(cfg.label) == "3D r{2,1,0}"; });
}

// -----------------------------------------------------------------------------
// (2) Computed-expected (NOT legacy parity), independent skip-OOB max oracle over MakeStructuringElement(...):
//     - Annulus: this filter ships ITK's real thickness-1 shell, whereas the legacy SimpleITK Annulus was empty.
//     - Box r{2,1,0}: the excluded parity cell from (1); the legacy ITK anchor path is broken for a zero-radius
//       axis on a 3D image, so the correct result is test-documented here rather than left silently absent.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleDilateImageFilter: Computed-expected (Annulus + Box zero-radius)", "[ImageProcessing][GrayscaleDilateImageFilter]")
{
  using morph_test::KernelType;
  auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); };
  const auto setParams = morph_test::DefaultKernelParamSetter<GrayscaleDilateImageFilter>();
  constexpr auto kDilate = ImageProcessing::MorphOp::Dilate;

  SECTION("Annulus 3D r{1,1,1}")
  {
    morph_test::RunMorphologyComputedExpected<GrayscaleDilateImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, kDilate, setParams);
  }
  SECTION("Annulus 3D r{2,2,2}")
  {
    morph_test::RunMorphologyComputedExpected<GrayscaleDilateImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {2, 2, 2}, kDilate, setParams);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    morph_test::RunMorphologyComputedExpected<GrayscaleDilateImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, kDilate, setParams);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    morph_test::RunMorphologyComputedExpected<GrayscaleDilateImageFilter, float32>(build, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, kDilate, setParams);
  }
  SECTION("Box 3D zero-radius r{2,1,0} (legacy anchor path broken)")
  {
    morph_test::RunMorphologyComputedExpected<GrayscaleDilateImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 0}, kDilate, setParams);
  }
}

// -----------------------------------------------------------------------------
// (2b) Degenerate empty structuring element (Annulus r{0,0,0}) must pass the input through unchanged. First
//      user-reachable exercise of the engine's offsets.empty() passthrough branch.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleDilateImageFilter: Empty-SE passthrough (Annulus r{0,0,0})", "[ImageProcessing][GrayscaleDilateImageFilter]")
{
  morph_test::RunMorphologyEmptySEPassthrough<GrayscaleDilateImageFilter, float32>(
      [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); }, 12, 12, 12,
      morph_test::DefaultKernelParamSetter<GrayscaleDilateImageFilter>());
}

// -----------------------------------------------------------------------------
// (3) Both-algorithm-paths computed-expected on a ramp (value == flat index) with a Box structuring element. The
//     morphology façade routes through ApplyMorphology's DispatchAlgorithm, so the AlgorithmTestScope runs the
//     filter under BOTH the in-core moving-histogram Direct path and the out-of-core Scanline path on in-memory
//     stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). The legacy ITK filter rejects out-of-core arrays and is
//     NOT run here; instead the mathematical property of a Box dilation of a strictly-increasing ramp is asserted
//     directly: the max over the (clamped) box neighborhood is attained at the +radius corner. (D^3 - 1 < 2^24,
//     so float32 represents every index exactly.) Real-disk out-of-core coverage is retired to the generic
//     SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleDilateImageFilter: Ramp computed-expected (both algorithm paths)", "[ImageProcessing][GrayscaleDilateImageFilter]")
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

  GrayscaleDilateImageFilter filter;
  Arguments args;
  args.insertOrAssign(GrayscaleDilateImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(GrayscaleDilateImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GrayscaleDilateImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(GrayscaleDilateImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Box)));
  args.insertOrAssign(GrayscaleDilateImageFilter::k_KernelRadius_Key,
                      std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{static_cast<uint32>(rx), static_cast<uint32>(ry), static_cast<uint32>(rz)}));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<float32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = D * D * D;
  REQUIRE(outStore.getSize() == numSlots);

  // Dilate (max) over a Box neighborhood of a ramp (value == flat index): the max flat index in the clamped
  // box is at the +radius corner clamped to the image bounds.
  auto expectedForSlot = [&](usize s) -> float32 {
    const int64 x = static_cast<int64>(s % D);
    const int64 y = static_cast<int64>((s / D) % D);
    const int64 z = static_cast<int64>(s / (D * D));
    const int64 cx = std::min<int64>(x + rx, static_cast<int64>(D) - 1);
    const int64 cy = std::min<int64>(y + ry, static_cast<int64>(D) - 1);
    const int64 cz = std::min<int64>(z + rz, static_cast<int64>(D) - 1);
    return static_cast<float32>(cz * static_cast<int64>(D) * static_cast<int64>(D) + cy * static_cast<int64>(D) + cx);
  };

  // Stream the output in bounded chunks (avoids per-element OOC access and per-voxel Catch2 assertions).
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
// ITK-sourced real-image md5 golden -- duplicates ITKGrayscaleDilateImageTest.cpp(GrayscaleDilate): STAPLE1.png
// (uint8), Ball radius {1,1,1}. (A) md5 == committed hash + (B) live-ITK bit-exact. STAPLE1 is a 195-value
// grayscale image (verified), which grayscale morphology processes directly (no binary-input constraint).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleDilateImageFilter: ITK real-image golden (GrayscaleDilate)", "[ImageProcessing][ItkGolden][GrayscaleDilateImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<GrayscaleDilateImageFilter>("STAPLE1.png", "cb692559f1eb21e4c932f6bbb3850ad3", static_cast<uint64>(morph_test::KernelType::Ball), {1, 1, 1},
                                                                    morph_test::DefaultKernelParamSetter<GrayscaleDilateImageFilter>());
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (error -8370) fires before the shared PreflightImageFilter, so a 3-component input is rejected at preflight.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleDilateImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][GrayscaleDilateImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<GrayscaleDilateImageFilter, float32>(-8370, morph_test::DefaultKernelParamSetter<GrayscaleDilateImageFilter>());
}

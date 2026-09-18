#include "ProjectionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryProjectionImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKBinaryProjectionImageFilter, created at runtime by UUID.
const Uuid k_LegacyBinaryProjectionUuid = *Uuid::FromString("04ea495e-2cf0-4dba-8d29-cf33a38c094d");

// Binary-projection foreground/background used by both the parity grid and the OOC case. The foreground
// value is deliberately chosen to appear in only SOME columns (see the builder), so both output branches
// (foreground / background) are exercised rather than trivially all-foreground or all-background.
constexpr float64 k_ForegroundValue = 100.0;
constexpr float64 k_BackgroundValue = 0.0;

/**
 * @brief Builds "Image Geometry"/"CellData"/"Input" (dim^3, float32) whose value at (x,y,z) is the Y
 *        coordinate. Because the value is constant along X and Z but varies along Y, a projected column
 *        contains the foreground value only for specific slots (collapse-X / collapse-Z: iff y == fg;
 *        collapse-Y: every column spans all Y values so it always contains fg) -- meaningfully exercising
 *        the fg/bg logic. Filled via bulk copyFromBuffer in bounded chunks so it is fast in-core AND OOC.
 */
template <class T>
DataPath BuildYCoordImage(DataStructure& ds, usize dim)
{
  const ShapeType cellShape = {dim, dim, dim};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dim, dim, dim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<T>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<T>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();

  const usize total = ref.getSize();
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<T[]>(k_ChunkValues);
  for(usize start = 0; start < total; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, total - start);
    for(usize i = 0; i < count; ++i)
    {
      const usize flatIndex = start + i;
      const usize y = (flatIndex / dim) % dim; // value == Y coordinate
      buffer[i] = static_cast<T>(y);
    }
    Result<> writeResult = ref.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
  return inputPath;
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: the new filter must produce the EXACT output the legacy ITK Binary Projection
//     filter produces, over ProjectionDimension in {0,1,2} x Perform-In-Place in {true,false}. The input's
//     value is the Y coordinate, so with foreground == a specific Y value some columns contain foreground
//     and some do not -- exercising both the foreground and background output branches. Foreground and
//     background values are set via the extra-params hook. This is the correctness gate for the
//     axis-projection façade (AllNumeric policy) + engine + BinaryReduce.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryProjectionImageFilter: Legacy parity grid (3D)", "[ImageProcessing][BinaryProjectionImageFilter]")
{
  // At dim==12 the foreground value must be within [0, 11] so some columns contain it; use 5.
  const auto setExtraParams = [](Arguments& args) {
    args.insertOrAssign(BinaryProjectionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(5.0));
    args.insertOrAssign(BinaryProjectionImageFilter::k_BackgroundValue_Key, std::make_any<float64>(k_BackgroundValue));
  };
  projection_test::RunProjectionParityGrid<BinaryProjectionImageFilter, float32>(k_LegacyBinaryProjectionUuid, [](DataStructure& ds) { return BuildYCoordImage<float32>(ds, 12); }, setExtraParams);
}

// -----------------------------------------------------------------------------
// (3) Preflight guard: BinaryReduce casts the Float64 foreground/background parameters to the input element
//     type; for an INTEGER input that conversion is undefined for a non-finite or out-of-range value. The
//     filter must reject such parameters at preflight. In-range integer values (and any value on a float
//     input, exercised elsewhere) must still preflight cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryProjectionImageFilter: Preflight rejects non-finite / out-of-range fg/bg for integer input", "[ImageProcessing][BinaryProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = BuildYCoordImage<uint8>(ds, 8); // integer (uint8) input, range [0, 255]

  const auto makeArgs = [&](float64 fg, float64 bg) {
    Arguments args;
    args.insertOrAssign(BinaryProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(BinaryProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(BinaryProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{0}));
    args.insertOrAssign(BinaryProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false)); // new-geometry: leave the input intact
    args.insertOrAssign(BinaryProjectionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(BinaryProjectionImageFilter::k_BackgroundValue_Key, std::make_any<float64>(bg));
    args.insertOrAssign(BinaryProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
    args.insertOrAssign(BinaryProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    return args;
  };

  BinaryProjectionImageFilter filter;

  // Foreground out of uint8 range (255 max) -> undefined cast -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(99999.0, 0.0)).outputActions.valid());
  // Background out of range on the negative side -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(1.0, -5.0)).outputActions.valid());
  // Non-finite (NaN) foreground -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(std::numeric_limits<float64>::quiet_NaN(), 0.0)).outputActions.valid());
  // Non-finite (Inf) background -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(1.0, std::numeric_limits<float64>::infinity())).outputActions.valid());
  // In-range integer values still preflight cleanly.
  auto preflightResult = filter.preflight(ds, makeArgs(5.0, 0.0));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
}

// -----------------------------------------------------------------------------
// (4) Fractional-truncation warning (-8352). A fractional but IN-RANGE fg/bg on an integer input is
//     representable (it truncates toward zero), so preflight stays VALID but must carry the truncation warning.
//     The error paths (non-finite / out-of-range) are covered above; this is the surviving warning branch.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryProjectionImageFilter: Fractional in-range fg on integer input -> warning (-8352)", "[ImageProcessing][BinaryProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = BuildYCoordImage<uint8>(ds, 8); // integer (uint8) input

  BinaryProjectionImageFilter filter;
  Arguments args;
  args.insertOrAssign(BinaryProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(BinaryProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BinaryProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{0}));
  args.insertOrAssign(BinaryProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(BinaryProjectionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(1.5)); // fractional, in [0, 255]
  args.insertOrAssign(BinaryProjectionImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
  args.insertOrAssign(BinaryProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
  args.insertOrAssign(BinaryProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  bool hasTruncationWarning = false;
  for(const auto& warning : preflightResult.outputActions.warnings())
  {
    if(warning.code == -8352)
    {
      hasTruncationWarning = true;
    }
  }
  REQUIRE(hasTruncationWarning);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden tests (plan Task 5) -- duplicate each ITKBinaryProjectionImageTest.cpp case on
// OUR ITK-free filter: (A) output md5 == ITK's committed hash + (B) live-ITK bit-exact parity. Binary projection
// emits foreground/background cast to the input type (type-preserving), so both oracles are bit-exact.
//   - "defaults": 2th_cthead1.mha read through OUR MHA reader; projection dimension 0, defaults fg=1/bg=0.
//   - "another_dimension": WhiteDots.png; projection dimension 1, foreground value 255.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryProjectionImageFilter: ITK real-image golden (defaults, MHA)", "[ImageProcessing][ItkGolden][BinaryProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<BinaryProjectionImageFilter>("2th_cthead1.mha", /*projDim=*/0, /*removeOriginalGeometry=*/true, "3fc3603b27bf51df592190227d6cd6ed");
}
TEST_CASE("ImageProcessing::BinaryProjectionImageFilter: ITK real-image golden (another_dimension)", "[ImageProcessing][ItkGolden][BinaryProjectionImageFilter]")
{
  const auto setForeground = [](Arguments& args) { args.insertOrAssign(BinaryProjectionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(255.0)); };
  projection_test::RunProjectionMd5ItkGolden<BinaryProjectionImageFilter>("WhiteDots.png", /*projDim=*/1, /*removeOriginalGeometry=*/true, "827f263ef9fb63d05499d14fcef32f60", setForeground);
}

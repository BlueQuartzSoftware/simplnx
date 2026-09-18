#include "ProjectionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MaximumProjectionImageFilter.hpp"

#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKMaximumProjectionImageFilter, created at runtime by UUID.
const Uuid k_LegacyMaximumProjectionUuid = *Uuid::FromString("6dfe9167-d77d-41c7-aebf-569d6190645d");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: the new filter must produce the EXACT output the legacy ITK Maximum Projection
//     filter produces, over ProjectionDimension in {0,1,2} x Perform-In-Place in {true,false}. A ramp
//     input (value == flat index) makes the per-axis maximum differ by axis, so a wrong axis mapping or a
//     wrong in-place/new-geometry output shape is caught. This is the correctness gate for the
//     axis-projection façade + engine + MaxReduce.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: Legacy parity grid (3D)", "[ImageProcessing][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MaximumProjectionImageFilter, float32>(k_LegacyMaximumProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<float32>(ds, 12, 0.0, 1.0); });
}

// -----------------------------------------------------------------------------
// (1b) In-place mode replaces the whole original geometry, so any OTHER arrays in its cell data are
//      discarded. Preflight must SUCCEED but emit a warning naming them (never silently drop them).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: In-place discards sibling arrays with a warning", "[ImageProcessing][MaximumProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, 8, 0.0, 1.0);

  // Add a second array "Extra" alongside "Input" in the same cell-data Attribute Matrix.
  const DataPath cellDataPath = inputPath.getParent();
  auto* cellAM = ds.getDataAs<AttributeMatrix>(cellDataPath);
  REQUIRE(cellAM != nullptr);
  const ShapeType cellShape = cellAM->getShape();
  auto extraStore = DataStoreUtilities::CreateDataStore<int16>(ds, cellDataPath.createChildPath("Extra"), cellShape, {1});
  REQUIRE(DataArray<int16>::Create(ds, "Extra", extraStore, cellAM->getId()) != nullptr);

  MaximumProjectionImageFilter filter;
  Arguments args;
  args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{1}));
  args.insertOrAssign(MaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true)); // in-place
  args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
  args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto& warnings = preflightResult.outputActions.warnings();
  bool namesExtra = false;
  for(const auto& warning : warnings)
  {
    if(warning.message.find("Extra") != std::string::npos)
    {
      namesExtra = true;
    }
  }
  REQUIRE(namesExtra);
}

// -----------------------------------------------------------------------------
// (Type-dispatch breadth) The parity grid above exercises only float32; the projection façade dispatches over
// {uint8, int16, uint16, float32}. These add uint8 and uint16 input coverage, still bit-exact vs the legacy ITK
// filter (which processes those types). Small dims keep the ramp within the integer type's range so values stay
// distinct (uint8: 6^3 = 216 <= 255).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: Legacy parity grid (uint8 input)", "[ImageProcessing][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MaximumProjectionImageFilter, uint8>(k_LegacyMaximumProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<uint8>(ds, 6, 0.0, 1.0); });
}
TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: Legacy parity grid (uint16 input)", "[ImageProcessing][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MaximumProjectionImageFilter, uint16>(k_LegacyMaximumProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<uint16>(ds, 8, 0.0, 1.0); });
}

// -----------------------------------------------------------------------------
// (Shared-façade preflight guards, PreflightAxisProjection) Two data/param-independent guards the parity grids
// never reach: an out-of-range Projection Dimension (> 2) -> -8340, and a multi-component (non-scalar) input ->
// -8341. Exercised on Maximum (the guards live in the shared façade, so one filter covers them for all).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: Preflight guards (projDim>2 -8340, non-scalar -8341)", "[ImageProcessing][MaximumProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  MaximumProjectionImageFilter filter;

  SECTION("Projection Dimension > 2 -> -8340")
  {
    DataStructure ds;
    const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, 6, 0.0, 1.0);
    Arguments args;
    args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{3})); // invalid axis
    args.insertOrAssign(MaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
    args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    auto result = filter.preflight(ds, args);
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == -8340);
  }

  SECTION("Non-scalar (multi-component) input -> -8341")
  {
    DataStructure ds;
    const ShapeType cellShape = {4, 4, 4};
    auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
    imageGeom->setDimensions({4, 4, 4});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
    auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    const DataPath inputPath({"Image Geometry", "CellData", "Input"});
    auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, cellShape, {3}); // 3-component (non-scalar)
    DataArray<float32>::Create(ds, "Input", store, cellAM->getId());
    Arguments args;
    args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(uint32{0}));
    args.insertOrAssign(MaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
    args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    auto result = filter.preflight(ds, args);
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == -8341);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden tests (plan Task 5) -- duplicate each ITKMaximumProjectionImageTest.cpp case on
// OUR ITK-free filter: (A) output md5 == ITK's committed hash + (B) live-ITK bit-exact parity. Maximum is a
// type-preserving reducer (float32 in -> float32 out; int16 in -> int16 out), so both oracles are bit-exact.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: ITK real-image golden (default in-place)", "[ImageProcessing][ItkGolden][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MaximumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/true, "fb78c55635b17fc9ff38ef0ef14f0948");
}
TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: ITK real-image golden (new geometry)", "[ImageProcessing][ItkGolden][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MaximumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/false, "fb78c55635b17fc9ff38ef0ef14f0948");
}
TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: ITK real-image golden (dimensional)", "[ImageProcessing][ItkGolden][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MaximumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/2, /*removeOriginalGeometry=*/true, "f3f0d97c83c6b0d92df10c28e2481520");
}
TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: ITK real-image golden (short)", "[ImageProcessing][ItkGolden][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MaximumProjectionImageFilter>("Ramp-Up-Short.nrrd", /*projDim=*/1, /*removeOriginalGeometry=*/true, "5390344262c91e83bc9208b0991a2fc9");
}

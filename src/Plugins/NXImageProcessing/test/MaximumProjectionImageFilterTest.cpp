#include "ProjectionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/MaximumProjectionImageFilter.hpp"

#include <memory>

using namespace nx::core;

// -----------------------------------------------------------------------------
// (1b) In-place mode replaces the whole original geometry, so any OTHER arrays in its cell data are
//      discarded. Preflight must SUCCEED but emit a warning naming them (never silently drop them).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::MaximumProjectionImageFilter: In-place discards sibling arrays with a warning", "[NXImageProcessing][MaximumProjectionImageFilter]")
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
// (Shared-façade preflight guards, PreflightAxisProjection) Two data/param-independent guards: an out-of-range Projection Dimension (> 2) -> -8340, and a multi-component (non-scalar) input ->
// -8341. Exercised on Maximum (the guards live in the shared façade, so one filter covers them for all).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::MaximumProjectionImageFilter: Preflight guards (projDim>2 -8340, non-scalar -8341)", "[NXImageProcessing][MaximumProjectionImageFilter]")
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
// OUR ITK-free filter: the output md5 must equal ITK's committed hash. Maximum is a type-preserving reducer
// (float32 in -> float32 out; int16 in -> int16 out), so the hash is bit-exact.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::MaximumProjectionImageFilter: ITK real-image golden (default in-place)", "[NXImageProcessing][ItkGolden][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MaximumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/true, "fb78c55635b17fc9ff38ef0ef14f0948");
}
TEST_CASE("NXImageProcessing::MaximumProjectionImageFilter: ITK real-image golden (new geometry)", "[NXImageProcessing][ItkGolden][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MaximumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/0, /*removeOriginalGeometry=*/false, "fb78c55635b17fc9ff38ef0ef14f0948");
}
TEST_CASE("NXImageProcessing::MaximumProjectionImageFilter: ITK real-image golden (dimensional)", "[NXImageProcessing][ItkGolden][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MaximumProjectionImageFilter>("RA-Float.nrrd", /*projDim=*/2, /*removeOriginalGeometry=*/true, "f3f0d97c83c6b0d92df10c28e2481520");
}
TEST_CASE("NXImageProcessing::MaximumProjectionImageFilter: ITK real-image golden (short)", "[NXImageProcessing][ItkGolden][MaximumProjectionImageFilter]")
{
  projection_test::RunProjectionMd5ItkGolden<MaximumProjectionImageFilter>("Ramp-Up-Short.nrrd", /*projDim=*/1, /*removeOriginalGeometry=*/true, "5390344262c91e83bc9208b0991a2fc9");
}

TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: Copy non-cell Attribute Matrices", "[ImageProcessing][MaximumProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const bool inPlace = GENERATE(true, false);
  const bool extraCellArray = GENERATE(false, true);
  // Omitted exercises callers written before this option existed.
  const int copyOption = GENERATE(-1, 0, 1);
  const bool copyNonCell = copyOption == 1;
  CAPTURE(inPlace, extraCellArray, copyOption);

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, 3, 0.0, 1.0);
  const DataPath geomPath({"Image Geometry"});
  auto& imageGeom = ds.getDataRefAs<ImageGeom>(geomPath);
  const DataPath featurePath = geomPath.createChildPath("Cell Feature Data");
  auto* featureAM = AttributeMatrix::Create(ds, "Cell Feature Data", {2}, imageGeom.getId());
  REQUIRE(featureAM != nullptr);
  const DataPath featureArrayPath = featurePath.createChildPath("Feature Values");
  auto featureStore = DataStoreUtilities::CreateDataStore<int32>(ds, featureArrayPath, {2}, {1});
  REQUIRE(Int32Array::Create(ds, "Feature Values", featureStore, featureAM->getId()) != nullptr);
  featureStore->setValue(0, 17);
  featureStore->setValue(1, 29);
  const DataPath ensemblePath = geomPath.createChildPath("Cell Ensemble Data");
  REQUIRE(AttributeMatrix::Create(ds, "Cell Ensemble Data", {1}, imageGeom.getId()) != nullptr);
  const auto featureId = featureAM->getId();
  const auto featureArrayId = ds.getDataRefAs<Int32Array>(featureArrayPath).getId();
  const auto ensembleId = ds.getDataRefAs<AttributeMatrix>(ensemblePath).getId();
  if(extraCellArray)
  {
    const DataPath cellPath = inputPath.getParent();
    auto& cellAM = ds.getDataRefAs<AttributeMatrix>(cellPath);
    auto extraStore = DataStoreUtilities::CreateDataStore<int16>(ds, cellPath.createChildPath("Extra"), cellAM.getShape(), {1});
    REQUIRE(Int16Array::Create(ds, "Extra", extraStore, cellAM.getId()) != nullptr);
  }

  MaximumProjectionImageFilter filter;
  Arguments args;
  args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(MaximumProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MaximumProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(0));
  args.insertOrAssign(MaximumProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(inPlace));
  args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
  args.insertOrAssign(MaximumProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  if(copyOption >= 0)
  {
    args.insertOrAssign("copy_non_cell_attribute_matrices", std::make_any<bool>(copyNonCell));
  }
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  std::string discardedWarning;
  for(const auto& warning : preflightResult.outputActions.warnings())
  {
    if(warning.code == -8343)
    {
      discardedWarning += warning.message;
    }
  }
  // Only discarded objects belong in the warning. Preserved matrices must not be listed.
  if(inPlace)
  {
    CHECK((discardedWarning.find("Cell Feature Data") != std::string::npos) == !copyNonCell);
    CHECK((discardedWarning.find("Cell Ensemble Data") != std::string::npos) == !copyNonCell);
    CHECK(discardedWarning.empty() == (copyNonCell && !extraCellArray));
    if(extraCellArray)
    {
      CHECK(discardedWarning.find("Extra") != std::string::npos);
    }
  }
  else
  {
    CHECK(discardedWarning.empty());
  }

  // Axis projection dispatches by store type; it has no Direct/Scanline AlgorithmTestScope override.
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputGeomPath = inPlace ? geomPath : DataPath({"Projected Image"});
  REQUIRE(ds.getDataRefAs<ImageGeom>(outputGeomPath).getDimensions() == SizeVec3{1, 3, 3});
  const auto& output = ds.getDataRefAs<Float32Array>(outputGeomPath.createChildPath("CellData").createChildPath("Output"));
  REQUIRE(output.getNumberOfTuples() == 9);
  for(usize slot = 0; slot < 9; ++slot)
  {
    // Along X, the 3x3x3 ramp has columns {3*s, 3*s+1, 3*s+2}, so max = 3*s+2.
    REQUIRE(output[slot] == static_cast<float32>(3 * slot + 2));
  }
  if(copyNonCell)
  {
    const DataPath outputFeaturePath = outputGeomPath.createChildPath("Cell Feature Data");
    const auto* outputFeatureAM = ds.getDataAs<AttributeMatrix>(outputFeaturePath);
    REQUIRE(outputFeatureAM != nullptr);
    CHECK(outputFeatureAM->getShape() == ShapeType{2});
    auto& features = ds.getDataRefAs<Int32Array>(outputFeaturePath.createChildPath("Feature Values"));
    CHECK(features[0] == 17);
    CHECK(features[1] == 29);
    const auto* ensemble = ds.getDataAs<AttributeMatrix>(outputGeomPath.createChildPath("Cell Ensemble Data"));
    REQUIRE(ensemble != nullptr);
    CHECK(ensemble->getShape() == ShapeType{1});
    CHECK(ensemble->getSize() == 0);
    if(inPlace)
    {
      CHECK(outputFeatureAM->getId() == featureId);
      CHECK(features.getId() == featureArrayId);
      CHECK(features.getDataStore() == featureStore.get());
      CHECK(ensemble->getId() == ensembleId);
    }
    else
    {
      CHECK(outputFeatureAM->getId() != featureId);
      CHECK(features.getId() != featureArrayId);
      CHECK(features.getDataStore() != featureStore.get());
      CHECK(ensemble->getId() != ensembleId);
      features[0] = 101;
      CHECK(ds.getDataRefAs<Int32Array>(featureArrayPath)[0] == 17);
      featureStore->setValue(1, 103);
      CHECK(features[1] == 29);
      featureStore->setValue(1, 29);
    }
  }
  else
  {
    CHECK(ds.getData(outputGeomPath.createChildPath("Cell Feature Data")) == nullptr);
    CHECK(ds.getData(outputGeomPath.createChildPath("Cell Ensemble Data")) == nullptr);
  }
  if(!inPlace)
  {
    const auto& features = ds.getDataRefAs<Int32Array>(featureArrayPath);
    CHECK(features[0] == 17);
    CHECK(features[1] == 29);
    CHECK(ds.getDataRefAs<AttributeMatrix>(featurePath).getId() == featureId);
    CHECK(ds.getDataRefAs<AttributeMatrix>(ensemblePath).getId() == ensembleId);
    CHECK(ds.getDataRefAs<ImageGeom>(geomPath).getDimensions() == SizeVec3{3, 3, 3});
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("ImageProcessing::MaximumProjectionImageFilter: Older pipelines default to discarding non-cell matrices", "[ImageProcessing][MaximumProjectionImageFilter]")
{
  MaximumProjectionImageFilter filter;
  auto json = filter.toJson(filter.getDefaultArguments());
  json.erase("copy_non_cell_attribute_matrices");
  json["parameters_version"] = 1;
  const auto converted = filter.fromJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(converted);
  REQUIRE(converted.value().contains("copy_non_cell_attribute_matrices"));
  CHECK_FALSE(converted.value().value<bool>("copy_non_cell_attribute_matrices"));
}

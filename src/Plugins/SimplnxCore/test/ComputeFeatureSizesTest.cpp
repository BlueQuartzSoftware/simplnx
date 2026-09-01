#include "SimplnxCore/Filters/ComputeFeatureSizesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>
#include <filesystem>

using namespace nx::core;

namespace fs = std::filesystem;

/**
 * @namespace LegacyTest
 * @brief Provides array names from the Small IN100 exemplar.
 */
namespace LegacyTest
{
const std::string k_Volumes("Volumes");
const std::string k_EquivalentDiameters("EquivalentDiameters");
} // namespace LegacyTest

/**
 * @namespace Test
 * @brief Provides generated feature-size fixtures and analytical validators.
 */
namespace Test
{
/**
 * @var k_RelativeTolerance
 * @brief Relative tolerance for hand-derived float32 oracle values.
 *
 * The tolerance permits several float32 ULPs from platform and TBB reduction order.
 * A larger difference indicates a deviation from the analytical result.
 */
constexpr float64 k_RelativeTolerance = 1.0e-6;

// These names and paths define the test geometry hierarchy.
const std::string k_ImageGeomName = "Image";
const DataPath k_ImageGeomPath = DataPath({k_ImageGeomName});

const std::string k_CellAMName = "CellData";
const DataPath k_CellAMPath = k_ImageGeomPath.createChildPath(k_CellAMName);
const std::string k_FeatureIdsName = "FeatureIds";
const DataPath k_FeatureIdsPath = k_CellAMPath.createChildPath(k_FeatureIdsName);

const std::string k_FeatureAMName = "FeatureData";
const DataPath k_FeatureAMPath = k_ImageGeomPath.createChildPath(k_FeatureAMName);

// These names and paths select generated output arrays.
const std::string k_NumElementsName = "NumElements";
const DataPath k_NumElementsPath = k_FeatureAMPath.createChildPath(k_NumElementsName);
const std::string k_VolumesName = "Volumes";
const DataPath k_VolumesPath = k_FeatureAMPath.createChildPath(k_VolumesName);
const std::string k_EquivalentDiametersName = "EquivalentDiameters";
const DataPath k_EquivalentDiametersPath = k_FeatureAMPath.createChildPath(k_EquivalentDiametersName);

/**
 * @brief Creates a two-dimensional ImageGeom feature fixture.
 * @return The populated DataStructure.
 */
DataStructure Create2DImageDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{20.2f, 0.1f, 1.0f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{5, 5, 1}});

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{5, 5, 1}, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{4}, imageGeom->getId());

  // clang-format off
  // The following values are the analytical outputs.
  // One cell has area 2.02.
  // Feature element counts are 0 11 1 13.
  // Feature areas are 0.0 22.22 2.02 26.26.
  // Equivalent diameters are 0.0 5.319 1.6037 5.782.
  const std::array<uint8, 25> featureIdsArray = {
    1, 2, 3, 3, 3,
    1, 1, 1, 1, 1,
    1, 1, 1, 3, 3,
    3, 3, 1, 1, 3,
    3, 3, 3, 3, 3
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  return dataStructure;
}

/**
 * @brief Compares two-dimensional feature sizes with analytical values.
 * @param dataStructure Contains the generated size arrays.
 */
void Validate2DImageDataStructure(const DataStructure& dataStructure)
{
  // The following values are the analytical outputs.
  // One cell has area 2.02.
  // Feature element counts are 0 11 1 13.
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 11);
  REQUIRE(numElements.getValue(2) == 1);
  REQUIRE(numElements.getValue(3) == 13);
  // Feature areas are 0.0 22.22 2.02 26.26.
  const auto& areas = dataStructure.getDataRefAs<Float32Array>(k_VolumesPath);
  REQUIRE(areas.getValue(1) == Approx(22.220001f).epsilon(k_RelativeTolerance));
  REQUIRE(areas.getValue(2) == Approx(2.0200002f).epsilon(k_RelativeTolerance));
  REQUIRE(areas.getValue(3) == Approx(26.260002f).epsilon(k_RelativeTolerance));
  // Equivalent diameters are 0.0 5.318964 1.603728 5.78232.
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath);
  REQUIRE(equivalentDiameters.getValue(1) == Approx(5.3189644f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(2) == Approx(1.60372818f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(3) == Approx(5.7823243f).epsilon(k_RelativeTolerance));
}

/**
 * @brief Creates a three-dimensional ImageGeom feature fixture.
 * @return The populated DataStructure.
 */
DataStructure Create3DImageDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.2f, 0.9f, 2.1f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{5, 5, 5}});

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{5, 5, 5}, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{4}, imageGeom->getId());

  // clang-format off
  // The following values are the analytical outputs.
  // One cell has volume 2.268.
  // Feature element counts are 0 73 29 23.
  // Feature volumes are 0.0 165.564 65.772 52.164.
  // Equivalent diameters are 0.0 6.813 5.008 4.636.
  const std::array<uint8, 125> featureIdsArray = {
    1, 2, 2, 2, 2,
    1, 1, 1, 1, 1,
    1, 1, 1, 2, 2,
    2, 2, 1, 1, 2,
    2, 2, 2, 2, 2,

    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,

    3, 3, 3, 3, 1,
    1, 3, 1, 3, 3,
    2, 2, 1, 1, 1,
    2, 2, 1, 1, 3,
    2, 2, 1, 3, 3,

    3, 2, 2, 1, 1,
    3, 2, 1, 1, 1,
    3, 1, 1, 1, 1,
    3, 2, 1, 2, 1,
    3, 2, 2, 2, 2,

    3, 1, 3, 1, 1,
    1, 1, 1, 1, 1,
    3, 1, 1, 1, 3,
    1, 1, 1, 3, 1,
    3, 1, 3, 1, 3
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  return dataStructure;
}

/**
 * @brief Compares three-dimensional ImageGeom feature sizes with analytical values.
 * @param dataStructure Contains the generated size arrays.
 */
void Validate3DImageDataStructure(const DataStructure& dataStructure)
{
  // The following values are the analytical outputs.
  // One cell has volume 2.268.
  // Feature element counts are 0 73 29 23.
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 73);
  REQUIRE(numElements.getValue(2) == 29);
  REQUIRE(numElements.getValue(3) == 23);
  // Feature volumes are 0.0 165.564 65.772 52.164.
  const auto& volumes = dataStructure.getDataRefAs<Float32Array>(k_VolumesPath);
  REQUIRE(volumes.getValue(1) == Approx(165.564f).epsilon(k_RelativeTolerance));
  REQUIRE(volumes.getValue(2) == Approx(65.771995f).epsilon(k_RelativeTolerance));
  REQUIRE(volumes.getValue(3) == Approx(52.163997f).epsilon(k_RelativeTolerance));
  // Equivalent diameters are 0.0 6.81275 5.00819 4.63579.
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath);
  REQUIRE(equivalentDiameters.getValue(1) == Approx(6.8127493f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(2) == Approx(5.0081901f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(3) == Approx(4.6357936f).epsilon(k_RelativeTolerance));
}

/**
 * @brief Creates a nonuniform RectGridGeom feature fixture.
 * @return The populated DataStructure.
 */
DataStructure CreateRectGridDataStructure()
{
  DataStructure dataStructure = {};
  RectGridGeom* rectGridGeom = RectGridGeom::Create(dataStructure, k_ImageGeomName);
  rectGridGeom->setDimensions(SizeVec3{std::array<usize, 3>{4, 4, 4}});

  // X bounds are 0.0f, 0.6f, 0.9f, 2.1f, 13.0f.
  Float32Array* xBoundsArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "xBounds", ShapeType{5}, ShapeType{1}, rectGridGeom->getId());
  xBoundsArray->setValue(0, 0.0f);
  xBoundsArray->setValue(1, 0.6f);
  xBoundsArray->setValue(2, 0.9f);
  xBoundsArray->setValue(3, 2.1f);
  xBoundsArray->setValue(4, 13.0f);
  rectGridGeom->setXBoundsId(xBoundsArray->getId());

  // Y bounds are 0.0f, 0.1f, 1.0f, 10.0f, 100.0f.
  Float32Array* yBoundsArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "yBounds", ShapeType{5}, ShapeType{1}, rectGridGeom->getId());
  yBoundsArray->setValue(0, 0.0f);
  yBoundsArray->setValue(1, 0.1f);
  yBoundsArray->setValue(2, 1.0f);
  yBoundsArray->setValue(3, 10.0f);
  yBoundsArray->setValue(4, 100.0f);
  rectGridGeom->setYBoundsId(yBoundsArray->getId());

  // Z bounds are 0.0f, 1.0f, 1.2f, 2.0f, 2.1f.
  Float32Array* zBoundsArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "zBounds", ShapeType{5}, ShapeType{1}, rectGridGeom->getId());
  zBoundsArray->setValue(0, 0.0f);
  zBoundsArray->setValue(1, 1.0f);
  zBoundsArray->setValue(2, 1.2f);
  zBoundsArray->setValue(3, 2.0f);
  zBoundsArray->setValue(4, 2.1f);
  rectGridGeom->setZBoundsId(zBoundsArray->getId());

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{4, 4, 4}, rectGridGeom->getId());
  rectGridGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{4}, rectGridGeom->getId());

  // clang-format off
  // The following values are the analytical outputs.
  // Feature element counts are 0 39 15 10.
  // Feature volumes are 0.0 2362.434 352.462 15.104.
  // Equivalent diameters are 0.0 16.5242 8.76404 3.06689.
  const std::array<uint8, 64> featureIdsArray = {
    1, 2, 2, 2,
    1, 1, 1, 1,
    1, 1, 1, 2,
    2, 2, 1, 1,

    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,

    3, 3, 3, 3,
    1, 3, 1, 3,
    2, 2, 1, 1,
    2, 2, 1, 1,

    3, 2, 2, 1,
    3, 2, 1, 1,
    3, 1, 1, 1,
    3, 2, 1, 2,
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  return dataStructure;
}

/**
 * @brief Compares RectGridGeom feature sizes with analytical values.
 * @param dataStructure Contains the generated size arrays.
 */
void ValidateRectGridDataStructure(const DataStructure& dataStructure)
{
  // The following values are the analytical outputs.
  // Feature element counts are 0 39 15 10.
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 39);
  REQUIRE(numElements.getValue(2) == 15);
  REQUIRE(numElements.getValue(3) == 10);
  // Feature volumes are 0.0 2362.434 352.462 15.104.
  const auto& volumes = dataStructure.getDataRefAs<Float32Array>(k_VolumesPath);
  REQUIRE(volumes.getValue(1) == Approx(2362.43384f).epsilon(k_RelativeTolerance));
  REQUIRE(volumes.getValue(2) == Approx(352.461884f).epsilon(k_RelativeTolerance));
  REQUIRE(volumes.getValue(3) == Approx(15.1039925f).epsilon(k_RelativeTolerance));
  // Equivalent diameters are 0.0 16.5242 8.76404 3.06689.
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath);
  REQUIRE(equivalentDiameters.getValue(1) == Approx(16.5241966f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(2) == Approx(8.7640428f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(3) == Approx(3.0668866f).epsilon(k_RelativeTolerance));
}
} // namespace Test

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image 2D", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create2DImageDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate2DImageDataStructure(dataStructure);

  // The geometry must not retain per-cell element sizes when storage is disabled.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);
  REQUIRE_FALSE(imageGeom.getElementSizesId().has_value());
  REQUIRE(imageGeom.getElementSizes() == nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// DREAM3D 6.5.171 calculates two-dimensional area from only the two nonflat spacings.
// Thus, four cells with nonflat spacings 2 and 3 have area 24.
// The current ImageGeom convention multiplies all three spacings and produces area 120.
// A flat spacing of 1 hides this difference, so this fixture uses flat spacing 5.
// It also rotates the flat axis to prevent an axis-specific implementation from passing.
// The `[!shouldfail]` tag records this known formula difference.
// An unexpected pass fails the test and requires review of the selected area contract.
TEST_CASE("SimplnxCore::ComputeFeatureSizes: 2D area excludes the flat-dimension spacing", "[SimplnxCore][ComputeFeatureSizes][2DFlatSpacing][!shouldfail]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  auto [label, dims, spacing] = GENERATE(std::make_tuple("flat Z", SizeVec3{2, 2, 1}, FloatVec3{2.0f, 3.0f, 5.0f}), std::make_tuple("flat X", SizeVec3{1, 2, 2}, FloatVec3{5.0f, 2.0f, 3.0f}),
                                         std::make_tuple("flat Y", SizeVec3{2, 1, 2}, FloatVec3{2.0f, 5.0f, 3.0f}));

  DYNAMIC_SECTION(label)
  {
    // All four cells belong to feature 1. The two nonflat spacings are always 2 and 3.
    // Each cell therefore has area 6, and the feature has area 24.
    // The flat-axis spacing is 5 and does not belong in the legacy area product.
    DataStructure dataStructure;
    auto* imageGeom = ImageGeom::Create(dataStructure, Test::k_ImageGeomName);
    imageGeom->setSpacing(spacing);
    imageGeom->setOrigin(FloatVec3{0.0f, 0.0f, 0.0f});
    imageGeom->setDimensions(dims);

    const ShapeType tupleShape{dims[2], dims[1], dims[0]};
    auto* cellData = AttributeMatrix::Create(dataStructure, Test::k_CellAMName, tupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellData);
    auto* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, Test::k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());
    featureIds->fill(1);
    AttributeMatrix::Create(dataStructure, Test::k_FeatureAMName, ShapeType{2}, imageGeom->getId());

    ComputeFeatureSizesFilter filter;
    Arguments args;
    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto& areas = dataStructure.getDataRefAs<Float32Array>(Test::k_VolumesPath);
    // The legacy oracle expects 24. The current slab calculation produces 120.
    REQUIRE(areas.getValue(1) == Approx(24.0f));

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image 2D with Element Sizes", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create2DImageDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(true));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate2DImageDataStructure(dataStructure);

  // The geometry must retain per-cell element sizes when storage is enabled.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);
  REQUIRE(imageGeom.getElementSizesId().has_value());
  REQUIRE(imageGeom.getElementSizes() != nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create3DImageDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate3DImageDataStructure(dataStructure);

  // The geometry must not retain per-cell element sizes when storage is disabled.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);
  REQUIRE_FALSE(imageGeom.getElementSizesId().has_value());
  REQUIRE(imageGeom.getElementSizes() == nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image_stack.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D with Element Size", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create3DImageDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(true));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate3DImageDataStructure(dataStructure);

  // The geometry must retain per-cell element sizes when storage is enabled.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);
  REQUIRE(imageGeom.getElementSizesId().has_value());
  REQUIRE(imageGeom.getElementSizes() != nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image_stack_w_element_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::CreateRectGridDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::ValidateRectGridDataStructure(dataStructure);

  // The geometry must not retain per-cell element sizes when storage is disabled.
  const auto& rectGridGeom = dataStructure.getDataRefAs<RectGridGeom>(Test::k_ImageGeomPath);
  REQUIRE_FALSE(rectGridGeom.getElementSizesId().has_value());
  REQUIRE(rectGridGeom.getElementSizes() == nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_rect_grid.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid with Element Size", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::CreateRectGridDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(true));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::ValidateRectGridDataStructure(dataStructure);

  // The geometry must retain per-cell element sizes when storage is enabled.
  const auto& rectGridGeom = dataStructure.getDataRefAs<RectGridGeom>(Test::k_ImageGeomPath);
  REQUIRE(rectGridGeom.getElementSizesId().has_value());
  REQUIRE(rectGridGeom.getElementSizes() != nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_rect_grid_w_element_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Invalid: Execution Failure", "[SimplnxCore][ComputeFeatureSizes]")
{
  // Both the in-core (Direct) and out-of-core (Scanline) paths must surface the same execution error.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create3DImageDataStructure();
  auto& featureIds = dataStructure.getDataRefAs<Int32Array>(Test::k_FeatureIdsPath);

  featureIds.setValue(61, 10);

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/invalid_execution.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Invalid: Preflight Failure", "[SimplnxCore][ComputeFeatureSizes]")
{
  DataStructure dataStructure = Test::Create3DImageDataStructure();
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);

  // A one-dimensional X extent is invalid for this filter.
  imageGeom.setDimensions(SizeVec3{std::array<usize, 3>{5, 1, 1}});
  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // A one-dimensional Y extent is invalid for this filter.
  imageGeom.setDimensions(SizeVec3{std::array<usize, 3>{1, 5, 1}});
  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // A one-dimensional Z extent is invalid for this filter.
  imageGeom.setDimensions(SizeVec3{std::array<usize, 3>{1, 1, 5}});
  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // A single-cell geometry is also invalid for this filter.
  imageGeom.setDimensions(SizeVec3{std::array<usize, 3>{1, 1, 1}});
  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/invalid_preflight.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Legacy: Small IN100 Test", "[SimplnxCore][ComputeFeatureSizes]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Load the Small IN100 input before feature-size calculation.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(Constants::k_CellData);
  DataPath cellPhasesPath = cellDataPath.createChildPath(Constants::k_Phases);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath featureGroup = smallIn100Group.createChildPath(Constants::k_CellFeatureData);
  std::string volumesName = "computed_volumes";
  std::string numElementsName = "computed_NumElements";
  std::string EquivalentDiametersName = "computed_EquivalentDiameters";

  std::vector<std::string> featureNames = {LegacyTest::k_Volumes, LegacyTest::k_EquivalentDiameters, Constants::k_NumElements};

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(featureGroup));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(volumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(numElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare each generated feature array with its exemplar array.
  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(LegacyTest::k_Volumes);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(volumesName));
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(LegacyTest::k_EquivalentDiameters);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(EquivalentDiametersName));
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(Constants::k_NumElements);
    UnitTest::CompareArrays<int32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(numElementsName));
  }

// The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/legacy_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeatureSizesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureSizesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureSizesFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureSizesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeFeatureSizesFilter::k_SaveElementSizes_Key) == true);
      CHECK(args.value<DataPath>(ComputeFeatureSizesFilter::k_GeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureSizesFilter::k_NumElementsName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureSizesFilter::k_VolumesName_Key) == "TestName");
    }
  }
}

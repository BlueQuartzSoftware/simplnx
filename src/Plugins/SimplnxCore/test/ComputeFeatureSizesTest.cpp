#include "SimplnxCore/Filters/ComputeFeatureSizesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace fs = std::filesystem;

namespace LegacyTest
{
const std::string k_Volumes("Volumes");
const std::string k_EquivalentDiameters("EquivalentDiameters");
} // namespace LegacyTest

namespace Test
{
// Geometry Level
const std::string k_ImageGeomName = "Image";
const DataPath k_ImageGeomPath = DataPath({k_ImageGeomName});

// Cell Level
const std::string k_CellAMName = "CellData";
const DataPath k_CellAMPath = k_ImageGeomPath.createChildPath(k_CellAMName);
const std::string k_FeatureIdsName = "FeatureIds";
const DataPath k_FeatureIdsPath = k_CellAMPath.createChildPath(k_FeatureIdsName);

// Feature Level
const std::string k_FeatureAMName = "FeatureData";
const DataPath k_FeatureAMPath = k_ImageGeomPath.createChildPath(k_FeatureAMName);

// Created Array Names and Paths
const std::string k_NumElementsName = "NumElements";
const DataPath k_NumElementsPath = k_FeatureAMPath.createChildPath(k_NumElementsName);
const std::string k_VolumesName = "Volumes";
const DataPath k_VolumesPath = k_FeatureAMPath.createChildPath(k_VolumesName);
const std::string k_EquivalentDiametersName = "EquivalentDiameters";
const DataPath k_EquivalentDiametersPath = k_FeatureAMPath.createChildPath(k_EquivalentDiametersName);

DataStructure Create2DImageDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{20.2f, 0.1f, 67777.1f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{5, 5, 1}});

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{5, 5, 1}, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{4}, imageGeom->getId());

  // clang-format off
  // Expected Outputs:
  // Single Voxel Area: 2.02
  // numElements: 0 11 1 13
  // areas: 0.0 22.22 2.02 26.26
  // eqDiameters: 0.0 5.319 1.6037 5.782
  const std::array<uint8, 25> featureIdsArray = {
    1, 2, 3, 3, 3,
    1, 1, 1, 1, 1,
    1, 1, 1, 3, 3,
    3, 3, 1, 1, 3,
    3, 3, 3, 3, 3,
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  return dataStructure;
}

void Validate2DImageDataStructure(const DataStructure& dataStructure)
{
  // Since we are using 20.2f the trash bits are one decimal place higher
  // this means we cant use the typical 1e-6 epsilon provided by standard library
  constexpr float32 epsilon = 0.00001;

  // Expected Outputs:
  // Single Voxel Area: 2.02
  // numElements: 0 11 1 13
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 11);
  REQUIRE(numElements.getValue(2) == 1);
  REQUIRE(numElements.getValue(3) == 13);
  // areas: 0.0 22.22 2.02 26.26
  const auto& areas = dataStructure.getDataRefAs<Float32Array>(k_VolumesPath);
  REQUIRE((areas.getValue(1) - 22.22f) < epsilon);
  REQUIRE((areas.getValue(2) - 2.02f) < epsilon);
  REQUIRE((areas.getValue(3) - 26.26f) < epsilon);
  // eqDiameters: 0.0 5.319 1.603728 5.78232
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath);
  REQUIRE((equivalentDiameters.getValue(1) - 5.319f) < epsilon);
  REQUIRE((equivalentDiameters.getValue(2) - 1.603728f) < epsilon);
  REQUIRE((equivalentDiameters.getValue(3) - 5.78232f) < epsilon);
}

DataStructure Create3DImageDataStructure()
{
  // Create an ImageGeom
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
  // Expected Outputs:
  // Single Voxel Volume: 2.268
  // numElements: 0 73 29 23
  // volumes: 0.0 165.564 65.772 52.164
  // eqDiameters: 0.0 6.813 5.008 4.636
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

void Validate3DImageDataStructure(const DataStructure& dataStructure)
{
  constexpr float32 epsilon = 0.00001;

  // Expected Outputs:
  // Single Voxel Volume: 2.268
  // numElements: 0 73 29 23
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 73);
  REQUIRE(numElements.getValue(2) == 29);
  REQUIRE(numElements.getValue(3) == 23);
  // volumes: 0.0 165.564 65.772 52.164
  const auto& volumes = dataStructure.getDataRefAs<Float32Array>(k_VolumesPath);
  REQUIRE((volumes.getValue(1) - 165.564f) < std::numeric_limits<float32>::epsilon());
  REQUIRE((volumes.getValue(2) - 65.772f) < std::numeric_limits<float32>::epsilon());
  REQUIRE((volumes.getValue(3) - 52.164f) < std::numeric_limits<float32>::epsilon());
  // eqDiameters: 0.0 6.81275 5.00819 4.63579
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath);
  REQUIRE((equivalentDiameters.getValue(1) - 6.81275f) < std::numeric_limits<float32>::epsilon());
  REQUIRE((equivalentDiameters.getValue(2) - 5.00819f) < std::numeric_limits<float32>::epsilon());
  REQUIRE((equivalentDiameters.getValue(3) - 4.63579f) < epsilon);
}

DataStructure CreateRectGridDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  RectGridGeom* rectGridGeom = RectGridGeom::Create(dataStructure, k_ImageGeomName);
  rectGridGeom->setDimensions(SizeVec3{std::array<usize, 3>{4, 4, 4}});

  // xBounds -> 0.0f, 0.6f, 0.9f, 2.1f, 13.0f
  Float32Array* xBoundsArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "xBounds", ShapeType{5}, ShapeType{1}, rectGridGeom->getId());
  xBoundsArray->setValue(0, 0.0f);
  xBoundsArray->setValue(1, 0.6f);
  xBoundsArray->setValue(2, 0.9f);
  xBoundsArray->setValue(3, 2.1f);
  xBoundsArray->setValue(4, 13.0f);
  rectGridGeom->setXBoundsId(xBoundsArray->getId());

  // yBounds -> 0.0f, 0.1f, 1.0f, 10.0f, 100.0f
  Float32Array* yBoundsArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "yBounds", ShapeType{5}, ShapeType{1}, rectGridGeom->getId());
  yBoundsArray->setValue(0, 0.0f);
  yBoundsArray->setValue(1, 0.1f);
  yBoundsArray->setValue(2, 1.0f);
  yBoundsArray->setValue(3, 10.0f);
  yBoundsArray->setValue(4, 100.0f);
  rectGridGeom->setYBoundsId(yBoundsArray->getId());

  // zBounds -> 0.0f, 1.0f, 1.2f, 2.0f, 2.1f
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
  // Expected Outputs:
  // numElements: 0 39 15 10
  // volumes: 0.0 2358.834 352.462 15.104
  // eqDiameters: 0.0 16.516 8.764 3.0994
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
} // namespace Test

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image 2D", "[SimplnxCore][ComputeFeatureSizes]")
{
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate2DImageDataStructure(dataStructure);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image 2D with Element Sizes", "[SimplnxCore][ComputeFeatureSizes]")
{
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate2DImageDataStructure(dataStructure);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D", "[SimplnxCore][ComputeFeatureSizes]")
{
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate3DImageDataStructure(dataStructure);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image_stack.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D with Element Size", "[SimplnxCore][ComputeFeatureSizes]")
{
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate3DImageDataStructure(dataStructure);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image_stack_w_elemnt_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid", "[SimplnxCore][ComputeFeatureSizes]")
{
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  // numElements: 0 39 15 10
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(Test::k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 39);
  REQUIRE(numElements.getValue(2) == 15);
  REQUIRE(numElements.getValue(3) == 10);
  // volumes: 0.0 2358.834 356.062 15.104
  const auto& volumes = dataStructure.getDataRefAs<Float32Array>(Test::k_VolumesPath);
  REQUIRE(volumes.getValue(1) == 2358.834f);
  REQUIRE(volumes.getValue(2) == 356.062f);
  REQUIRE(volumes.getValue(3) == 15.104f);
  // eqDiameters: 0.0 16.516 8.764 3.0994
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(Test::k_EquivalentDiametersPath);
  REQUIRE(equivalentDiameters.getValue(1) == 16.516f);
  REQUIRE(equivalentDiameters.getValue(2) == 8.764f);
  REQUIRE(equivalentDiameters.getValue(3) == 3.0994f);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_rect_grid.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid with Element Size", "[SimplnxCore][ComputeFeatureSizes]")
{
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  // numElements: 0 39 15 10
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(Test::k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 39);
  REQUIRE(numElements.getValue(2) == 15);
  REQUIRE(numElements.getValue(3) == 10);
  // volumes: 0.0 2358.834 352.462 15.59
  const auto& volumes = dataStructure.getDataRefAs<Float32Array>(Test::k_VolumesPath);
  REQUIRE(volumes.getValue(1) == 2358.834f);
  REQUIRE(volumes.getValue(2) == 352.462f);
  REQUIRE(volumes.getValue(3) == 15.59f);
  // eqDiameters: 0.0 16.516 8.764 3.0994
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(Test::k_EquivalentDiametersPath);
  REQUIRE(equivalentDiameters.getValue(1) == 16.516f);
  REQUIRE(equivalentDiameters.getValue(2) == 8.764f);
  REQUIRE(equivalentDiameters.getValue(3) == 3.0994f);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_rect_grid_w_elemnt_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Invalid: Execution Failure", "[SimplnxCore][ComputeFeatureSizes]")
{
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/invalid_execution.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Invalid: Preflight Failure", "[SimplnxCore][ComputeFeatureSizes]")
{
  DataStructure dataStructure = Test::Create3DImageDataStructure();
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);

  // Set first invalid dimensions
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // Set second invalid dimensions
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // Set third invalid dimensions
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // Set fourth invalid dimensions
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/invalid_preflight.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Legacy: Small IN100 Test", "[SimplnxCore][ComputeFeatureSizes]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare Outputs
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

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/legacy_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/CreateImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/RegularGridSampleSurfaceMeshFilter.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_TriGeomName = "Input Triangle Geometry";
const DataPath k_TriGeomPath = DataPath({k_TriGeomName});
const DataPath k_FaceLabelsPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath(Constants::k_FaceLabels);

const std::string k_ExemplarImageGeomName = "Exemplar Sample Triangle Geometry on Regular Grid";
const DataPath k_ExemplarImageGeomPath = DataPath({k_ExemplarImageGeomName});
const DataPath k_ExemplarFeatureIdsPath = k_ExemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);

const DataPath k_GeneratedImageGeomPath = DataPath({Constants::k_ImageGeometry});
const DataPath k_GeneratedCellDataPath = k_GeneratedImageGeomPath.createChildPath(Constants::k_CellData);
const DataPath k_GeneratedFeatureIdsPath = k_GeneratedCellDataPath.createChildPath(Constants::k_FeatureIds);

const std::vector<uint64> k_Dims{171, 200, 150};
const std::vector<float32> k_Origin{1.0f, 1.99f, 0.0f};
const std::vector<float32> k_Spacing{0.1f, 0.1f, 0.02f};

/**
 * @brief Builds vertical quads with two triangles and two Face Labels per quad.
 * @param dataStructure Receives the Triangle Geometry and its arrays.
 * @param positions Specifies each quad's X coordinate.
 * @param labels Specifies the pair of Face Labels for each quad.
 */
void BuildVerticalQuads(DataStructure& dataStructure, const std::vector<float32>& positions, const std::vector<std::array<int32, 2>>& labels)
{
  REQUIRE(positions.size() == labels.size());
  auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, k_TriGeomName);
  REQUIRE(triangleGeomPtr != nullptr);
  const usize numQuads = positions.size();
  const auto verticesPath = k_TriGeomPath.createChildPath("Vertices");
  const auto facesPath = k_TriGeomPath.createChildPath("Faces");
  auto* verticesPtr = Float32Array::Create(dataStructure, "Vertices", DataStoreUtilities::CreateDataStore<float32>(dataStructure, verticesPath, {numQuads * 4}, {3}), triangleGeomPtr->getId());
  auto* facesPtr = IGeometry::MeshIndexArrayType::Create(dataStructure, "Faces", DataStoreUtilities::CreateDataStore<IGeometry::MeshIndexType>(dataStructure, facesPath, {numQuads * 2}, {3}),
                                                         triangleGeomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  triangleGeomPtr->setVertices(*verticesPtr);
  triangleGeomPtr->setFaceList(*facesPtr);
  auto* faceDataPtr = AttributeMatrix::Create(dataStructure, Constants::k_FaceData, ShapeType{numQuads * 2}, triangleGeomPtr->getId());
  REQUIRE(faceDataPtr != nullptr);
  triangleGeomPtr->setFaceAttributeMatrix(*faceDataPtr);
  auto* labelsPtr = Int32Array::Create(dataStructure, Constants::k_FaceLabels, DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FaceLabelsPath, {numQuads * 2}, {2}), faceDataPtr->getId());
  REQUIRE(labelsPtr != nullptr);
  for(usize quadIdx = 0; quadIdx < numQuads; ++quadIdx)
  {
    const float32 x = positions[quadIdx];
    const std::array<float32, 12> coords = {x, 0.0F, 0.0F, x, 1.0F, 0.0F, x, 1.0F, 1.0F, x, 0.0F, 1.0F};
    const std::array<IGeometry::MeshIndexType, 6> indices = {0, 1, 2, 0, 2, 3};
    for(usize compIdx = 0; compIdx < coords.size(); ++compIdx)
    {
      (*verticesPtr)[quadIdx * 12 + compIdx] = coords[compIdx];
    }
    for(usize compIdx = 0; compIdx < indices.size(); ++compIdx)
    {
      (*facesPtr)[quadIdx * 6 + compIdx] = quadIdx * 4 + indices[compIdx];
    }
    for(usize compIdx = 0; compIdx < 4; ++compIdx)
    {
      (*labelsPtr)[quadIdx * 4 + compIdx] = labels[quadIdx][compIdx % 2];
    }
  }
}

Arguments MakeSmallGridArguments()
{
  Arguments args;
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{5, 1, 1}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0, 0, 0}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriGeomPath));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_GeneratedImageGeomPath));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));
  return args;
}

template <typename T>
void RequireSmallGridFeatureIds(const DataStructure& dataStructure)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_GeneratedFeatureIdsPath));
  const auto& featureIdsRef = dataStructure.getDataRefAs<DataArray<T>>(k_GeneratedFeatureIdsPath);
  const std::vector<T> actual(featureIdsRef.begin(), featureIdsRef.end());
  const std::vector<T> expected = {0, 2, 2, 2, 0};
  REQUIRE(actual == expected);
}

} // namespace

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Reject nonpositive or nonfinite spacing when creating geometry", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  const auto axis = GENERATE(usize{0}, usize{1}, usize{2});
  const auto invalidSpacing = GENERATE(0.0F, -1.0F, std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity());
  CAPTURE(axis, invalidSpacing);
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  auto validResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(validResult.outputActions);

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Existing Image Geometry");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({5, 1, 1});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, Constants::k_CellData, ShapeType{1, 1, 5}, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);

  VectorFloat32Parameter::ValueType spacing{1.0F, 1.0F, 1.0F};
  spacing[axis] = invalidSpacing;
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, spacing);
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key,
                      std::make_any<ChoicesParameter::ValueType>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, DataPath({"Existing Image Geometry"}));
  auto existingResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(existingResult.outputActions);

  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key,
                      std::make_any<ChoicesParameter::ValueType>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -11802);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Reject Face Labels outside Face Attribute Matrix", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  auto* otherFaceDataPtr = AttributeMatrix::Create(dataStructure, "Other Face Data", ShapeType{4});
  REQUIRE(otherFaceDataPtr != nullptr);
  const DataPath otherLabelsPath({"Other Face Data", "Face Labels"});
  auto* otherLabelsPtr = Int32Array::Create(dataStructure, "Face Labels", DataStoreUtilities::CreateDataStore<int32>(dataStructure, otherLabelsPath, {4}, {2}), otherFaceDataPtr->getId());
  REQUIRE(otherLabelsPtr != nullptr);
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, otherLabelsPath);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -11803);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Reject missing Face Attribute Matrix without throwing", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, "Without Face Data");
  REQUIRE(triangleGeomPtr != nullptr);
  auto* sourceGeomPtr = dataStructure.getDataAs<TriangleGeom>(k_TriGeomPath);
  REQUIRE(sourceGeomPtr != nullptr);
  triangleGeomPtr->setVertices(*sourceGeomPtr->getVertices());
  triangleGeomPtr->setFaceList(*sourceGeomPtr->getFaces());
  REQUIRE(triangleGeomPtr->getFaceAttributeMatrix() == nullptr);
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, DataPath({"Without Face Data"}));
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = filter.preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -11803);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter:CreateImageGeom", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "sample_surface_mesh_regular_grid_v1.tar.gz", "sample_surface_mesh_regular_grid_v1");
  auto baseDataFilePath = fs::path(fmt::format("{}/sample_surface_mesh_regular_grid_v1/sample_surface_mesh_regular_grid_v1.dream3d", unit_test::k_TestFilesDir));

  std::string sectionTitle = "Exemplar Image Geometry Face Labels Create";
  SECTION(sectionTitle)
  {
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(k_Dims));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Origin));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Spacing));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(6ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath exemplarImageGeomPath({sectionTitle});
    UnitTest::CompareImageGeometry(dataStructure, exemplarImageGeomPath, ::k_GeneratedImageGeomPath);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds"), ::k_GeneratedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "Exemplar Image Geometry Part Numbers int32 Create";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber");

    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(k_Dims));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Origin));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Spacing));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(6ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath exemplarImageGeomPath({sectionTitle});
    UnitTest::CompareImageGeometry(dataStructure, exemplarImageGeomPath, ::k_GeneratedImageGeomPath);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds"), ::k_GeneratedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "Exemplar Image Geometry Part Numbers uint8 Create";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber (uint8)");

    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(k_Dims));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Origin));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Spacing));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(6ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath exemplarImageGeomPath({sectionTitle});
    UnitTest::CompareImageGeometry(dataStructure, exemplarImageGeomPath, ::k_GeneratedImageGeomPath);
    UnitTest::CompareArrays<uint8>(dataStructure, exemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds"), ::k_GeneratedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "Exemplar Image Geometry Part Numbers float32 Create";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber (float32)");

    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(k_Dims));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Origin));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Spacing));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(6ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter:ExistingImageGeom", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "sample_surface_mesh_regular_grid_v1.tar.gz", "sample_surface_mesh_regular_grid_v1");
  auto baseDataFilePath = fs::path(fmt::format("{}/sample_surface_mesh_regular_grid_v1/sample_surface_mesh_regular_grid_v1.dream3d", unit_test::k_TestFilesDir));

  DataPath exemplarImageGeometryPath({"Exemplar Image Geometry Existing"});
  std::string sectionTitle = "FeatureIds (FaceLabels)";
  SECTION(sectionTitle)
  {
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(exemplarImageGeometryPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
    DataPath exemplarFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath(sectionTitle);
    DataPath computedFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds");
    UnitTest::CompareArrays<int32>(dataStructure, exemplarFeatureIdsPath, computedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "FeatureIds (PartNumber)";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber");
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(exemplarImageGeometryPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
    DataPath exemplarFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath(sectionTitle);
    DataPath computedFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds");
    UnitTest::CompareArrays<int32>(dataStructure, exemplarFeatureIdsPath, computedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "FeatureIds (PartNumber uint8)";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber (uint8)");
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(exemplarImageGeometryPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
    DataPath exemplarFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath(sectionTitle);
    DataPath computedFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds");
    UnitTest::CompareArrays<uint8>(dataStructure, exemplarFeatureIdsPath, computedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RegularGridSampleSurfaceMeshFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RegularGridSampleSurfaceMeshFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RegularGridSampleSurfaceMeshFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<ChoicesParameter::ValueType>(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key) == 0);
        // Complex type (UInt64ToVec3FilterParameterConverter) - verified by successful pipeline loading
      }
      CHECK(args.value<DataPath>(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key) == "TestName");
      CHECK(args.value<std::string>(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Custom Output Type", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  const auto outputType = GENERATE(DataType::uint8, DataType::int64);
  CAPTURE(outputType);
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, true);
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(outputType)));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_GeneratedFeatureIdsPath));
  REQUIRE(dataStructure.getDataRefAs<IDataArray>(k_GeneratedFeatureIdsPath).getDataType() == outputType);
  if(outputType == DataType::uint8)
  {
    RequireSmallGridFeatureIds<uint8>(dataStructure);
  }
  else
  {
    RequireSmallGridFeatureIds<int64>(dataStructure);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Output type ignored when Use Custom Output Type is off", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, false);
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DataType::uint8)));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_FaceLabelsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_GeneratedFeatureIdsPath));
  REQUIRE(dataStructure.getDataRefAs<IDataArray>(k_GeneratedFeatureIdsPath).getDataType() == dataStructure.getDataRefAs<IDataArray>(k_FaceLabelsPath).getDataType());
  RequireSmallGridFeatureIds<int32>(dataStructure);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Custom Output Type value out of range", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 300}, {0, 300}});
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, true);
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DataType::uint8)));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors()[0].code == -11801);
  REQUIRE(result.result.errors()[0].message.find("300") != std::string::npos);
  REQUIRE(result.result.errors()[0].message.find("uint8") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Exterior -1 Labels", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  SECTION("A: exterior and Feature 2")
  {
    BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{-1, 2}, {-1, 2}});
  }
  SECTION("B: exterior and Feature 0 before Feature 2")
  {
    BuildVerticalQuads(dataStructure, {0.2F, 1.2F, 3.7F}, {{-1, 0}, {0, 2}, {0, 2}});
  }
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  RequireSmallGridFeatureIds<int32>(dataStructure);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Use Custom Output Type parameter layout", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  RegularGridSampleSurfaceMeshFilter filter;
  const auto params = filter.parameters();
  SECTION("Order")
  {
    const auto keys = params.getKeys();
    const auto customIter = std::find(keys.begin(), keys.end(), RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key);
    REQUIRE(customIter != keys.end());
    REQUIRE(std::next(customIter) != keys.end());
    REQUIRE(*std::next(customIter) == RegularGridSampleSurfaceMeshFilter::k_OutputType_Key);
  }
  SECTION("Link")
  {
    const auto& groups = params.getLinkedGroups(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key);
    REQUIRE(groups.size() == 1);
    REQUIRE(groups[0].first == RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key);
    REQUIRE(std::any_cast<bool>(groups[0].second));
    Arguments args;
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, false);
    REQUIRE_FALSE(params.isParameterActive(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, args));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, true);
    REQUIRE(params.isParameterActive(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, args));
  }
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Nested single label boxes", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter][L24]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  // Two closed boxes: [1, 6] x [0, 3] x [0, 3] and [2, 4] x [1, 2] x [1, 2].
  auto* geom = TriangleGeom::Create(dataStructure, k_TriGeomName);
  REQUIRE(geom != nullptr);
  auto* vertices = Float32Array::Create(dataStructure, "Vertices", DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_TriGeomPath.createChildPath("Vertices"), {16}, {3}), geom->getId());
  auto* faces = IGeometry::MeshIndexArrayType::Create(dataStructure, "Faces",
                                                      DataStoreUtilities::CreateDataStore<IGeometry::MeshIndexType>(dataStructure, k_TriGeomPath.createChildPath("Faces"), {24}, {3}), geom->getId());
  REQUIRE(vertices != nullptr);
  REQUIRE(faces != nullptr);
  geom->setVertices(*vertices);
  geom->setFaceList(*faces);
  auto* faceData = AttributeMatrix::Create(dataStructure, Constants::k_FaceData, ShapeType{24}, geom->getId());
  REQUIRE(faceData != nullptr);
  geom->setFaceAttributeMatrix(*faceData);
  auto* labels = Int32Array::Create(dataStructure, Constants::k_FaceLabels, DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FaceLabelsPath, {24}, {1}), faceData->getId());
  REQUIRE(labels != nullptr);
  const std::array<float32, 48> coords = {1, 0, 0, 6, 0, 0, 6, 3, 0, 1, 3, 0, 1, 0, 3, 6, 0, 3, 6, 3, 3, 1, 3, 3, 2, 1, 1, 4, 1, 1, 4, 2, 1, 2, 2, 1, 2, 1, 2, 4, 1, 2, 4, 2, 2, 2, 2, 2};
  const std::array<usize, 36> indices = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5, 2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7};
  for(usize i = 0; i < coords.size(); ++i)
  {
    (*vertices)[i] = coords[i];
  }
  for(usize box = 0; box < 2; ++box)
  {
    for(usize i = 0; i < indices.size(); ++i)
    {
      (*faces)[box * 36 + i] = box * 8 + indices[i];
    }
    for(usize i = 0; i < 12; ++i)
    {
      (*labels)[box * 12 + i] = static_cast<int32>(box + 1);
    }
  }
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{7, 3, 3}));
  RegularGridSampleSurfaceMeshFilter filter;
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_GeneratedFeatureIdsPath);
  // Cell centers are half-integers. Only the middle Y row of the middle Z slice intersects the inner box.
  const std::vector<int32> expected = {0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 2, 2,
                                       1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0};
  const std::vector<int32> actual(featureIds.begin(), featureIds.end());
  REQUIRE(actual == expected);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Touching and overlapping single label boxes", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter][L24]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* geom = TriangleGeom::Create(dataStructure, k_TriGeomName);
  auto* vertices = Float32Array::Create(dataStructure, "Vertices", DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_TriGeomPath.createChildPath("Vertices"), {24}, {3}), geom->getId());
  auto* faces = IGeometry::MeshIndexArrayType::Create(dataStructure, "Faces",
                                                      DataStoreUtilities::CreateDataStore<IGeometry::MeshIndexType>(dataStructure, k_TriGeomPath.createChildPath("Faces"), {36}, {3}), geom->getId());
  geom->setVertices(*vertices);
  geom->setFaceList(*faces);
  auto* faceData = AttributeMatrix::Create(dataStructure, Constants::k_FaceData, ShapeType{36}, geom->getId());
  geom->setFaceAttributeMatrix(*faceData);
  auto* labels = Int32Array::Create(dataStructure, Constants::k_FaceLabels, DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FaceLabelsPath, {36}, {1}), faceData->getId());
  // All boxes have y in [0,1] and z in [0,3]. Each row samples y = z = 0.5.
  std::array<std::array<float32, 3>, 3> boxes;
  std::vector<int32> expected;
  SECTION("Touching parts with B entry before A exit")
  {
    // Face order B, A, C puts B's entry first at the shared x = 3 boundary.
    boxes = {{{3, 5, 2}, {1, 3, 1}, {6, 8, 3}}};
    // A occupies centers 1.5, 2.5; B 3.5, 4.5; C 6.5, 7.5.
    expected = {0, 1, 1, 2, 2, 0, 3, 3, 0, 0};
  }
  SECTION("Overlapping parts close below the stack top")
  {
    boxes = {{{1, 4, 1}, {3, 6, 2}, {7, 9, 3}}};
    // Crossings: 1 opens A, 3 opens B, 4 closes A below B, 6 closes B,
    // 7 opens C, 9 closes C. The most recently opened remaining part wins.
    expected = {0, 1, 1, 2, 2, 2, 0, 3, 3, 0, 0};
  }
  const std::array<usize, 36> indices = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5, 2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7};
  for(usize b = 0; b < 3; ++b)
  {
    const float32 x0 = boxes[b][0];
    const float32 x1 = boxes[b][1];
    const std::array<float32, 24> c = {x0, 0, 0, x1, 0, 0, x1, 1, 0, x0, 1, 0, x0, 0, 3, x1, 0, 3, x1, 1, 3, x0, 1, 3};
    for(usize i = 0; i < 24; ++i)
    {
      (*vertices)[b * 24 + i] = c[i];
    }
    for(usize i = 0; i < 36; ++i)
    {
      (*faces)[b * 36 + i] = b * 8 + indices[i];
    }
    for(usize i = 0; i < 12; ++i)
    {
      (*labels)[b * 12 + i] = static_cast<int32>(boxes[b][2]);
    }
  }
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{static_cast<uint64>(expected.size()), 1, 1}));
  RegularGridSampleSurfaceMeshFilter filter;
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_GeneratedFeatureIdsPath);
  const std::vector<int32> actual(featureIds.begin(), featureIds.end());
  REQUIRE(actual == expected);
}

namespace
{
void CreateSamplingTetrahedron(DataStructure& ds)
{
  auto result = CreateTriangleGeometryAction(DataPath({"Mesh"}), 4, 4, "Vertex Data", "Face Data", "Vertices", "Faces").apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE_NOTHROW(ds.getDataRefAs<TriangleGeom>(DataPath({"Mesh"})));
  auto& geom = ds.getDataRefAs<TriangleGeom>(DataPath({"Mesh"}));
  const std::array<float32, 12> coords = {0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4};
  const std::array<uint64, 12> faces = {0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3};
  auto write = geom.getVerticesRef().getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(coords));
  SIMPLNX_RESULT_REQUIRE_VALID(write);
  auto faceWrite = geom.getFacesRef().getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint64>(faces));
  SIMPLNX_RESULT_REQUIRE_VALID(faceWrite);
  auto labels = CreateArrayAction(DataType::int32, {4}, {2}, DataPath({"Mesh", "Face Data", "Labels"})).apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(labels);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Face Data", "Labels"})));
  auto& array = ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Face Data", "Labels"}));
  for(usize i = 0; i < 4; ++i)
  {
    array[2 * i] = 1;
    array[2 * i + 1] = 0;
  }
}
} // namespace

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Spatial parameter domains", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter][SpatialParameterDomains]")
{

  DataStructure ds;
  CreateSamplingTetrahedron(ds);
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, DataPath({"Mesh"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, DataPath({"Mesh", "Face Data", "Labels"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, DataPath({"Grid"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::vector<uint64>{1, 1, 1});
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{0, 0, 0});
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::vector<float32>{1, 1, 1});
  int32 expectedCode;
  std::string parameter;
  const auto axis = GENERATE(0, 1, 2);
  const auto value = GENERATE(std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
  CAPTURE(axis, value);
  std::vector<float32> values = {0, 0, 0};
  values[axis] = value;
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, values);
  expectedCode = -86112;
  parameter = "Origin";

  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == expectedCode);
  CHECK(result.outputActions.errors()[0].message.find(parameter) != std::string::npos);

  CHECK(result.outputActions.errors()[0].message.find(fmt::format("{}", value)) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find(std::string(1, "XYZ"[axis])) != std::string::npos);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Sampling boundary controls", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter][SpatialSamplingControls]")
{

  DataStructure ds;
  CreateSamplingTetrahedron(ds);
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, DataPath({"Mesh"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, DataPath({"Mesh", "Face Data", "Labels"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, DataPath({"Grid"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::vector<uint64>{1, 1, 1});
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{0, 0, 0});
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::vector<float32>{1, 1, 1});
  auto geometry = CreateImageGeometryAction(DataPath({"Existing"}), {1, 1, 1}, {0, 0, 0}, {1, 1, 1}, "Cell Data").apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(geometry);
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, uint64{1});
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, DataPath({"Existing"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{});
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::vector<float32>{});
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Existing", "Cell Data", "Feature Ids"})));
  CHECK(ds.getDataRefAs<Int32Array>(DataPath({"Existing", "Cell Data", "Feature Ids"}))[0] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Origin vector length", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{

  DataStructure ds;
  CreateSamplingTetrahedron(ds);
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, DataPath({"Mesh"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, DataPath({"Mesh", "Face Data", "Labels"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, DataPath({"Grid"}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::vector<uint64>{1, 1, 1});
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{0, 0, 0});
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::vector<float32>{1, 1, 1});
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{0, 0});
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == -1);
  CHECK(filter.parameters().at(RegularGridSampleSurfaceMeshFilter::k_Origin_Key)->humanName() == "Origin");
  CHECK(result.outputActions.errors()[0].message == "VectorParameter requires size 3. Received size 2.");
}

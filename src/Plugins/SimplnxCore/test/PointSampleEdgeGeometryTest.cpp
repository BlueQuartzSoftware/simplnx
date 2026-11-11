#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/PointSampleEdgeGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_ScanVectorGeometryPath = DataPath({"ALSAM Scan"});
const DataPath k_PowerArrayPath = k_ScanVectorGeometryPath.createChildPath("Segments").createChildPath("Traveler - Power");
const DataPath k_SliceIdArrayPath = k_ScanVectorGeometryPath.createChildPath("Segments").createChildPath("LayerNumber");
const DataPath k_SampledVertexGeomPath = DataPath({"Sampled Vertex Geometry"});
const DataPath k_SampledVertexAMPath = k_SampledVertexGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName);
const DataPath k_ExemplarySampledVertexGeomPath = DataPath({"Exemplary Sampled Vertex Geometry"});
} // namespace

TEST_CASE("SimplnxCore::PointSampleEdgeGeometryFilter: Valid Filter Execution", "[SimplnxCore][PointSampleEdgeGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "point_sample_edge_geometry.tar.gz", "point_sample_edge_geometry");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/point_sample_edge_geometry/point_sample_edge_geometry.dream3d", unit_test::k_TestFilesDir)));
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/point_sample_edge_geometry/point_sample_edge_geometry.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter and an Arguments Object
  PointSampleEdgeGeometryFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorSamplingRes_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorGeometryPath_Key, std::make_any<DataPath>(k_ScanVectorGeometryPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CalculateCumulativeSampleDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SampledVertexGeometryPath_Key, std::make_any<DataPath>(k_SampledVertexGeomPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_EdgeIdsArrayName_Key, std::make_any<std::string>("Edge Ids"));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CumulativeSampleDistanceArrayName_Key, std::make_any<std::string>("Cumulative Sample Distance"));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({k_PowerArrayPath, k_SliceIdArrayPath}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_SampledVertexAMPath, k_ExemplarySampledVertexGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PointSampleEdgeGeometryFilter: Invalid Filter Execution", "[SimplnxCore][PointSampleEdgeGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "point_sample_edge_geometry.tar.gz", "point_sample_edge_geometry");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/point_sample_edge_geometry/point_sample_edge_geometry.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  PointSampleEdgeGeometryFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorSamplingRes_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorGeometryPath_Key, std::make_any<DataPath>(k_ScanVectorGeometryPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CalculateCumulativeSampleDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SampledVertexGeometryPath_Key, std::make_any<DataPath>(k_SampledVertexGeomPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_EdgeIdsArrayName_Key, std::make_any<std::string>("Edge Ids"));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CumulativeSampleDistanceArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({k_PowerArrayPath, k_SliceIdArrayPath}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include <limits>

namespace
{
void CreateSamplingEdges(DataStructure& ds, bool squares = false)
{
  auto result = CreateEdgeGeometryAction(DataPath({"Edges"}), squares ? 8 : 1, squares ? 8 : 2, "Vertex Data", "Edge Data", "Vertices", "Edges").apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE_NOTHROW(ds.getDataRefAs<EdgeGeom>(DataPath({"Edges"})));
  auto& geom = ds.getDataRefAs<EdgeGeom>(DataPath({"Edges"}));
  auto& vertices = geom.getVerticesRef();
  auto& edges = geom.getEdgesRef();
  if(squares)
  {
    const std::array<float32, 24> coords = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1};
    auto write = vertices.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(coords));
    SIMPLNX_RESULT_REQUIRE_VALID(write);
    for(usize i = 0; i < 8; ++i)
    {
      edges[2 * i] = i;
      edges[2 * i + 1] = (i / 4) * 4 + (i + 1) % 4;
    }
    for(const auto& name : {"Layers", "Regions"})
    {
      auto array = CreateArrayAction(DataType::int32, {8}, {1}, DataPath({"Edges", "Edge Data", name})).apply(ds, IDataAction::Mode::Execute);
      SIMPLNX_RESULT_REQUIRE_VALID(array);
      REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Edges", "Edge Data", name})));
      auto& values = ds.getDataRefAs<Int32Array>(DataPath({"Edges", "Edge Data", name}));
      for(usize i = 0; i < 8; ++i)
      {
        values[i] = std::string(name) == "Layers" ? static_cast<int32>(i / 4) : 0;
      }
    }
  }
  else
  {
    const std::array<float32, 6> coords = {0, 0, 0, 2, 0, 0};
    auto write = vertices.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(coords));
    SIMPLNX_RESULT_REQUIRE_VALID(write);
    edges[0] = 0;
    edges[1] = 1;
  }
}
} // namespace

TEST_CASE("SimplnxCore::PointSampleEdgeGeometryFilter: Spatial parameter domains", "[SimplnxCore][PointSampleEdgeGeometryFilter][SpatialParameterDomains]")
{

  DataStructure ds;
  CreateSamplingEdges(ds);
  PointSampleEdgeGeometryFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorGeometryPath_Key, DataPath({"Edges"}));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CalculateCumulativeSampleDistance_Key, false);
  const auto value = GENERATE(0.0F, -1.0F, std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
  CAPTURE(value);
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorSamplingRes_Key, value);
  const int32 expectedCode = -86102;
  const std::string parameter = "Sampling Spacing (mm)";

  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == expectedCode);
  CHECK(result.outputActions.errors()[0].message.find(parameter) != std::string::npos);

  CHECK(result.outputActions.errors()[0].message.find(fmt::format("{}", value)) != std::string::npos);
}

TEST_CASE("SimplnxCore::PointSampleEdgeGeometryFilter: Checked final sample counts", "[SimplnxCore][PointSampleEdgeGeometryFilter][SpatialSamplingControls]")
{
  DataStructure ds;
  CreateSamplingEdges(ds);
  PointSampleEdgeGeometryFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorGeometryPath_Key, DataPath({"Edges"}));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CalculateCumulativeSampleDistance_Key, false);
  const bool excessive = GENERATE(false, true);
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorSamplingRes_Key, excessive ? std::ldexp(1.0F, -30) : 1.0F);
  auto result = filter.execute(ds, args);
  if(excessive)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    CHECK(result.result.errors()[0].code == -86124);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE_NOTHROW(ds.getDataRefAs<VertexGeom>(DataPath({"Sampled Vertex Geometry"})));
    const auto& geom = ds.getDataRefAs<VertexGeom>(DataPath({"Sampled Vertex Geometry"}));
    REQUIRE(geom.getNumberOfVertices() == 3);
    for(usize idx = 0; idx < 3; ++idx)
    {
      CHECK(geom.getVerticesRef()[3 * idx] == Approx((2 * idx + 1) / 3.0F).margin(1e-6F));
      CHECK(geom.getVerticesRef()[3 * idx + 1] == 0);
      CHECK(geom.getVerticesRef()[3 * idx + 2] == 0);
    }
    UnitTest::CheckArraysInheritTupleDims(ds);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<EdgeGeom>(DataPath({"Edges"})));
  const auto& source = ds.getDataRefAs<EdgeGeom>(DataPath({"Edges"}));
  CHECK(source.getNumberOfVertices() == 2);
  CHECK(source.getVerticesRef()[3] == 2);
}

#include "SimplnxCore/Filters/Algorithms/PointSampleEdgeGeometry.hpp"
#include <cmath>

TEST_CASE("SimplnxCore::PointSampleEdgeGeometryFilter: Sample size arithmetic", "[SimplnxCore][PointSampleEdgeGeometryFilter][SpatialSamplingControls]")
{
  const auto maximum = std::numeric_limits<usize>::max();
  SECTION("Aggregate overflow")
  {
    auto result = detail::AddSampleCounts(maximum, 1);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors()[0].code == -86125);
    CHECK(result.errors()[0].message.find("aggregate sample count") != std::string::npos);
    CHECK(result.errors()[0].message.find(std::to_string(maximum)) != std::string::npos);
    CHECK(result.errors()[0].message.find("increment=1") != std::string::npos);
  }
  SECTION("Index overflow")
  {
    auto result = detail::CheckSampleIndexCount(usize{1} << 63);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors()[0].code == -86125);
    CHECK(result.errors()[0].message.find("sample index count") != std::string::npos);
    CHECK(result.errors()[0].message.find(std::to_string(usize{1} << 63)) != std::string::npos);
    CHECK(result.errors()[0].message.find(std::to_string(std::numeric_limits<int64>::max())) != std::string::npos);
  }
  SECTION("Value overflow")
  {
    auto result = detail::CheckSampleArraySize(usize{1} << 62, 4, 1, "TestValues");
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors()[0].code == -86125);
    CHECK(result.errors()[0].message.find("sample value count") != std::string::npos);
    CHECK(result.errors()[0].message.find("TestValues") != std::string::npos);
    CHECK(result.errors()[0].message.find(std::to_string(usize{1} << 62)) != std::string::npos);
    CHECK(result.errors()[0].message.find("components=4") != std::string::npos);
  }
  SECTION("Byte overflow")
  {
    auto result = detail::CheckSampleArraySize(usize{1} << 61, 1, 8, "TestEdgeIds");
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors()[0].code == -86125);
    CHECK(result.errors()[0].message.find("sample byte count") != std::string::npos);
    CHECK(result.errors()[0].message.find("TestEdgeIds") != std::string::npos);
    CHECK(result.errors()[0].message.find(std::to_string(usize{1} << 61)) != std::string::npos);
    CHECK(result.errors()[0].message.find("element bytes=8") != std::string::npos);
  }
  SECTION("Representable boundaries")
  {
    auto result = detail::AddSampleCounts(maximum - 1, 1);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(result.value() == maximum);
    auto size = detail::CheckSampleArraySize(maximum / 12, 3, 4, "Vertices");
    SIMPLNX_RESULT_REQUIRE_VALID(size);
  }
}

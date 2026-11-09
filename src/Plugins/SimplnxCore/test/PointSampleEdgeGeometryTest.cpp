#include "SimplnxCore/Filters/PointSampleEdgeGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <filesystem>
#include <limits>
#include <string>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const DataPath k_ScanVectorGeometryPath = DataPath({"ALSAM Scan"});
const DataPath k_PowerArrayPath = k_ScanVectorGeometryPath.createChildPath("Segments").createChildPath("Traveler - Power");
const DataPath k_SliceIdArrayPath = k_ScanVectorGeometryPath.createChildPath("Segments").createChildPath("LayerNumber");
const DataPath k_SampledVertexGeomPath = DataPath({"Sampled Vertex Geometry"});
const DataPath k_SampledVertexAMPath = k_SampledVertexGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName);
const DataPath k_ExemplarySampledVertexGeomPath = DataPath({"Exemplary Sampled Vertex Geometry"});

template <typename T>
DataArray<T>& createFixtureArray(DataStructure& dataStructure, const DataPath& path, const ShapeType& tupleShape, const ShapeType& componentShape, DataObject::IdType parentId)
{
  auto store = DataStoreUtilities::CreateDataStore<T>(dataStructure, path, tupleShape, componentShape);
  REQUIRE((store != nullptr));
  auto* arrayPtr = DataArray<T>::Create(dataStructure, path.getTargetName(), store, parentId);
  REQUIRE((arrayPtr != nullptr));
  return *arrayPtr;
}

void requireArrayStorage(const IDataArray& array, bool outOfCore)
{
  const auto expectedStore = outOfCore ? IDataStore::StoreType::OutOfCore : IDataStore::StoreType::InMemory;
  REQUIRE((array.getIDataStoreRef().getStoreType() == expectedStore));
  if(outOfCore)
  {
    REQUIRE((array.getIDataStoreRef().getDataFormat() == "HDF5-OOC"));
  }
}

void requireEndpointDistances(const EdgeGeom& inputGeom, const VertexGeom& outputGeom, const UInt64Array& edgeIds, const Float32Array& distances)
{
  const auto& inputVertices = inputGeom.getVerticesRef();
  const auto& edges = inputGeom.getEdgesRef();
  const auto& outputVertices = outputGeom.getVerticesRef();
  REQUIRE((distances.getNumberOfTuples() == outputGeom.getNumberOfVertices()));
  REQUIRE((edgeIds.getNumberOfTuples() == distances.getNumberOfTuples()));
  for(usize sample = 0; sample < distances.getNumberOfTuples(); ++sample)
  {
    const uint64 edge = edgeIds[sample];
    REQUIRE((edge < inputGeom.getNumberOfEdges()));
    const uint64 startVertex = edges[2 * edge];
    REQUIRE((startVertex < inputGeom.getNumberOfVertices()));
    std::array<float64, 3> displacement{};
    float64 coordinateScale = 1.0;
    for(usize component = 0; component < 3; ++component)
    {
      const float64 start = inputVertices[(3 * startVertex) + component];
      const float64 point = outputVertices[(3 * sample) + component];
      displacement.at(component) = point - start;
      coordinateScale = std::max({coordinateScale, std::abs(start), std::abs(point)});
    }
    const float64 expectedDistance = std::hypot(displacement[0], displacement[1], displacement[2]);
    // Coordinate rounding grows with the magnitude of the stored float32 coordinates.
    const float64 tolerance = 8.0 * std::numeric_limits<float32>::epsilon() * coordinateScale;
    CAPTURE(sample, edge, expectedDistance, distances[sample]);
    REQUIRE((distances[sample] == Approx(expectedDistance).margin(tolerance)));
  }
}

void requireEndpointDistanceFixture(bool outOfCore, bool calculateDistance, const std::array<float64, 3>& direction)
{
  constexpr std::array<float64, 3> lengths{2.5, 0.1, 2.6};
  constexpr std::array<usize, 3> sampleCounts{11, 1, 11};
  constexpr std::array<float64, 3> start{1.0, -2.0, 3.0};
  const DataPath inputPath{{"Input Edges"}};
  const DataPath attributePath = inputPath.createChildPath(EdgeGeom::k_EdgeAttributeMatrixName).createChildPath("Attributes");
  DataStructure dataStructure;
  auto* geomPtr = EdgeGeom::Create(dataStructure, inputPath.getTargetName());
  REQUIRE((geomPtr != nullptr));
  geomPtr->setUnits(IGeometry::LengthUnit::Millimeter);
  auto* edgeAmPtr = AttributeMatrix::Create(dataStructure, EdgeGeom::k_EdgeAttributeMatrixName, {3}, geomPtr->getId());
  REQUIRE((edgeAmPtr != nullptr));
  geomPtr->setEdgeAttributeMatrix(*edgeAmPtr);
  auto& inputVertices = createFixtureArray<float32>(dataStructure, inputPath.createChildPath(EdgeGeom::k_SharedVertexListName), {6}, {3}, geomPtr->getId());
  auto& inputEdges = createFixtureArray<IGeometry::MeshIndexType>(dataStructure, inputPath.createChildPath(EdgeGeom::k_SharedEdgeListName), {3}, {2}, geomPtr->getId());
  auto& inputAttributes = createFixtureArray<int32>(dataStructure, attributePath, {3}, {2}, edgeAmPtr->getId());
  geomPtr->setVertices(inputVertices);
  geomPtr->setEdgeList(inputEdges);
  for(usize edge = 0; edge < 3; ++edge)
  {
    for(usize endpoint = 0; endpoint < 2; ++endpoint)
    {
      inputEdges[(2 * edge) + endpoint] = (2 * edge) + endpoint;
      inputAttributes[(2 * edge) + endpoint] = 100 + static_cast<int32>((10 * edge) + endpoint);
      for(usize component = 0; component < 3; ++component)
      {
        inputVertices[(6 * edge) + (3 * endpoint) + component] = static_cast<float32>(start.at(component) + (static_cast<float64>(endpoint) * lengths.at(edge) * direction.at(component)));
      }
    }
  }
  requireArrayStorage(inputVertices, outOfCore);
  requireArrayStorage(inputEdges, outOfCore);
  requireArrayStorage(inputAttributes, outOfCore);

  const PointSampleEdgeGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorSamplingRes_Key, std::make_any<float32>(0.25F));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorGeometryPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CalculateCumulativeSampleDistance_Key, std::make_any<bool>(calculateDistance));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SampledVertexGeometryPath_Key, std::make_any<DataPath>(k_SampledVertexGeomPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({attributePath}));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<VertexGeom>(k_SampledVertexGeomPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64Array>(k_SampledVertexAMPath.createChildPath("Edge Ids")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_SampledVertexAMPath.createChildPath("Attributes")));
  const auto& outputGeom = dataStructure.getDataRefAs<VertexGeom>(k_SampledVertexGeomPath);
  const auto& vertices = outputGeom.getVerticesRef();
  const auto& edgeIds = dataStructure.getDataRefAs<UInt64Array>(k_SampledVertexAMPath.createChildPath("Edge Ids"));
  const auto& attributes = dataStructure.getDataRefAs<Int32Array>(k_SampledVertexAMPath.createChildPath("Attributes"));
  requireArrayStorage(vertices, outOfCore);
  requireArrayStorage(edgeIds, outOfCore);
  requireArrayStorage(attributes, outOfCore);
  REQUIRE((outputGeom.getNumberOfVertices() == 23));
  REQUIRE((edgeIds.getNumberOfTuples() == 23));
  REQUIRE((attributes.getNumberOfTuples() == 23));
  REQUIRE((attributes.getNumberOfComponents() == 2));
  usize sample = 0;
  for(usize edge = 0; edge < 3; ++edge)
  {
    for(usize localSample = 0; localSample < sampleCounts.at(edge); ++localSample, ++sample)
    {
      CAPTURE(edge, localSample);
      REQUIRE((edgeIds[sample] == edge));
      const float64 fraction = (static_cast<float64>(localSample) + 0.5) / static_cast<float64>(sampleCounts.at(edge));
      for(usize component = 0; component < 3; ++component)
      {
        const float64 expectedCoordinate = start.at(component) + (fraction * lengths.at(edge) * direction.at(component));
        REQUIRE((vertices[(3 * sample) + component] == Approx(expectedCoordinate).margin(2.0e-6)));
      }
      for(usize component = 0; component < 2; ++component)
      {
        REQUIRE((attributes[(2 * sample) + component] == 100 + static_cast<int32>((10 * edge) + component)));
      }
    }
    for(usize endpoint = 0; endpoint < 2; ++endpoint)
    {
      REQUIRE((inputEdges[(2 * edge) + endpoint] == (2 * edge) + endpoint));
      REQUIRE((inputAttributes[(2 * edge) + endpoint] == 100 + static_cast<int32>((10 * edge) + endpoint)));
      for(usize component = 0; component < 3; ++component)
      {
        const float32 originalCoordinate = static_cast<float32>(start.at(component) + (static_cast<float64>(endpoint) * lengths.at(edge) * direction.at(component)));
        REQUIRE((inputVertices[(6 * edge) + (3 * endpoint) + component] == originalCoordinate));
      }
    }
  }
  const DataPath distancePath = k_SampledVertexAMPath.createChildPath("Cumulative Sample Distance");
  if(calculateDistance)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(distancePath));
    const auto& distances = dataStructure.getDataRefAs<Float32Array>(distancePath);
    requireArrayStorage(distances, outOfCore);
    requireEndpointDistances(*geomPtr, outputGeom, edgeIds, distances);
  }
  else
  {
    REQUIRE((dataStructure.getDataAs<IDataArray>(distancePath) == nullptr));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

void requireEndpointDistanceVariants(bool outOfCore)
{
  constexpr std::array<std::array<float64, 3>, 5> directions{{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {-1.0, 0.0, 0.0}, {3.0 / 13.0, 4.0 / 13.0, 12.0 / 13.0}}};
  for(const auto& direction : directions)
  {
    DYNAMIC_SECTION("Direction " << direction[0] << ", " << direction[1] << ", " << direction[2])
    {
      requireEndpointDistanceFixture(outOfCore, false, direction);
      requireEndpointDistanceFixture(outOfCore, true, direction);
    }
  }
}
} // namespace

TEST_CASE("SimplnxCore::PointSampleEdgeGeometryFilter: Distance from endpoint", "[SimplnxCore][PointSampleEdgeGeometryFilter]")
{
  REQUIRE((Application::GetOrCreateInstance() != nullptr));
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  requireEndpointDistanceVariants(false);
}

TEST_CASE("SimplnxCore::PointSampleEdgeGeometryFilter: real-store distance from endpoint", "[SimplnxCore][PointSampleEdgeGeometryFilter][.OocStoreContract]")
{
  REQUIRE((Application::GetOrCreateInstance()->getIOManager("HDF5-OOC") != nullptr));
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  requireEndpointDistanceVariants(true);
}

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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(k_ScanVectorGeometryPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<VertexGeom>(k_SampledVertexGeomPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64Array>(k_SampledVertexAMPath.createChildPath("Edge Ids")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_SampledVertexAMPath.createChildPath("Cumulative Sample Distance")));
  requireEndpointDistances(dataStructure.getDataRefAs<EdgeGeom>(k_ScanVectorGeometryPath), dataStructure.getDataRefAs<VertexGeom>(k_SampledVertexGeomPath),
                           dataStructure.getDataRefAs<UInt64Array>(k_SampledVertexAMPath.createChildPath("Edge Ids")),
                           dataStructure.getDataRefAs<Float32Array>(k_SampledVertexAMPath.createChildPath("Cumulative Sample Distance")));
  // The archive's distance array uses a different origin. Compare all other arrays with the unchanged archive.
  REQUIRE((exemplarDataStructure.removeData(k_ExemplarySampledVertexGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath("Cumulative Sample Distance"))));
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

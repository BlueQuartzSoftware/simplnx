#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/CreateAMScanPathsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const nx::core::DataPath k_ExemplarEdgeGeometryPath = DataPath({"Exemplar Slice Geometry"});
const nx::core::DataPath k_ExemplarScanVectorsPath = DataPath({"Exemplar Scan Paths Geometry"});
const nx::core::DataPath k_RegionIdsPath = DataPath({"Exemplar Slice Geometry", "Edge Data", "Part Number"});
const nx::core::DataPath k_SliceIdsPath = DataPath({"Exemplar Slice Geometry", "Edge Data", "Slice Ids"});

const nx::core::DataPath k_ComputedScanVectorsPath = DataPath({"Output Scan Vectors"});

const DataObjectNameParameter::ValueType k_EdgeData("Edge Data");
// const DataObjectNameParameter::ValueType k_VertexData("VertexData");
const DataObjectNameParameter::ValueType k_RegionIdsName("RegionIds");
} // namespace

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Valid Filter Execution", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  UnitTest::LoadPlugins();

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_0_SurfaceMesh_Test_Files_v2.tar.gz", "7_0_SurfaceMesh_Test_Files");
  auto baseDataFilePath = fs::path(fmt::format("{}/7_0_SurfaceMesh_Test_Files/7_0_SurfaceMesh_Test_Files.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateAMScanPathsFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, std::make_any<float32>(0.14f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_StripeWidth_Key, std::make_any<float32>(7.0f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, std::make_any<float32>(67.0f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key, std::make_any<DataPath>(k_ExemplarEdgeGeometryPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key, std::make_any<DataPath>(k_SliceIdsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key, std::make_any<DataPath>(k_RegionIdsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key, std::make_any<DataPath>(k_ComputedScanVectorsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_VertexAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_VertexData));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_EdgeData));
  args.insertOrAssign(CreateAMScanPathsFilter::k_RegionIdsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_RegionIdsName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  std::cout << "Writing File: " << fmt::format("{}/create_am_scan_paths_test.dream3d", unit_test::k_BinaryTestOutputDir) << "\n";
  WriteTestDataStructure(dataStructure, fmt::format("{}/create_am_scan_paths_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare the exemplar and the computed outputs
  {
    auto exemplarGeom = dataStructure.getDataAs<IGeometry>(k_ExemplarScanVectorsPath);
    auto computedGeom = dataStructure.getDataAs<IGeometry>(k_ComputedScanVectorsPath);
    REQUIRE(UnitTest::CompareIGeometry(exemplarGeom, computedGeom));
  }

  {
    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath("EdgeData").createChildPath(k_SliceIdsPath.getTargetName());
    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_SliceIdsPath.getTargetName());
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  {
    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath("EdgeData").createChildPath("RegionIds");
    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_RegionIdsName);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CreateAMScanPathsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateAMScanPathsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CreateAMScanPathsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<float32>(CreateAMScanPathsFilter::k_StripeWidth_Key) == Approx(2.5f));
      CHECK(args.value<float32>(CreateAMScanPathsFilter::k_HatchSpacing_Key) == Approx(2.5f));
      CHECK(args.value<DataPath>(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "SliceIds"}));
      CHECK(args.value<DataPath>(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "RegionIds"}));
      CHECK(args.value<DataPath>(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key) == DataPath({"HatchDataContainer"}));
      CHECK(args.value<std::string>(CreateAMScanPathsFilter::k_VertexAttributeMatrixName_Key) == "VertexData");
      CHECK(args.value<std::string>(CreateAMScanPathsFilter::k_HatchAttributeMatrixName_Key) == "HatchData");
      CHECK(args.value<std::string>(CreateAMScanPathsFilter::k_RegionIdsArrayName_Key) == "RegionIds");
    }
  }
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Scan vector geometry below an Attribute Matrix", "[SimplnxCore][CreateAMScanPathsFilter][L02]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* sliceGeom = EdgeGeom::Create(dataStructure, "Slice Geometry");
  REQUIRE(sliceGeom != nullptr);
  auto* edgeData = AttributeMatrix::Create(dataStructure, "Edge Data", {1}, sliceGeom->getId());
  REQUIRE(edgeData != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Slice Ids", {1}, {1}, edgeData->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Part Number", {1}, {1}, edgeData->getId()) != nullptr);

  auto* image = DataGroup::Create(dataStructure, "Image");
  REQUIRE(image != nullptr);
  REQUIRE(AttributeMatrix::Create(dataStructure, "Cell Data", {1}, image->getId()) != nullptr);

  // The parent path 'Image/Cell Data/Sub' of the scan vector geometry needs a DataGroup inside an Attribute Matrix
  const DataPath hatchPath({"Image", "Cell Data", "Sub", "X"});
  Arguments args;
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key, std::make_any<DataPath>(DataPath({"Slice Geometry"})));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Slice Geometry", "Edge Data", "Slice Ids"})));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Slice Geometry", "Edge Data", "Part Number"})));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key, std::make_any<DataPath>(hatchPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_EdgeData));
  args.insertOrAssign(CreateAMScanPathsFilter::k_RegionIdsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_RegionIdsName));

  CreateAMScanPathsFilter filter;
  Pipeline pipeline;
  auto pipelineFilter = std::make_shared<PipelineFilter>(filter.clone(), args);
  REQUIRE(pipeline.push_back(pipelineFilter));
  REQUIRE_FALSE(pipeline.preflight(dataStructure, std::atomic_bool{false}));
  REQUIRE_FALSE(pipelineFilter->getErrors().empty());
  REQUIRE(pipelineFilter->getErrors().front().code == -5403);
  REQUIRE_FALSE(dataStructure.containsData(hatchPath));
  REQUIRE_FALSE(dataStructure.containsData(hatchPath.getParent()));
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Hatch crossings at vertices", "[SimplnxCore][CreateAMScanPathsFilter][L13]")
{
  UnitTest::LoadPlugins();
  const bool reverseEdges = GENERATE(false, true);
  CAPTURE(reverseEdges);
  std::vector<float32> inputVertices;
  std::vector<float32> expectedVertices;
  SECTION("Diamond through vertices and tangent extrema")
  {
    // At y=0 and y=2 the hatch only touches a tip. At y=1 it crosses
    // (-1,1) and (1,1), so exactly one segment must survive.
    inputVertices = {0, 0, 0, 1, 1, 0, 0, 2, 0, -1, 1, 0};
    expectedVertices = {-1, 1, 0, 1, 1, 0};
  }
  SECTION("Square with hatch-parallel boundary edges")
  {
    // Half-open [minY,maxY) includes y=0 and y=1, excludes y=2.
    // Horizontal edges do not contribute crossings of their own.
    inputVertices = {0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 2, 0};
    expectedVertices = {0, 0, 0, 2, 0, 0, 0, 1, 0, 2, 1, 0};
  }

  DataStructure dataStructure;
  auto* geom = EdgeGeom::Create(dataStructure, "Polygon");
  REQUIRE(geom != nullptr);
  auto* vertices = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {4}, {3}, geom->getId());
  REQUIRE(vertices != nullptr);
  geom->setVertices(*vertices);
  for(usize i = 0; i < inputVertices.size(); i++)
  {
    (*vertices)[i] = inputVertices[i];
  }
  auto* edges = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "Edges", {4}, {2}, geom->getId());
  REQUIRE(edges != nullptr);
  geom->setEdgeList(*edges);
  for(usize i = 0; i < 4; i++)
  {
    (*edges)[2 * i] = reverseEdges ? (i + 1) % 4 : i;
    (*edges)[2 * i + 1] = reverseEdges ? i : (i + 1) % 4;
  }
  auto* edgeData = AttributeMatrix::Create(dataStructure, "Edge Data", {4}, geom->getId());
  REQUIRE(edgeData != nullptr);
  geom->setEdgeAttributeMatrix(*edgeData);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Slice Ids", {4}, {1}, edgeData->getId()) != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<int32>(dataStructure, "Part Number", {4}, {1}, edgeData->getId()) != nullptr);

  Arguments args;
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, std::make_any<float32>(0.0f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key, std::make_any<DataPath>(DataPath({"Polygon"})));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Polygon", "Edge Data", "Slice Ids"})));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Polygon", "Edge Data", "Part Number"})));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key, std::make_any<DataPath>(k_ComputedScanVectorsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchAttributeMatrixName_Key, std::make_any<std::string>(k_EdgeData));
  args.insertOrAssign(CreateAMScanPathsFilter::k_RegionIdsArrayName_Key, std::make_any<std::string>(k_RegionIdsName));

  CreateAMScanPathsFilter filter;
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* output = dataStructure.getDataAs<EdgeGeom>(k_ComputedScanVectorsPath);
  REQUIRE(output != nullptr);
  REQUIRE(output->getNumberOfEdges() == expectedVertices.size() / 6);
  REQUIRE(output->getNumberOfVertices() == expectedVertices.size() / 3);
  const auto& outputVertices = output->getVerticesRef();
  const auto& outputEdges = output->getEdgesRef();
  for(usize i = 0; i < expectedVertices.size(); i++)
  {
    CAPTURE(i);
    REQUIRE(outputVertices[i] == expectedVertices[i]);
  }
  for(usize i = 0; i < outputEdges.getSize(); i++)
  {
    REQUIRE(outputEdges[i] == i);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

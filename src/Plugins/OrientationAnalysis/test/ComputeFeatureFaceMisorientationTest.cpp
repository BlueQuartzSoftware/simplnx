#include "OrientationAnalysis/Filters/ComputeFeatureFaceMisorientationFilter.hpp"
#include "OrientationAnalysis/Filters/ConvertOrientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
constexpr StringLiteral k_NXFaceMisorientationColors("NXFaceMisorientationColors");
constexpr StringLiteral k_AvgQuats("AvgQuats");

bool CompareFloats(const float32 generated, const float32 expected)
{
  return std::abs(generated - expected) < 0.000012f;
}
} // namespace

namespace curated
{
constexpr StringLiteral k_TriGeomName = "triangle_geom";
const DataPath k_TriGeomPath({k_TriGeomName});

const DataPath k_FaceDataPath = k_TriGeomPath.createChildPath(Constants::k_FaceData);
const DataPath k_FaceLabelsPath = k_FaceDataPath.createChildPath(Constants::k_FaceLabels);

const DataPath k_FeatureDataPath = k_TriGeomPath.createChildPath(Constants::k_Grain_Data);
const DataPath k_AvgEulerAnglesPath = k_FeatureDataPath.createChildPath(Constants::k_AvgEulerAngles);
const DataPath k_FeaturePhasesPath = k_FeatureDataPath.createChildPath(Constants::k_Phases);
const DataPath k_AvgQuatsPath = k_FeatureDataPath.createChildPath(k_AvgQuats);

const DataPath k_PhaseDataPath = k_TriGeomPath.createChildPath(Constants::k_Phase_Data);
const DataPath k_CrystalStructurePath = k_PhaseDataPath.createChildPath(Constants::k_CrystalStructures);

/**
 * The data for this test structure was hand-rolled and provided by Mike Jackson.
 */
DataStructure CreateTestDataStructure()
{
  DataStructure dataStructure = {};

  TriangleGeom* geom = TriangleGeom::Create(dataStructure, k_TriGeomName);

  // Make Shared Vertex List
  {
    // clang-format off
    std::unique_ptr<TriangleGeom::SharedVertexList::value_type[]> sharedVertexListBuffer(new TriangleGeom::SharedVertexList::value_type[] {
      0.0f,0.0f,0.0f,
      1.0f,0.0f,0.0f,
      0.0f,1.0f,0.0f,
      2.0f,0.0f,0.0f,
      3.0f,0.0f,0.0f,
      2.0f,1.0f,0.0f,
      4.0f,0.0f,0.0f,
      5.0f,0.0f,0.0f,
      4.0f,1.0f,0.0f,
      0.0f,2.0f,0.0f,
      1.0f,2.0f,0.0f,
      0.0f,3.0f,0.0f,
      2.0f,2.0f,0.0f,
      3.0f,2.0f,0.0f,
      2.0f,3.0f,0.0f,
      4.0f,2.0f,0.0f,
      5.0f,2.0f,0.0f,
      4.0f,3.0f,0.0f,
      0.0f,4.0f,0.0f,
      1.0f,4.0f,0.0f,
      0.0f,5.0f,0.0f,
      2.0f,4.0f,0.0f,
      3.0f,4.0f,0.0f,
      2.0f,5.0f,0.0f,
      4.0f,4.0f,0.0f,
      5.0f,4.0f,0.0f,
      4.0f,5.0f,0.0f,
      0.0f,6.0f,0.0f,
      1.0f,6.0f,0.0f,
      0.0f,7.0f,0.0f,
      2.0f,6.0f,0.0f,
      3.0f,6.0f,0.0f,
      2.0f,7.0f,0.0f,
      4.0f,6.0f,0.0f,
      5.0f,6.0f,0.0f,
      4.0f,7.0f,0.0f,
      0.0f,8.0f,0.0f,
      1.0f,8.0f,0.0f,
      0.0f,9.0f,0.0f,
      2.0f,8.0f,0.0f,
      3.0f,8.0f,0.0f,
      2.0f,9.0f,0.0f,
      4.0f,8.0f,0.0f,
      5.0f,8.0f,0.0f,
      4.0f,9.0f,0.0f,
      0.0f,10.0f,0.0f,
      1.0f,10.0f,0.0f,
      0.0f,11.0f,0.0f,
      2.0f,10.0f,0.0f,
      3.0f,10.0f,0.0f,
      2.0f,11.0f,0.0f,
      4.0f,10.0f,0.0f,
      5.0f,10.0f,0.0f,
      4.0f,11.0f,0.0f,
      0.0f,12.0f,0.0f,
      1.0f,12.0f,0.0f,
      0.0f,13.0f,0.0f,
      2.0f,12.0f,0.0f,
      3.0f,12.0f,0.0f,
      2.0f,13.0f,0.0f,
      4.0f,12.0f,0.0f,
      5.0f,12.0f,0.0f,
      4.0f,13.0f,0.0f,
      0.0f,14.0f,0.0f,
      1.0f,14.0f,0.0f,
      0.0f,15.0f,0.0f,
      2.0f,14.0f,0.0f,
      3.0f,14.0f,0.0f,
      2.0f,15.0f,0.0f,
      4.0f,14.0f,0.0f,
      5.0f,14.0f,0.0f,
      4.0f,15.0f,0.0f,
      0.0f,16.0f,0.0f,
      1.0f,16.0f,0.0f,
      0.0f,17.0f,0.0f,
      2.0f,16.0f,0.0f,
      3.0f,16.0f,0.0f,
      2.0f,17.0f,0.0f,
      4.0f,16.0f,0.0f,
      5.0f,16.0f,0.0f,
      4.0f,17.0f,0.0f,
      0.0f,18.0f,0.0f,
      1.0f,18.0f,0.0f,
      0.0f,19.0f,0.0f,
      2.0f,18.0f,0.0f,
      3.0f,18.0f,0.0f,
      2.0f,19.0f,0.0f,
      4.0f,18.0f,0.0f,
      5.0f,18.0f,0.0f,
      4.0f,19.0f,0.0f,
      0.0f,20.0f,0.0f,
      1.0f,20.0f,0.0f,
      0.0f,21.0f,0.0f,
      2.0f,20.0f,0.0f,
      3.0f,20.0f,0.0f,
      2.0f,21.0f,0.0f,
      4.0f,20.0f,0.0f,
      5.0f,20.0f,0.0f,
      4.0f,21.0f,0.0f,
      0.0f,22.0f,0.0f,
      1.0f,22.0f,0.0f,
      0.0f,23.0f,0.0f
    });
    // clang-format on

    const TriangleGeom::SharedVertexList* sharedVerts =
        TriangleGeom::SharedVertexList::Create(dataStructure, TriangleGeom::k_SharedVertexListName,
                                               std::make_shared<DataStore<TriangleGeom::SharedVertexList::value_type>>(std::move(sharedVertexListBuffer), ShapeType{102}, ShapeType{3}), geom->getId());

    geom->setVertexListId(sharedVerts->getId());
  }

  // Create Shared Triangles List
  {
    // clang-format off
    std::unique_ptr<TriangleGeom::SharedFaceList::value_type[]> sharedFaceListBuffer(new TriangleGeom::SharedFaceList::value_type[] {
      0,1,2,
      3,4,5,
      6,7,8,
      9,10,11,
      12,13,14,
      15,16,17,
      18,19,20,
      21,22,23,
      24,25,26,
      27,28,29,
      30,31,32,
      33,34,35,
      36,37,38,
      39,40,41,
      42,43,44,
      45,46,47,
      48,49,50,
      51,52,53,
      54,55,56,
      57,58,59,
      60,61,62,
      63,64,65,
      66,67,68,
      69,70,71,
      72,73,74,
      75,76,77,
      78,79,80,
      81,82,83,
      84,85,86,
      87,88,89,
      90,91,92,
      93,94,95,
      96,97,98,
      99,100,101
    });
    // clang-format on

    const TriangleGeom::SharedFaceList* sharedFaces =
        TriangleGeom::SharedFaceList::Create(dataStructure, TriangleGeom::k_SharedFacesListName,
                                             std::make_shared<DataStore<TriangleGeom::SharedFaceList::value_type>>(std::move(sharedFaceListBuffer), ShapeType{34}, ShapeType{3}), geom->getId());

    geom->setFaceListId(sharedFaces->getId());
  }

  AttributeMatrix* faceDataAM = AttributeMatrix::Create(dataStructure, k_FaceDataPath.getTargetName(), ShapeType{34}, geom->getId());

  // Make Face Labels
  {
    // clang-format off
    std::unique_ptr<int32[]> faceLabelsBuffer(new int32[]{
      1,2,
      1,3,
      1,4,
      5,6,
      5,7,
      5,8,
      9,10,
      9,11,
      9,12,
      13,14,
      13,15,
      13,16,
      17,18,
      17,19,
      17,20,
      21,22,
      21,23,
      21,24,
      25,26,
      25,27,
      25,28,
      29,30,
      29,31,
      29,32,
      33,34,
      33,35,
      33,36,
      37,38,
      37,39,
      37,40,
      0,1,
      1,0,
      1,5,
      5,1
    });
    // clang-format on

    DataArray<int32>::Create(dataStructure, k_FaceLabelsPath.getTargetName(), std::make_shared<Int32DataStore>(std::move(faceLabelsBuffer), faceDataAM->getShape(), ShapeType{2}), faceDataAM->getId());
  }

  AttributeMatrix* featureDataAM = AttributeMatrix::Create(dataStructure, k_FeatureDataPath.getTargetName(), ShapeType{41}, geom->getId());

  // Create AvgEulers
  {
    // clang-format off
    std::unique_ptr<float32[]> avgEulerAnglesBuffer(new float32[] {
      0.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f,
      0.00f,0.00f,0.00f,
      45.00f,0.00f,0.00f,
      90.00f,0.00f,0.00f,
      180.00f,0.00f,0.00f
    });
    // clang-format on

    DataArray<float32>::Create(dataStructure, k_AvgEulerAnglesPath.getTargetName(), std::make_shared<Float32DataStore>(std::move(avgEulerAnglesBuffer), featureDataAM->getShape(), ShapeType{3}),
                               featureDataAM->getId());
  }

  // Create Phases
  {
    // clang-format off
    std::unique_ptr<int32[]> phasesBuffer(new int32[] {
      999,
      1,
      1,
      1,
      1,
      2,
      2,
      2,
      2,
      3,
      3,
      3,
      3,
      4,
      4,
      4,
      4,
      5,
      5,
      5,
      5,
      6,
      6,
      6,
      6,
      7,
      7,
      7,
      7,
      8,
      8,
      8,
      8,
      9,
      9,
      9,
      9,
      10,
      10,
      10,
      10
    });
    // clang-format on

    DataArray<int32>::Create(dataStructure, k_FeaturePhasesPath.getTargetName(), std::make_shared<Int32DataStore>(std::move(phasesBuffer), featureDataAM->getShape(), ShapeType{1}),
                             featureDataAM->getId());
  }

  AttributeMatrix* phaseDataAM = AttributeMatrix::Create(dataStructure, k_PhaseDataPath.getTargetName(), ShapeType{12}, geom->getId());

  // Create CrystalStructures
  {
    // clang-format off
    std::unique_ptr<uint32[]> crystalStructBuffer(new uint32[] {
      999,
      0,
      1,
      2,
      3,
      4,
      5,
      6,
      7,
      8,
      9,
      10
    });
    // clang-format on

    DataArray<uint32>::Create(dataStructure, k_CrystalStructurePath.getTargetName(), std::make_shared<UInt32DataStore>(std::move(crystalStructBuffer), phaseDataAM->getShape(), ShapeType{1}),
                              phaseDataAM->getId());
  }

  return dataStructure;
}
} // namespace curated

TEST_CASE("OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Curated Data", "[OrientationAnalysis][ComputeFeatureFaceMisorientationFilter]")
{
  DataStructure dataStructure = curated::CreateTestDataStructure();

  // Convert the AvgEulerAngles array to AvgQuats for use in ComputeFeatureFaceMisorientationFilter input
  {
    // Instantiate the filter, and an Arguments Object
    ConvertOrientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ConvertOrientationsFilter::k_InputType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ConvertOrientationsFilter::k_OutputType_Key, std::make_any<uint64>(2));
    args.insertOrAssign(ConvertOrientationsFilter::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(curated::k_AvgEulerAnglesPath));
    args.insertOrAssign(ConvertOrientationsFilter::k_OutputOrientationArrayName_Key, std::make_any<std::string>(k_AvgQuats));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // ComputeFeatureFaceMisorientationFilter
  {
    // Instantiate the filter, and an Arguments Object
    ComputeFeatureFaceMisorientationFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(curated::k_FaceLabelsPath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(curated::k_AvgQuatsPath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(curated::k_FeaturePhasesPath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(curated::k_CrystalStructurePath));
    args.insertOrAssign(ComputeFeatureFaceMisorientationFilter::k_MisorientationArrayName_Key, std::make_any<std::string>(::k_NXFaceMisorientationColors));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Validate Computed Misorientations
  const auto& faceMisorientations = dataStructure.getDataRefAs<Float32Array>(curated::k_FaceDataPath.createChildPath(::k_NXFaceMisorientationColors));

  // Outputs were validated by Mike Jackson
  REQUIRE(::CompareFloats(faceMisorientations[0], 15.0f));
  REQUIRE(::CompareFloats(faceMisorientations[1], 30.0f));
  REQUIRE(::CompareFloats(faceMisorientations[2], 0.0f));
  REQUIRE(::CompareFloats(faceMisorientations[3], 45.0f));

  // Special case (expected 0 but value was validated via MTEX, not for sure correct, but not incorrect)
  REQUIRE(::CompareFloats(faceMisorientations[4], 0.021200536f));

  REQUIRE(::CompareFloats(faceMisorientations[5], 0.0f));
  REQUIRE(::CompareFloats(faceMisorientations[6], 15.0f));
  REQUIRE(::CompareFloats(faceMisorientations[7], 30.0f));
  REQUIRE(::CompareFloats(faceMisorientations[8], 0.0f));
  REQUIRE(::CompareFloats(faceMisorientations[9], 45.0f));
  REQUIRE(::CompareFloats(faceMisorientations[10], 90.0f));
  REQUIRE(::CompareFloats(faceMisorientations[11], 0.0f));
  REQUIRE(::CompareFloats(faceMisorientations[12], 45.0f));
  REQUIRE(::CompareFloats(faceMisorientations[13], 90.0f));
  REQUIRE(::CompareFloats(faceMisorientations[14], 180.0f));
  REQUIRE(::CompareFloats(faceMisorientations[15], 45.0f));
  REQUIRE(::CompareFloats(faceMisorientations[16], 90.0f));
  REQUIRE(::CompareFloats(faceMisorientations[17], 180.0f));
  REQUIRE(::CompareFloats(faceMisorientations[18], 45.0f));
  REQUIRE(::CompareFloats(faceMisorientations[19], 90.0f));
  REQUIRE(::CompareFloats(faceMisorientations[20], 0.0f));
  REQUIRE(::CompareFloats(faceMisorientations[21], 45.0f));
  REQUIRE(::CompareFloats(faceMisorientations[22], 0.0f));
  REQUIRE(::CompareFloats(faceMisorientations[23], 0.0f));
  REQUIRE(::CompareFloats(faceMisorientations[24], 45.0f));
  REQUIRE(::CompareFloats(faceMisorientations[25], 0.0f));
  REQUIRE(::CompareFloats(faceMisorientations[26], 0.0f));
  REQUIRE(::CompareFloats(faceMisorientations[27], 45.0f));
  REQUIRE(::CompareFloats(faceMisorientations[28], 30.0f));
  REQUIRE(::CompareFloats(faceMisorientations[29], 60.0f));

  // Special Cases
  REQUIRE(std::isnan(faceMisorientations[30]));
  REQUIRE(std::isnan(faceMisorientations[31]));
  REQUIRE(std::isnan(faceMisorientations[32]));
  REQUIRE(std::isnan(faceMisorientations[33]));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeFeatureFaceMisorientationFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureFaceMisorientationFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureFaceMisorientationFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureFaceMisorientationFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeatureFaceMisorientationFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureFaceMisorientationFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureFaceMisorientationFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureFaceMisorientationFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFeatureFaceMisorientationFilter::k_MisorientationArrayName_Key) == "TestName");
    }
  }
}

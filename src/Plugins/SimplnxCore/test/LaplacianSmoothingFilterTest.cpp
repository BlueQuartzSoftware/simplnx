#include "SimplnxCore/Filters/LaplacianSmoothingFilter.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::LaplacianSmoothingFilter", "[SurfaceMeshing][LaplacianSmoothingFilter]")
{
  UnitTest::LoadPlugins();

  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = INodeGeometry2D::k_FaceAttributeMatrixName;
  std::string normalsDataArrayName = "FaceNormals";
  std::string triangleVertexDataGroupName = INodeGeometry0D::k_VertexAttributeMatrixName;
  std::string nodeTypeArrayName = "Node Type";

  DataStructure dataStructure;

  {
    Arguments args;
    ReadStlFileFilter filter;

    DataPath triangleGeomDataPath({triangleGeometryName});
    DataPath triangleFaceDataGroupDataPath({triangleGeometryName, triangleFaceDataGroupName});
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_SimplnxTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }

  {
    DataPath triangleGeometryPath({triangleGeometryName});

    DataObject::IdType triangleGeometryId = dataStructure.getId(triangleGeometryPath).value();
    DataPath vertexDataGroupPath = triangleGeometryPath.createChildPath(triangleVertexDataGroupName);
    DataObject::IdType vertexDataGroupId = dataStructure.getId(vertexDataGroupPath).value();

    // Instantiate the filter, a DataStructure object and an Arguments Object
    LaplacianSmoothingFilter filter;
    Arguments args;

    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeometryPath);
    std::vector<size_t> tupleShape = {triangleGeom.getNumberOfVertices()};
    std::vector<size_t> compShape = {1};
    // Insert a Node Type array into the DataStructure so the filter works.
    Int8Array* nodeType = nx::core::UnitTest::CreateTestDataArray<int8_t>(dataStructure, nodeTypeArrayName, tupleShape, compShape, vertexDataGroupId);

    // Assign the `Default Node Type` to all the values
    for(size_t i = 0; i < triangleGeom.getNumberOfVertices(); i++)
    {
      (*nodeType)[i] = nx::core::NodeType::Default;
    }

    DataPath nodeTypeArrayPath = vertexDataGroupPath.createChildPath(nodeTypeArrayName);

    // Create default Parameters for the filter.
    args.insertOrAssign(LaplacianSmoothingFilter::k_IterationSteps_Key, std::make_any<int32>(5));
    args.insertOrAssign(LaplacianSmoothingFilter::k_Lambda_Key, std::make_any<float32>(0.15F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_UseTaubinSmoothing_Key, std::make_any<bool>(true));
    args.insertOrAssign(LaplacianSmoothingFilter::k_MuFactor_Key, std::make_any<float32>(.1F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_TripleLineLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_QuadPointLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_SurfacePointLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_SurfaceTripleLineLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_SurfaceQuadPointLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_SurfaceMeshNodeTypeArrayPath_Key, std::make_any<DataPath>(nodeTypeArrayPath));
    args.insertOrAssign(LaplacianSmoothingFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(triangleGeometryPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    if(preflightResult.outputActions.invalid())
    {
      for(const auto& error : preflightResult.outputActions.errors())
      {
        std::cout << error.code << ": " << error.message << std::endl;
      }
    }
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/laplacian_smoothing_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::LaplacianSmoothingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][LaplacianSmoothingFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "LaplacianSmoothingFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "LaplacianSmoothingFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<LaplacianSmoothingFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int32>(LaplacianSmoothingFilter::k_IterationSteps_Key) == 5);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_Lambda_Key) == 2.5f);
      CHECK(args.value<bool>(LaplacianSmoothingFilter::k_UseTaubinSmoothing_Key) == true);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_MuFactor_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_TripleLineLambda_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_QuadPointLambda_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_SurfacePointLambda_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_SurfaceTripleLineLambda_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_SurfaceQuadPointLambda_Key) == 2.5f);
      CHECK(args.value<DataPath>(LaplacianSmoothingFilter::k_TriangleGeometryDataPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(LaplacianSmoothingFilter::k_SurfaceMeshNodeTypeArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(LaplacianSmoothingFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

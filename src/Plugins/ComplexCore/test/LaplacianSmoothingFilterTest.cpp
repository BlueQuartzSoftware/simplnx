#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/LaplacianSmoothingFilter.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::LaplacianSmoothingFilter", "[SurfaceMeshing][LaplacianSmoothingFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string normalsDataArrayName = "Normals";
  std::string triangleVertexDataGroupName = "Vertex Data";
  std::string nodeTypeArrayName = "Node Type";

  DataStructure dataGraph;

  {
    Arguments args;
    StlFileReaderFilter filter;

    DataPath triangleGeomDataPath({triangleGeometryName});
    DataPath triangleFaceDataGroupDataPath({triangleGeometryName, triangleFaceDataGroupName});
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(StlFileReaderFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(StlFileReaderFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(triangleGeomDataPath));
    args.insertOrAssign(StlFileReaderFilter::k_FaceGroupDataPath_Key, std::make_any<DataPath>(triangleFaceDataGroupDataPath));
    args.insertOrAssign(StlFileReaderFilter::k_FaceNormalsDataPath_Key, std::make_any<DataPath>(normalsDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }

  {
    DataPath triangleGeometryPath({triangleGeometryName});

    DataObject::IdType triangleGeometryId = dataGraph.getId(triangleGeometryPath).value();
    DataPath vertexDataGroupPath = triangleGeometryPath.createChildPath(triangleVertexDataGroupName);
    DataGroup::Create(dataGraph, triangleVertexDataGroupName, triangleGeometryId);
    DataObject::IdType vertexDataGroupId = dataGraph.getId(vertexDataGroupPath).value();

    // Instantiate the filter, a DataStructure object and an Arguments Object
    LaplacianSmoothingFilter filter;
    Arguments args;

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(triangleGeometryPath);
    std::vector<size_t> tupleShape = {triangleGeom.getNumberOfVertices()};
    std::vector<size_t> compShape = {1};
    // Insert a Node Type array into the DataStructure so the filter works.
    Int8Array* nodeType = complex::UnitTest::CreateTestDataArray<int8_t>(dataGraph, nodeTypeArrayName, tupleShape, compShape, vertexDataGroupId);

    // Assign the `Default Node Type` to all the values
    for(size_t i = 0; i < triangleGeom.getNumberOfVertices(); i++)
    {
      (*nodeType)[i] = complex::NodeType::Default;
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
    auto preflightResult = filter.preflight(dataGraph, args);
    if(preflightResult.outputActions.invalid())
    {
      for(const auto& error : preflightResult.outputActions.errors())
      {
        std::cout << error.code << ": " << error.message << std::endl;
      }
    }
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());
  }

  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/LaplacianSmoothing.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

// TEST_CASE("SurfaceMeshing::LaplacianSmoothingFilter: Valid filter execution")
//{
//
//}

// TEST_CASE("SurfaceMeshing::LaplacianSmoothingFilter: InValid filter execution")
//{
//
//}

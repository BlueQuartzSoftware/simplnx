#include <catch2/catch.hpp>

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"
#include "complex/Utilities/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CalculateTriangleAreasFilter.hpp"
#include "ComplexCore/Filters/PointSampleTriangleGeometryFilter.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"

#include <filesystem>
#include <limits>

namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

std::array<float, 6> FindMinMaxCoord(AbstractGeometry::SharedVertexList* vertices, usize numVerts)
{
  std::array<float, 6> minMaxVerts = {std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), std::numeric_limits<float>::max(),
                                      std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), std::numeric_limits<float>::min()};

  for(usize v = 0; v < numVerts; v++)
  {
    if((*vertices)[v * 3] < minMaxVerts[0])
    {
      minMaxVerts[0] = (*vertices)[v * 3];
    }
    if((*vertices)[v * 3] > minMaxVerts[1])
    {
      minMaxVerts[1] = (*vertices)[v * 3];
    }

    if((*vertices)[v * 3 + 1] < minMaxVerts[2])
    {
      minMaxVerts[2] = (*vertices)[v * 3 + 1];
    }
    if((*vertices)[v * 3 + 1] > minMaxVerts[3])
    {
      minMaxVerts[3] = (*vertices)[v * 3 + 1];
    }

    if((*vertices)[v * 3 + 2] < minMaxVerts[4])
    {
      minMaxVerts[4] = (*vertices)[v * 3 + 2];
    }
    if((*vertices)[v * 3 + 2] > minMaxVerts[5])
    {
      minMaxVerts[5] = (*vertices)[v * 3 + 2];
    }
  }

  //  for(auto& coord : minMaxVerts)
  //  {
  //    std::cout << coord << ",";
  //  }
  //  std::cout << std::endl;

  return minMaxVerts;
}

TEST_CASE("DREAM3DReview::PointSampleTriangleGeometryFilter", "[DREAM3DReview][PointSampleTriangleGeometryFilter]")
{

  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string normalsDataArrayName = "Normals";
  std::string triangleAreasName = "Triangle Areas";
  std::string vertexGeometryName = "[Vertex Geometry]";
  std::string vertexNodeDataGroup = "Vertex Data";
  DataStructure dataGraph;
  DataPath parentPath = DataPath({k_LevelZero});
  // Read in the STL File to load a Triangle Geometry to sample
  {
    StlFileReaderFilter filter;
    Arguments args;

    DataGroup::Create(dataGraph, k_LevelZero);

    DataPath normalsDataPath = parentPath.createChildPath(triangleGeometryName).createChildPath(triangleFaceDataGroupName).createChildPath(normalsDataArrayName);

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(StlFileReaderFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(StlFileReaderFilter::k_ParentDataGroupPath_Key, std::make_any<DataPath>(parentPath));
    args.insertOrAssign(StlFileReaderFilter::k_GeometryName_Key, std::make_any<std::string>(triangleGeometryName));
    args.insertOrAssign(StlFileReaderFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(triangleFaceDataGroupName));
    args.insertOrAssign(StlFileReaderFilter::k_FaceNormalsArrayName_Key, std::make_any<DataPath>(normalsDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(parentPath.createChildPath(triangleGeometryName));
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }

  // Calculate the Triangle Areas
  {
    CalculateTriangleAreasFilter filter;
    Arguments args;

    DataPath geometryPath = DataPath({k_LevelZero, triangleGeometryName});

    // Create default Parameters for the filter.
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleAreasName);
    args.insertOrAssign(CalculateTriangleAreasFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CalculateTriangleAreasFilter::k_CalculatedAreasDataPath_Key, std::make_any<DataPath>(triangleAreasDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    // Let's sum up all the areas.
    Float64Array& faceAreas = dataGraph.getDataRefAs<Float64Array>(triangleAreasDataPath);
    double sumOfAreas = 0.0;
    for(const auto& area : faceAreas)
    {
      sumOfAreas += area;
    }
    REQUIRE(sumOfAreas > 7098.90);
    REQUIRE(sumOfAreas < 7098.94);
  }

  // Run the PointSampleTriangleGeometryFilter
  {
    const std::string k_OutputFile("PointSampleTriangleGeometryTest.dream3d");

    // Instantiate the filter, a DataStructure object and an Arguments Object
    PointSampleTriangleGeometryFilter filter;
    Arguments args;

    //    const ChoicesParameter::ValueType k_ManualSampling = 0;
    //    const int32_t k_GeometrySampling = 1;

    // Create default Parameters for the filter.
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_NumberOfSamples_Key, std::make_any<int32>(200));
    // args.insertOrAssign(PointSampleTriangleGeometryFilter::k_SamplesNumberType_Key, std::make_any<ChoicesParameter::ValueType>(k_ManualSampling));

    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_UseMask_Key, std::make_any<bool>(false));

    DataPath triangleGeometryPath = parentPath.createChildPath(triangleGeometryName);
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_TriangleGeometry_Key, std::make_any<DataPath>(triangleGeometryPath));

    // args.insertOrAssign(PointSampleTriangleGeometryFilter::k_ParentGeometry_Key, std::make_any<DataPath>(DataPath{}));

    DataPath triangleAreasDataPath = triangleGeometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleAreasName);
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_TriangleAreasArrayPath_Key, std::make_any<DataPath>(triangleAreasDataPath));

    DataPath maskDataPath = {};
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskDataPath));

    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_VertexParentGroup_Key, std::make_any<DataPath>(parentPath));
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_VertexGeometryName_Key, std::make_any<StringParameter::ValueType>(vertexGeometryName));
    DataPath vertexDataGroupPath = parentPath.createChildPath(vertexGeometryName).createChildPath(vertexNodeDataGroup);
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_VertexData_DataPath_Key, std::make_any<DataPath>(vertexDataGroupPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    DataPath vertGeometryDataPath = parentPath.createChildPath(vertexGeometryName);
    VertexGeom& vertGeom = dataGraph.getDataRefAs<VertexGeom>(vertGeometryDataPath);
    usize numVerts = vertGeom.getNumberOfVertices();
    AbstractGeometry::SharedVertexList* vertices = vertGeom.getVertices();
    std::array<float, 6> minMaxVerts = FindMinMaxCoord(vertices, numVerts);

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(triangleGeometryPath);
    AbstractGeometry::SharedVertexList* triVerts = triangleGeom.getVertices();
    usize triNumVerts = triangleGeom.getNumberOfVertices();
    std::array<float, 6> minMaxTriVerts = FindMinMaxCoord(triVerts, triNumVerts);

    // Compare the min/max vertex coords from the input Triangle Geometry and the output vertex geometry. These
    // should be without reasonable bounds of each other. This test will only catch if the algorithm has
    // completely gone off the rails.
    for(size_t i = 0; i < 6; i++)
    {
      REQUIRE((minMaxVerts[i] > minMaxTriVerts[i] - 1.0 && minMaxVerts[i] < minMaxTriVerts[i] + 1));
    }

    // We need to insert this small data set for the XDMF to work correctly.
    DataPath xdmfVertsDataPath = parentPath.createChildPath(vertexGeometryName).createChildPath("Verts");
    DataObject::IdType parentId = dataGraph.getId(parentPath.createChildPath(vertexGeometryName)).value();
    std::vector<usize> tupleShape = {vertGeom.getNumberOfVertices()};
    std::vector<usize> componentShape = {1};
    DataArray<int64_t>* vertsArray = DataArray<int64_t>::CreateWithStore<DataStore<int64_t>>(dataGraph, "Verts", tupleShape, componentShape, parentId);
    for(int64_t i = 0; i < tupleShape[0]; i++)
    {
      (*vertsArray)[i] = i;
    }
    std::string outputFilePath = fmt::format("{}/{}", unit_test::k_BinaryDir, k_OutputFile);
    // std::cout << "Writing Output file to " << outputFilePath << std::endl;
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(outputFilePath);
    H5::FileWriter fileWriter = std::move(result.value());

    herr_t err = dataGraph.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  }
}

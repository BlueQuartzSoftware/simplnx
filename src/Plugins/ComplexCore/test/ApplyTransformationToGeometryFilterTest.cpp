#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"

#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

namespace
{
const std::vector<float> s_ManualVertices = {
    0.00000,    19.00000, 3.20000,  -40.35763,  19.00000, 3.20000,  0.00000,    19.00000, 0.00000,  -40.35763,  19.00000, 0.00000,  0.00000,    0.00000,  3.20000,  0.00000,    0.00000,  -0.00000,
    -40.35763,  0.00000,  3.20000,  -40.35763,  0.00000,  -0.00000, -45.55068,  1.30760,  -0.00000, -45.55068,  1.30760,  3.20000,  -50.82293,  2.24626,  -0.00000, -50.82293,  2.24626,  3.20000,
    -56.14818,  2.81133,  -0.00000, -56.14818,  2.81133,  3.20000,  -61.50000,  3.00000,  -0.00000, -61.50000,  3.00000,  3.20000,  -118.50000, 3.00000,  3.20000,  -118.50000, 3.00000,  -0.00000,
    -123.85180, 2.81133,  -0.00000, -123.85180, 2.81133,  3.20000,  -129.17709, 2.24626,  -0.00000, -129.17709, 2.24626,  3.20000,  -134.44930, 1.30760,  -0.00000, -134.44930, 1.30760,  3.20000,
    -139.64240, 0.00000,  -0.00000, -139.64240, 0.00000,  3.20000,  -180.00000, 0.00000,  3.20000,  -180.00000, 0.00000,  -0.00000, -180.00000, 19.00000, 3.20000,  -180.00000, 19.00000, 0.00000,
    -139.64240, 19.00000, 3.20000,  -139.64240, 19.00000, 0.00000,  -134.44930, 17.69240, -0.00000, -134.44930, 17.69240, 3.20000,  -129.17709, 16.75374, -0.00000, -129.17709, 16.75374, 3.20000,
    -123.85180, 16.18867, -0.00000, -123.85180, 16.18867, 3.20000,  -118.50000, 16.00000, -0.00000, -118.50000, 16.00000, 3.20000,  -61.50000,  16.00000, 3.20000,  -61.50000,  16.00000, -0.00000,
    -56.14818,  16.18867, -0.00000, -56.14818,  16.18867, 3.20000,  -50.82293,  16.75374, -0.00000, -50.82293,  16.75374, 3.20000,  -45.55068,  17.69240, -0.00000, -45.55068,  17.69240, 3.20000};

const std::vector<float> s_RotationVertices = {
    -13.43503, 13.43503,  3.20000,  15.10213, 41.97218,  3.20000,  -13.43503, 13.43503,  0.00000,  15.10213,  41.97218,  0.00000,  0.00000,   0.00000,   3.20000,  0.00000,   0.00000,   -0.00000,
    28.53716,  28.53716,  3.20000,  28.53716, 28.53716,  -0.00000, 31.28458,  33.13381,  -0.00000, 31.28458,  33.13381,  3.20000,  34.34889,  37.52558,  -0.00000, 34.34889,  37.52558,  3.20000,
    37.71484,  41.69067,  -0.00000, 37.71484, 41.69067,  3.20000,  41.36575,  45.60839,  -0.00000, 41.36575,  45.60839,  3.20000,  81.67083,  85.91348,  3.20000,  81.67083,  85.91348,  -0.00000,
    85.58854,  89.56435,  -0.00000, 85.58854, 89.56435,  3.20000,  89.75365,  92.93034,  -0.00000, 89.75365,  92.93034,  3.20000,  94.14539,  95.99462,  -0.00000, 94.14539,  95.99462,  3.20000,
    98.74208,  98.74208,  -0.00000, 98.74208, 98.74208,  3.20000,  127.27922, 127.27922, 3.20000,  127.27922, 127.27922, -0.00000, 113.84419, 140.71425, 3.20000,  113.84419, 140.71425, 0.00000,
    85.30705,  112.17711, 3.20000,  85.30705, 112.17711, 0.00000,  82.55959,  107.58042, -0.00000, 82.55959,  107.58042, 3.20000,  79.49532,  103.18867, -0.00000, 79.49532,  103.18867, 3.20000,
    76.12933,  99.02357,  -0.00000, 76.12933, 99.02357,  3.20000,  72.47845,  95.10586,  -0.00000, 72.47845,  95.10586,  3.20000,  32.17336,  54.80078,  3.20000,  32.17336,  54.80078,  -0.00000,
    28.25564,  51.14987,  -0.00000, 28.25564, 51.14987,  3.20000,  24.09056,  47.78392,  -0.00000, 24.09056,  47.78392,  3.20000,  19.69878,  44.71961,  -0.00000, 19.69878,  44.71961,  3.20000};

const std::vector<float> s_ScaleVertices = {
    0.00000,   57.00000, 12.80000, 80.71526,  57.00000, 12.80000, 0.00000,   57.00000, 0.00000,  80.71526,  57.00000, 0.00000,  0.00000,   0.00000,  12.80000, 0.00000,   0.00000,  -0.00000,
    80.71526,  0.00000,  12.80000, 80.71526,  0.00000,  -0.00000, 91.10136,  3.92280,  -0.00000, 91.10136,  3.92280,  12.80000, 101.64586, 6.73878,  -0.00000, 101.64586, 6.73878,  12.80000,
    112.29636, 8.43399,  -0.00000, 112.29636, 8.43399,  12.80000, 123.00000, 9.00000,  -0.00000, 123.00000, 9.00000,  12.80000, 237.00000, 9.00000,  12.80000, 237.00000, 9.00000,  -0.00000,
    247.70360, 8.43399,  -0.00000, 247.70360, 8.43399,  12.80000, 258.35419, 6.73878,  -0.00000, 258.35419, 6.73878,  12.80000, 268.89859, 3.92280,  -0.00000, 268.89859, 3.92280,  12.80000,
    279.28479, 0.00000,  -0.00000, 279.28479, 0.00000,  12.80000, 360.00000, 0.00000,  12.80000, 360.00000, 0.00000,  -0.00000, 360.00000, 57.00000, 12.80000, 360.00000, 57.00000, 0.00000,
    279.28479, 57.00000, 12.80000, 279.28479, 57.00000, 0.00000,  268.89859, 53.07721, -0.00000, 268.89859, 53.07721, 12.80000, 258.35419, 50.26122, -0.00000, 258.35419, 50.26122, 12.80000,
    247.70360, 48.56601, -0.00000, 247.70360, 48.56601, 12.80000, 237.00000, 48.00000, -0.00000, 237.00000, 48.00000, 12.80000, 123.00000, 48.00000, 12.80000, 123.00000, 48.00000, -0.00000,
    112.29636, 48.56601, -0.00000, 112.29636, 48.56601, 12.80000, 101.64586, 50.26122, -0.00000, 101.64586, 50.26122, 12.80000, 91.10136,  53.07721, -0.00000, 91.10136,  53.07721, 12.80000};

const std::vector<float> s_TranslationVertices = {
    100.00000, 119.00000, 103.20000, 140.35764, 119.00000, 103.20000, 100.00000, 119.00000, 100.00000, 140.35764, 119.00000, 100.00000, 100.00000, 100.00000, 103.20000, 100.00000,
    100.00000, 100.00000, 140.35764, 100.00000, 103.20000, 140.35764, 100.00000, 100.00000, 145.55067, 101.30760, 100.00000, 145.55067, 101.30760, 103.20000, 150.82294, 102.24626,
    100.00000, 150.82294, 102.24626, 103.20000, 156.14818, 102.81133, 100.00000, 156.14818, 102.81133, 103.20000, 161.50000, 103.00000, 100.00000, 161.50000, 103.00000, 103.20000,
    218.50000, 103.00000, 103.20000, 218.50000, 103.00000, 100.00000, 223.85181, 102.81133, 100.00000, 223.85181, 102.81133, 103.20000, 229.17709, 102.24626, 100.00000, 229.17709,
    102.24626, 103.20000, 234.44930, 101.30760, 100.00000, 234.44930, 101.30760, 103.20000, 239.64240, 100.00000, 100.00000, 239.64240, 100.00000, 103.20000, 280.00000, 100.00000,
    103.20000, 280.00000, 100.00000, 100.00000, 280.00000, 119.00000, 103.20000, 280.00000, 119.00000, 100.00000, 239.64240, 119.00000, 103.20000, 239.64240, 119.00000, 100.00000,
    234.44930, 117.69240, 100.00000, 234.44930, 117.69240, 103.20000, 229.17709, 116.75374, 100.00000, 229.17709, 116.75374, 103.20000, 223.85181, 116.18867, 100.00000, 223.85181,
    116.18867, 103.20000, 218.50000, 116.00000, 100.00000, 218.50000, 116.00000, 103.20000, 161.50000, 116.00000, 103.20000, 161.50000, 116.00000, 100.00000, 156.14818, 116.18867,
    100.00000, 156.14818, 116.18867, 103.20000, 150.82294, 116.75374, 100.00000, 150.82294, 116.75374, 103.20000, 145.55067, 117.69240, 100.00000, 145.55067, 117.69240, 103.20000,
};

const std::string triangleGeometryName = "[Triangle Geometry]";
const std::string triangleFaceDataGroupName = "Face Data";
const std::string normalsDataArrayName = "Normals";

} // namespace

void ReadSTLFile(DataStructure& dataGraph)
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

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter_Translation", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  DataStructure dataGraph;
  ReadSTLFile(dataGraph);
  DataPath geometryPath = DataPath({triangleGeometryName});

  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;
    std::string triangleAreasName = "Triangle Areas";

    // Create default Parameters for the filter.
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleAreasName);
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_GeometryToTransform_Key, std::make_any<DataPath>(geometryPath));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformType_Key, std::make_any<complex::ChoicesParameter::ValueType>(4));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({100.0F, 100.0F, 100.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }
  // Validate the output data
  {
    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
    Float32Array* vertices = triangleGeom.getVertices();
    bool verticesEqual = true;
    for(size_t eIdx = 0; eIdx < vertices->getSize(); eIdx++)
    {
      if(fabsf((*vertices)[eIdx] - ::s_TranslationVertices[eIdx]) > 0.0001F)
      {
        verticesEqual = false;
        break;
      }
    }
    REQUIRE(verticesEqual);
  }
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_translation.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter_Rotation", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  DataStructure dataGraph;
  ReadSTLFile(dataGraph);
  DataPath geometryPath = DataPath({triangleGeometryName});

  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;
    std::string triangleAreasName = "Triangle Areas";

    // Create default Parameters for the filter.
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleAreasName);
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_GeometryToTransform_Key, std::make_any<DataPath>(geometryPath));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformType_Key, std::make_any<complex::ChoicesParameter::ValueType>(3));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_RotationAxisAngle_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F, 45.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }
  // Validate the output data
  {
    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
    Float32Array* vertices = triangleGeom.getVertices();
    bool verticesEqual = true;
    for(size_t eIdx = 0; eIdx < vertices->getSize(); eIdx++)
    {
      if(fabsf((*vertices)[eIdx] - ::s_RotationVertices[eIdx]) > 0.0001F)
      {
        verticesEqual = false;
        break;
      }
    }
    REQUIRE(verticesEqual);
  }
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_rotation.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter_Scale", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  DataStructure dataGraph;
  ReadSTLFile(dataGraph);
  DataPath geometryPath = DataPath({triangleGeometryName});

  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;
    std::string triangleAreasName = "Triangle Areas";

    // Create default Parameters for the filter.
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleAreasName);
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_GeometryToTransform_Key, std::make_any<DataPath>(geometryPath));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformType_Key, std::make_any<complex::ChoicesParameter::ValueType>(5));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({2.0F, 3.0F, 4.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }
  // Validate the output data
  {
    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
    Float32Array* vertices = triangleGeom.getVertices();
    bool verticesEqual = true;
    for(size_t eIdx = 0; eIdx < vertices->getSize(); eIdx++)
    {
      if(fabsf((*vertices)[eIdx] - ::s_ScaleVertices[eIdx]) > 0.0001F)
      {
        verticesEqual = false;
        break;
      }
    }
    REQUIRE(verticesEqual);
  }
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_scale.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter_Manual", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  DataStructure dataGraph;
  ReadSTLFile(dataGraph);
  DataPath geometryPath = DataPath({triangleGeometryName});

  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;
    std::string triangleAreasName = "Triangle Areas";

    // Create default Parameters for the filter.
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleAreasName);
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_GeometryToTransform_Key, std::make_any<DataPath>(geometryPath));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformType_Key, std::make_any<complex::ChoicesParameter::ValueType>(2));

    // This should reflect the geometry across the x-axis.
    DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}, {"R0", "R1", "R2", "R3"}, {"C0", "C1", "C2", "C3"}};
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<complex::DynamicTableParameter::ValueType>(dynamicTable));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }
  // Validate the output data
  {
    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
    Float32Array* vertices = triangleGeom.getVertices();
    bool verticesEqual = true;
    for(size_t eIdx = 0; eIdx < vertices->getSize(); eIdx++)
    {
      if(fabsf((*vertices)[eIdx] - ::s_ManualVertices[eIdx]) > 0.0001F)
      {
        verticesEqual = false;
        break;
      }
    }
    REQUIRE(verticesEqual);
  }
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_manual.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter_Precomputed", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  DataStructure dataGraph;
  ReadSTLFile(dataGraph);
  DataPath geometryPath = DataPath({triangleGeometryName});

  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;
    std::string triangleAreasName = "Triangle Areas";
    std::string precomputedName = "Precomputed Matrix";

    std::vector<size_t> tupleShape = {4, 4};
    std::vector<size_t> componentShape = {1};
    size_t i = 0;
    Float32Array* precomputedData = complex::UnitTest::CreateTestDataArray<float32>(dataGraph, precomputedName, tupleShape, componentShape, 0);
    (*precomputedData)[i++] = -1.0F;
    (*precomputedData)[i++] = 0.0F;
    (*precomputedData)[i++] = 0.0F;
    (*precomputedData)[i++] = 0.0F;

    (*precomputedData)[i++] = 0.0F;
    (*precomputedData)[i++] = 1.0F;
    (*precomputedData)[i++] = 0.0F;
    (*precomputedData)[i++] = 0.0F;

    (*precomputedData)[i++] = 0.0F;
    (*precomputedData)[i++] = 0.0F;
    (*precomputedData)[i++] = 1.0F;
    (*precomputedData)[i++] = 0.0F;

    (*precomputedData)[i++] = 0.0F;
    (*precomputedData)[i++] = 0.0F;
    (*precomputedData)[i++] = 0.0F;
    (*precomputedData)[i++] = 1.0F;

    // Create default Parameters for the filter.
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleAreasName);
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_GeometryToTransform_Key, std::make_any<DataPath>(geometryPath));

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformType_Key, std::make_any<complex::ChoicesParameter::ValueType>(1));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>({precomputedName}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }
  // Validate the output data
  {
    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
    Float32Array* vertices = triangleGeom.getVertices();
    bool verticesEqual = true;
    for(size_t eIdx = 0; eIdx < vertices->getSize(); eIdx++)
    {
      if(fabsf((*vertices)[eIdx] - ::s_ManualVertices[eIdx]) > 0.0001F)
      {
        verticesEqual = false;
        break;
      }
    }
    REQUIRE(verticesEqual);
  }
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_precomputed.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

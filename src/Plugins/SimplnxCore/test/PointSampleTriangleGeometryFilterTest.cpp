#include "SimplnxCore/Filters/PointSampleTriangleGeometryFilter.hpp"
#include "SimplnxCore/Filters/Algorithms/TupleTransfer.hpp"
#include "SimplnxCore/Filters/ComputeTriangleAreasFilter.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <algorithm>
#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <type_traits>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

static std::array<float, 6> FindMinMaxCoord(IGeometry::SharedVertexList* vertices, usize numVerts)
{
  std::array<float, 6> minMaxVerts = {std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), std::numeric_limits<float>::max(),
                                      std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), std::numeric_limits<float>::min()};

  for(usize v = 0; v < numVerts; v++)
  {
    minMaxVerts[0] = std::min<float>((*vertices)[v * 3], minMaxVerts[0]);
    minMaxVerts[1] = std::max<float>((*vertices)[v * 3], minMaxVerts[1]);

    minMaxVerts[2] = std::min<float>((*vertices)[v * 3 + 1], minMaxVerts[2]);
    minMaxVerts[3] = std::max<float>((*vertices)[v * 3 + 1], minMaxVerts[3]);

    minMaxVerts[4] = std::min<float>((*vertices)[v * 3 + 2], minMaxVerts[4]);
    minMaxVerts[5] = std::max<float>((*vertices)[v * 3 + 2], minMaxVerts[5]);
  }

  //  for(auto& coord : minMaxVerts)
  //  {
  //    std::cout << coord << ",";
  //  }
  //  std::cout << std::endl;

  return minMaxVerts;
}

TEST_CASE("SimplnxCore::PointSampleTriangleGeometryFilter", "[DREAM3DReview][PointSampleTriangleGeometryFilter]")
{
  UnitTest::LoadPlugins();

  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = INodeGeometry2D::k_FaceAttributeMatrixName;
  std::string normalsDataArrayName = "FaceNormals";
  std::string triangleAreasName = "Triangle Areas";
  std::string vertexGeometryName = "[Vertex Geometry]";
  std::string vertexNodeDataGroup = "Vertex Data";
  DataStructure dataStructure;

  // Read in the STL File to load a Triangle Geometry to sample
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

  // Calculate the Triangle Areas
  {
    ComputeTriangleAreasFilter filter;
    Arguments args;

    DataPath geometryPath = DataPath({triangleGeometryName});

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeTriangleAreasFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(ComputeTriangleAreasFilter::k_CalculatedAreasDataName_Key, std::make_any<std::string>(triangleAreasName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(geometryPath);
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleGeom.getFaceAttributeMatrix()->getName()).createChildPath(triangleAreasName);

    // Let's sum up all the areas.
    Float64Array& faceAreas = dataStructure.getDataRefAs<Float64Array>(triangleAreasDataPath);
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
    const std::string kOutputFile("PointSampleTriangleGeometryTest.dream3d");

    // Instantiate the filter, a DataStructure object and an Arguments Object
    PointSampleTriangleGeometryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_NumberOfSamples_Key, std::make_any<int32>(200));
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_UseMask_Key, std::make_any<bool>(false));

    DataPath triangleGeometryPath({triangleGeometryName});
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_TriangleGeometry_Key, std::make_any<DataPath>(triangleGeometryPath));

    DataPath triangleAreasDataPath = triangleGeometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleAreasName);
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_TriangleAreasArrayPath_Key, std::make_any<DataPath>(triangleAreasDataPath));

    DataPath maskDataPath = {};
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskDataPath));

    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

    DataPath vertGeometryDataPath({vertexGeometryName});
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(vertGeometryDataPath));
    DataPath vertexDataGroupPath = vertGeometryDataPath.createChildPath(vertexNodeDataGroup);
    args.insertOrAssign(PointSampleTriangleGeometryFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(vertexNodeDataGroup));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    VertexGeom& vertGeom = dataStructure.getDataRefAs<VertexGeom>(vertGeometryDataPath);
    usize numVerts = vertGeom.getNumberOfVertices();
    IGeometry::SharedVertexList* verticesPtr = vertGeom.getVertices();
    std::array<float, 6> minMaxVerts = FindMinMaxCoord(verticesPtr, numVerts);

    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeometryPath);
    IGeometry::SharedVertexList* triVertsPtr = triangleGeom.getVertices();
    usize triNumVerts = triangleGeom.getNumberOfVertices();
    std::array<float, 6> minMaxTriVerts = FindMinMaxCoord(triVertsPtr, triNumVerts);

    // We need to insert this small data set for the XDMF to work correctly.
    DataPath xdmfVertsDataPath = vertGeometryDataPath.createChildPath("Verts");
    DataObject::IdType parentId = dataStructure.getId(vertGeometryDataPath).value();
    ShapeType tupleShape = {vertGeom.getNumberOfVertices()};
    ShapeType componentShape = {1};
    DataArray<int64_t>* vertsArrayPtr = DataArray<int64_t>::CreateWithStore<DataStore<int64_t>>(dataStructure, "Verts", tupleShape, componentShape, parentId);
    for(int64_t i = 0; i < tupleShape[0]; i++)
    {
      (*vertsArrayPtr)[i] = i;
    }
    std::string outputFilePath = fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, kOutputFile);
    // std::cout << "Writing Output file to " << outputFilePath << std::endl;
    nx::core::hdf5::FileIO fileWriter = nx::core::hdf5::FileIO::WriteFile(outputFilePath);

    auto resultH5 = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
    //    for(size_t i = 0; i < 6; i++)
    //    {
    //      printf("%0.8f    %0.8f\n", minMaxTriVerts[i], minMaxVerts[i]);
    //    }

    // We are just going to make sure that the min value is more positive than a specific
    // value for this data set and that the max value is less positive than a specific
    // value for this data set. We are basically allowing the generated points to
    // fall within the original 3D box that encompasses the triangle geometry
    // + 0.5 in all directions. This should allow for the randomness in the algorithm
    // but catch if the algorithm goes off the deep end. Of course if the algorithm
    // goes off the deep end and puts all the new points on top of each other this
    // would not be covered in this example. So we should probably check some sort
    // of spread of the data to ensure that the new points are reasonably distributed
    // over the triangle geometry.
    REQUIRE(minMaxVerts[0] >= -0.5f);
    REQUIRE(minMaxVerts[1] <= 180.5f);

    REQUIRE(minMaxVerts[2] >= -0.5f);
    REQUIRE(minMaxVerts[3] <= 19.5f);

    REQUIRE(minMaxVerts[4] >= -0.5f);
    REQUIRE(minMaxVerts[5] <= 3.7f);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PointSampleTriangleGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][PointSampleTriangleGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "PointSampleTriangleGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "PointSampleTriangleGeometryFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<PointSampleTriangleGeometryFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<int32>(PointSampleTriangleGeometryFilter::k_NumberOfSamples_Key) == 5);
      CHECK(args.value<bool>(PointSampleTriangleGeometryFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(PointSampleTriangleGeometryFilter::k_TriangleGeometry_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(PointSampleTriangleGeometryFilter::k_TriangleAreasArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(PointSampleTriangleGeometryFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
      // Complex type (StringToDataPathFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<std::string>(PointSampleTriangleGeometryFilter::k_VertexDataGroupName_Key) == "TestName");
    }
  }
}

TEMPLATE_TEST_CASE("SimplnxCore::PointSampleTriangleGeometryFilter: Transfer complete face tuples", "[SimplnxCore][PointSampleTriangleGeometryFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64, float32, float64, bool)
{
  UnitTest::LoadPlugins();
  const usize numComponents = GENERATE(usize{3}, usize{1}, usize{2});
  const usize numTriangles = GENERATE(usize{4}, usize{1});
  const int32 numSamples = GENERATE(2, 8);
  CAPTURE(numComponents, numTriangles, numSamples);

  DataStructure dataStructure;
  auto* trianglesPtr = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(trianglesPtr != nullptr);
  auto* coordinatesPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Vertices", {3 * numTriangles}, {3}, trianglesPtr->getId());
  auto* facesPtr = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, "Faces", {numTriangles}, {3}, trianglesPtr->getId());
  REQUIRE(coordinatesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  for(usize face = 0; face < numTriangles; ++face)
  {
    // Each triangle has A=(0,0,z), B=(2,0,z), C=(1,2,z), with a separate plane.
    const std::array<float32, 9> vertices = {0, 0, static_cast<float32>(10 * face), 2, 0, static_cast<float32>(10 * face), 1, 2, static_cast<float32>(10 * face)};
    for(usize index = 0; index < vertices.size(); ++index)
    {
      (*coordinatesPtr)[9 * face + index] = vertices[index];
    }
    for(usize corner = 0; corner < 3; ++corner)
    {
      (*facesPtr)[3 * face + corner] = 3 * face + corner;
    }
  }
  trianglesPtr->setVertices(*coordinatesPtr);
  trianglesPtr->setFaceList(*facesPtr);
  auto* faceDataPtr = AttributeMatrix::Create(dataStructure, "Face Data", {numTriangles}, trianglesPtr->getId());
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", {3 * numTriangles}, trianglesPtr->getId());
  REQUIRE(faceDataPtr != nullptr);
  REQUIRE(vertexDataPtr != nullptr);
  trianglesPtr->setFaceAttributeMatrix(*faceDataPtr);
  trianglesPtr->setVertexAttributeMatrix(*vertexDataPtr);
  auto* areasPtr = UnitTest::CreateTestDataArray<float64>(dataStructure, "Areas", {numTriangles}, {1}, faceDataPtr->getId());
  auto* valuesPtr = UnitTest::CreateTestDataArray<TestType>(dataStructure, "Values", {numTriangles}, {numComponents}, faceDataPtr->getId());
  REQUIRE(areasPtr != nullptr);
  REQUIRE(valuesPtr != nullptr);
  areasPtr->fill(2.0);
  const auto expectedValue = [](usize face, usize component) -> TestType {
    if constexpr(std::is_same_v<TestType, bool>)
    {
      constexpr std::array<std::array<bool, 3>, 4> k_Values = {{{false, true, false}, {true, false, true}, {true, true, false}, {false, false, true}}};
      return k_Values[face][component];
    }
    else
    {
      constexpr std::array<std::array<int32, 3>, 4> k_Values = {{{11, 12, 13}, {21, 22, 23}, {31, 32, 33}, {41, 42, 43}}};
      return static_cast<TestType>(k_Values[face][component]);
    }
  };
  for(usize face = 0; face < numTriangles; ++face)
  {
    for(usize component = 0; component < numComponents; ++component)
    {
      (*valuesPtr)[face * numComponents + component] = expectedValue(face, component);
    }
  }

  PointSampleTriangleGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(PointSampleTriangleGeometryFilter::k_NumberOfSamples_Key, std::make_any<int32>(numSamples));
  args.insertOrAssign(PointSampleTriangleGeometryFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(PointSampleTriangleGeometryFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(PointSampleTriangleGeometryFilter::k_SeedValue_Key, std::make_any<uint64>(2205));
  args.insertOrAssign(PointSampleTriangleGeometryFilter::k_TriangleGeometry_Key, std::make_any<DataPath>(DataPath({"Triangles"})));
  args.insertOrAssign(PointSampleTriangleGeometryFilter::k_TriangleAreasArrayPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Face Data", "Areas"})));
  args.insertOrAssign(PointSampleTriangleGeometryFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath({"Triangles", "Face Data", "Values"})}));
  args.insertOrAssign(PointSampleTriangleGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(DataPath({"Samples"})));
  args.insertOrAssign(PointSampleTriangleGeometryFilter::k_VertexDataGroupName_Key, std::make_any<std::string>("Vertex Data"));
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  const DataPath outputPath({"Samples"});
  const DataPath outputValuesPath({"Samples", "Vertex Data", "Values"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<VertexGeom>(outputPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputValuesPath));
  const auto& output = dataStructure.getDataRefAs<VertexGeom>(outputPath);
  const auto& outputValues = dataStructure.getDataRefAs<DataArray<TestType>>(outputValuesPath);
  REQUIRE(output.getNumberOfVertices() == static_cast<usize>(numSamples));
  REQUIRE(outputValues.getNumberOfTuples() == static_cast<usize>(numSamples));
  REQUIRE(outputValues.getNumberOfComponents() == numComponents);
  for(usize sample = 0; sample < static_cast<usize>(numSamples); ++sample)
  {
    const auto point = output.getVertexCoordinate(sample);
    // Solve P=A+v(B-A)+w(C-A): w=y/2, v=(x-w)/2, u=1-v-w.
    const float64 w = point[1] / 2.0;
    const float64 v = (point[0] - w) / 2.0;
    const float64 u = 1.0 - v - w;
    constexpr float64 k_Tolerance = 1.0e-5;
    usize containingTriangle = numTriangles;
    usize matches = 0;
    for(usize face = 0; face < numTriangles; ++face)
    {
      if(std::abs(point[2] - static_cast<float64>(10 * face)) <= k_Tolerance && u >= -k_Tolerance && v >= -k_Tolerance && w >= -k_Tolerance)
      {
        containingTriangle = face;
        ++matches;
      }
    }
    CAPTURE(sample, containingTriangle);
    REQUIRE(matches == 1);
    for(usize component = 0; component < numComponents; ++component)
    {
      CAPTURE(component);
      CHECK(outputValues[sample * numComponents + component] == expectedValue(containingTriangle, component));
    }
  }
  for(usize face = 0; face < numTriangles; ++face)
  {
    CHECK((*areasPtr)[face] == 2.0);
    for(usize component = 0; component < numComponents; ++component)
    {
      CHECK((*valuesPtr)[face * numComponents + component] == expectedValue(face, component));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PointSampleTriangleGeometryFilter: Transfer complete feature tuples", "[SimplnxCore][PointSampleTriangleGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* featuresPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Features", {3}, {3});
  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {2}, {1});
  auto* samplesPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Samples", {8}, {3});
  REQUIRE(featuresPtr != nullptr);
  REQUIRE(featureIdsPtr != nullptr);
  REQUIRE(samplesPtr != nullptr);
  const std::array<int32, 9> expected = {11, 12, 13, 21, 22, 23, 31, 32, 33};
  for(usize index = 0; index < expected.size(); ++index)
  {
    (*featuresPtr)[index] = expected[index];
  }
  (*featureIdsPtr)[0] = 2;
  (*featureIdsPtr)[1] = 1;
  samplesPtr->fill(-1);
  TransferFeatureTuple<int32, int32> transfer(dataStructure, DataPath({"Features"}), DataPath({"Samples"}), DataPath({"FeatureIds"}));
  // This shared override is not exposed by the filter, but uses the same tuple-index contract.
  transfer.pointSampleTransfer(1, 0);
  for(usize component = 0; component < 3; ++component)
  {
    REQUIRE((*samplesPtr)[component] == expected[3 + component]);
  }
  transfer.pointSampleTransfer(0, 7);
  for(usize component = 0; component < 3; ++component)
  {
    CHECK((*samplesPtr)[21 + component] == expected[6 + component]);
  }
  for(usize index = 3; index < 21; ++index)
  {
    CHECK((*samplesPtr)[index] == -1);
  }
  for(usize index = 0; index < expected.size(); ++index)
  {
    CHECK((*featuresPtr)[index] == expected[index]);
  }
  CHECK((*featureIdsPtr)[0] == 2);
  CHECK((*featureIdsPtr)[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/QuickSurfaceMeshFilter.hpp"

using namespace complex;
using namespace complex::UnitTest;
using namespace complex::Constants;

TEST_CASE("ComplexCore::QuickSurfaceMeshFilter", "[ComplexCore][QuickSurfaceMeshFilter]")
{
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1.dream3d", unit_test::k_TestDataSourceDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_DataContainer});

  std::vector<size_t> imageDims = {100, 100, 100};
  size_t numTuples = imageDims[0] * imageDims[1] * imageDims[2];
  FloatVec3 imageSpacing = {0.10F, 2.0F, 33.0F};
  FloatVec3 imageOrigin = {
      0.0f,
      22.0f,
      77.0f,
  };

  {
    Arguments args;
    QuickSurfaceMeshFilter filter;

    DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
    DataPath ebsdSanDataPath({k_DataContainer, k_CellData});
    DataPath triangleParentGroup({k_DataContainer});
    // Create default Parameters for the filter.
    args.insertOrAssign(QuickSurfaceMeshFilter::k_GenerateTripleLines_Key, std::make_any<bool>(false));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));

    DataPath gridGeomDataPath({k_DataContainer});
    args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));

    MultiArraySelectionParameter::ValueType selectedArrayPaths = {ebsdSanDataPath.createChildPath(k_ConfidenceIndex), ebsdSanDataPath.createChildPath(k_IPFColors),
                                                                  ebsdSanDataPath.createChildPath(k_ImageQuality), ebsdSanDataPath.createChildPath(k_Phases)};

    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedArrayPaths));

    // args.insertOrAssign(QuickSurfaceMeshFilter::k_ParentDataGroupPath_Key, std::make_any<DataPath>({}));

    DataPath triangleGeometryPath = triangleParentGroup.createChildPath(k_TriangleGeometryName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_TriangleGeometryName_Key, std::make_any<DataPath>(triangleGeometryPath));

    DataPath vertexGroupDataPath = triangleGeometryPath.createChildPath(k_VertexDataGroupName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<DataPath>(vertexGroupDataPath));

    DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<DataPath>(nodeTypeDataPath));

    DataPath faceGroupDataPath = triangleGeometryPath.createChildPath(k_FaceDataGroupName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<DataPath>(faceGroupDataPath));

    DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_FaceLabels);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<DataPath>(faceLabelsDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

    // Check a few things about the generated data.
    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeometryPath);
    AbstractGeometry::SharedTriList* triangle = triangleGeom.getFaces();
    AbstractGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

    REQUIRE(triangle->getNumberOfTuples() == 785088);
    REQUIRE(vertices->getNumberOfTuples() == 356239);

    for(const auto& selectedDataPath : selectedArrayPaths)
    {
      auto* cellDataArray = dataStructure.getDataAs<IDataArray>(selectedDataPath);
      REQUIRE(cellDataArray->getNumberOfTuples() == numTuples);

      DataPath createdDataPath = faceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
      auto* faceDataArray = dataStructure.getDataAs<IDataArray>(createdDataPath);
      REQUIRE(faceDataArray->getNumberOfTuples() == 785088);
    }
  }

  {
    // Write out the DataStructure for later viewing/debugging
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryDir));
    H5::FileWriter fileWriter = std::move(result.value());

    herr_t err = dataStructure.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  }
}

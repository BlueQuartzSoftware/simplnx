
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
namespace
{
std::shared_ptr<DataStructure> CreateEbsdTestDataStructure()
{
  std::shared_ptr<DataStructure> dataGraph = std::shared_ptr<DataStructure>(new DataStructure);

  DataGroup* group = complex::DataGroup::Create(*dataGraph, complex::Constants::k_SmallIN100);
  DataGroup* scanData = complex::DataGroup::Create(*dataGraph, complex::Constants::k_EbsdScanData, group->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(*dataGraph, k_SmallIn100ImageGeom, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  complex::SizeVec3 imageGeomDims = {100, 100, 100};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  std::vector<size_t> compDims = {1};
  std::vector<size_t> tupleDims = {100, 100, 100};

  std::string filePath = complex::unit_test::k_SourceDir.str() + "/data/";

  std::string fileName = "ConfidenceIndex.raw";
  complex::ImportFromBinaryFile<float>(filePath + fileName, k_ConfidenceIndex, dataGraph.get(), tupleDims, compDims, scanData->getId());

  fileName = "FeatureIds.raw";
  complex::ImportFromBinaryFile<int32_t>(filePath + fileName, k_FeatureIds, dataGraph.get(), tupleDims, compDims, scanData->getId());

  fileName = "ImageQuality.raw";
  complex::ImportFromBinaryFile<float>(filePath + fileName, k_ImageQuality, dataGraph.get(), tupleDims, compDims, scanData->getId());

  fileName = "Phases.raw";
  complex::ImportFromBinaryFile<int32_t>(filePath + fileName, k_Phases, dataGraph.get(), tupleDims, compDims, scanData->getId());

  fileName = "IPFColors.raw";
  compDims = {3};
  complex::ImportFromBinaryFile<uint8_t>(filePath + fileName, k_IpfColors, dataGraph.get(), tupleDims, compDims, scanData->getId());

  // Add in another group that is just information about the grid data.
  DataGroup* phaseGroup = complex::DataGroup::Create(*dataGraph, k_PhaseData, group->getId());
  Int32Array::CreateWithStore<Int32DataStore>(*dataGraph, k_LaueClass, {2}, compDims, phaseGroup->getId());

  return dataGraph;
}
} // namespace
TEST_CASE("SurfaceMeshing::QuickSurfaceMeshFilter", "[SurfaceMeshing][QuickSurfaceMeshFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object

  std::shared_ptr<DataStructure> dataGraphPtr = ::CreateEbsdTestDataStructure();
  DataStructure& dataGraph = *dataGraphPtr;

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

    DataPath featureIdsDataPath({k_SmallIN100, k_EbsdScanData, k_FeatureIds});
    DataPath ebsdSanDataPath({k_SmallIN100, k_EbsdScanData});
    DataPath triangleParentGroup({k_SmallIN100});
    // Create default Parameters for the filter.
    args.insertOrAssign(QuickSurfaceMeshFilter::k_GenerateTripleLines_Key, std::make_any<bool>(false));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));

    DataPath gridGeomDataPath({k_SmallIN100, k_EbsdScanData, k_SmallIn100ImageGeom});
    args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));

    MultiArraySelectionParameter::ValueType selectedArrayPaths = {ebsdSanDataPath.createChildPath(k_ConfidenceIndex), ebsdSanDataPath.createChildPath(k_IpfColors),
                                                                  ebsdSanDataPath.createChildPath(k_ImageQuality), ebsdSanDataPath.createChildPath(k_Phases)};

    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedArrayPaths));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_ParentDataGroupPath_Key, std::make_any<DataPath>(triangleParentGroup));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_TriangleGeometryName_Key, std::make_any<std::string>(k_TriangleGeometryName));

    DataPath vertexGroupDataPath = triangleParentGroup.createChildPath(k_TriangleGeometryName).createChildPath(k_VertexDataGroupName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<DataPath>(vertexGroupDataPath));

    DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<DataPath>(nodeTypeDataPath));

    DataPath faceGroupDataPath = triangleParentGroup.createChildPath(k_TriangleGeometryName).createChildPath(k_FaceDataGroupName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<DataPath>(faceGroupDataPath));

    DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_FaceLabels);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<DataPath>(faceLabelsDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    // Check a few things about the generated data.
    DataPath triangleGeometryDataPath = triangleParentGroup.createChildPath(k_TriangleGeometryName);
    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(triangleGeometryDataPath);
    AbstractGeometry::SharedTriList* triangle = triangleGeom.getFaces();
    AbstractGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

    REQUIRE(triangle->getNumberOfTuples() == 785088);
    REQUIRE(vertices->getNumberOfTuples() == 356239);

    for(const auto& selectedDataPath : selectedArrayPaths)
    {
      auto* cellDataArray = dataGraph.getDataAs<IDataArray>(selectedDataPath);
      REQUIRE(cellDataArray->getNumberOfTuples() == numTuples);

      DataPath createdDataPath = faceGroupDataPath.createChildPath(selectedDataPath.getTargetName());
      auto* faceDataArray = dataGraph.getDataAs<IDataArray>(createdDataPath);
      REQUIRE(faceDataArray->getNumberOfTuples() == 785088);
    }
  }

  // Write out the DataStructure for later viewing/debugging
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

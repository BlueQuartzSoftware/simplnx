/**
 * This file is auto generated from the original Plugins/PartitionGeometryFilter
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[PartitionGeometryFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>
#include <filesystem>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ImportDREAM3DFilter.hpp"
#include "ComplexCore/Filters/PartitionGeometryFilter.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
const fs::path k_TestFilesPath = fs::path(unit_test::k_DREAM3DDataDir.str()) / fs::path("TestFiles") / fs::path("PartitionGeometryTest");
const fs::path k_ImageGeomTestFilePath = k_TestFilesPath / fs::path("image_geom.dream3d");
const fs::path k_RectGridGeomTestFilePath = k_TestFilesPath / fs::path("rectgrid_geom.dream3d");
const fs::path k_EdgeGeomTestFilePath = k_TestFilesPath / fs::path("edge_geom.dream3d");
const fs::path k_HexahedralGeomTestFilePath = k_TestFilesPath / fs::path("hexahedral_geom.dream3d");
const fs::path k_QuadGeomTestFilePath = k_TestFilesPath / fs::path("quad_geom.dream3d");
const fs::path k_TetrahedralGeomTestFilePath = k_TestFilesPath / fs::path("tetrahedral_geom.dream3d");
const fs::path k_TriangleGeomTestFilePath = k_TestFilesPath / fs::path("triangle_geom.dream3d");
const fs::path k_VertexGeomTestFilePath = k_TestFilesPath / fs::path("vertex_geom.dream3d");
const fs::path k_PlanalXYNodeGeomTestFilePath = k_TestFilesPath / fs::path("planal_xy_node_geom.dream3d");
const fs::path k_PlanalXZNodeGeomTestFilePath = k_TestFilesPath / fs::path("planal_xz_node_geom.dream3d");
const fs::path k_PlanalYZNodeGeomTestFilePath = k_TestFilesPath / fs::path("planal_yz_node_geom.dream3d");

// -----------------------------------------------------------------------------
Arguments createBasicPartitionGeometryArguments(const DataPath& inputGeometryPath, const DataPath& attrMatrixPath, const std::string& partitionIdsArrayName, const IntVec3& numOfPartitionsPerAxis,
                                                const std::optional<DataPath>& maskArrayPath)
{
  Arguments args;
  args.insert(PartitionGeometryFilter::k_PartitioningMode_Key, static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::Basic));
  args.insert(PartitionGeometryFilter::k_NumberOfCellsPerAxis_Key, std::vector<int32>{numOfPartitionsPerAxis.getX(), numOfPartitionsPerAxis.getY(), numOfPartitionsPerAxis.getZ()});
  args.insert(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key, attrMatrixPath);
  args.insert(PartitionGeometryFilter::k_PartitionIdsArrayName_Key, partitionIdsArrayName);
  args.insert(PartitionGeometryFilter::k_InputGeometryToPartition_Key, inputGeometryPath);

  if(maskArrayPath.has_value())
  {
    args.insert(PartitionGeometryFilter::k_UseVertexMask_Key, true);
    args.insert(PartitionGeometryFilter::k_VertexMaskPath_Key, *maskArrayPath);
  }

  return args;
}

// -----------------------------------------------------------------------------
Arguments createAdvancedPartitionGeometryArguments(const DataPath& inputGeometryPath, const DataPath& attrMatrixPath, const std::string& partitionIdsArrayName, const IntVec3& numOfPartitionsPerAxis,
                                                   const FloatVec3& partitioningSchemeOrigin, const FloatVec3& lengthPerPartition)
{
  Arguments args;
  args.insert(PartitionGeometryFilter::k_PartitioningMode_Key, static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::Advanced));
  args.insert(PartitionGeometryFilter::k_NumberOfCellsPerAxis_Key, std::vector<int32>{numOfPartitionsPerAxis.getX(), numOfPartitionsPerAxis.getY(), numOfPartitionsPerAxis.getZ()});
  args.insert(PartitionGeometryFilter::k_PartitionGridOrigin_Key, std::vector<float>{partitioningSchemeOrigin.getX(), partitioningSchemeOrigin.getY(), partitioningSchemeOrigin.getZ()});
  args.insert(PartitionGeometryFilter::k_CellLength_Key, std::vector<float>{lengthPerPartition.getX(), lengthPerPartition.getY(), lengthPerPartition.getZ()});
  args.insert(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key, attrMatrixPath);
  args.insert(PartitionGeometryFilter::k_PartitionIdsArrayName_Key, partitionIdsArrayName);
  args.insert(PartitionGeometryFilter::k_InputGeometryToPartition_Key, inputGeometryPath);
  return args;
}

// -----------------------------------------------------------------------------
Arguments createBoundingBoxPartitionGeometryArguments(const DataPath& inputGeometryPath, const DataPath& attrMatrixPath, const std::string& partitionIdsArrayName,
                                                      const IntVec3& numOfPartitionsPerAxis, const FloatVec3& lowerLeftCoord, const FloatVec3& upperRightCoord)
{
  Arguments args;
  args.insert(PartitionGeometryFilter::k_PartitioningMode_Key, static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::BoundingBox));
  args.insert(PartitionGeometryFilter::k_NumberOfCellsPerAxis_Key, std::vector<int32>{numOfPartitionsPerAxis.getX(), numOfPartitionsPerAxis.getY(), numOfPartitionsPerAxis.getZ()});
  args.insert(PartitionGeometryFilter::k_MinGridCoord_Key, std::vector<float>{lowerLeftCoord.getX(), lowerLeftCoord.getY(), lowerLeftCoord.getZ()});
  args.insert(PartitionGeometryFilter::k_MaxGridCoord_Key, std::vector<float>{upperRightCoord.getX(), upperRightCoord.getY(), upperRightCoord.getZ()});
  args.insert(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key, attrMatrixPath);
  args.insert(PartitionGeometryFilter::k_PartitionIdsArrayName_Key, partitionIdsArrayName);
  args.insert(PartitionGeometryFilter::k_InputGeometryToPartition_Key, inputGeometryPath);

  return args;
}

// -----------------------------------------------------------------------------
Arguments createExistingPartitioningSchemeGeometryArguments(const DataPath& inputGeometryPath, const DataPath& attrMatrixPath, const std::string& partitionIdsArrayName, const DataPath& existingPSPath)
{
  Arguments args;
  args.insert(PartitionGeometryFilter::k_PartitioningMode_Key, static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid));
  args.insert(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key, attrMatrixPath);
  args.insert(PartitionGeometryFilter::k_PartitionIdsArrayName_Key, partitionIdsArrayName);
  args.insert(PartitionGeometryFilter::k_ExistingPartitionGridPath_Key, existingPSPath);
  args.insert(PartitionGeometryFilter::k_InputGeometryToPartition_Key, inputGeometryPath);
  return args;
}
} // namespace

TEST_CASE("ComplexCore::PartitionGeometryFilter: Valid filter execution", "[Plugins][PartitionGeometryFilter]")
{
  const std::string kDataInputArchive = "PartitionGeometryTest.tar.gz";
  const std::string kExpectedOutputTopLevel = "PartitionGeometryTest";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  Arguments partitionGeometryArgs;
  Arguments importD3DArgs;

  const std::string partitionIdsArrayName = "PartitioningSchemeIds";
  const DataPath existingPSGeometryPath = {{"ExemplaryPSDataContainer"}};
  std::string exemplaryArrayName;

  SECTION("Test Basic Image Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 5, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_ImageGeomTestFilePath});
  }
  SECTION("Test Advanced Image Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 5, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    const FloatVec3 partitioningSchemeOrigin = {-10, 5, 2};
    const FloatVec3 lengthPerPartition = {5, 5, 5};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createAdvancedPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_ImageGeomTestFilePath});
  }
  SECTION("Test Bounding Box Image Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 5, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    const FloatVec3 lowerLeftCoord = {-10, 5, 2};
    const FloatVec3 upperRightCoord = {15, 30, 27};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBoundingBoxPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_ImageGeomTestFilePath});
  }
  SECTION("Test Existing Partitioning Scheme Image Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_ImageGeomTestFilePath});
  }
  SECTION("Test Basic Rect Grid Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 5, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_RectGridGeomTestFilePath});
  }
  SECTION("Test Advanced Rect Grid Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 5, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    const FloatVec3 partitioningSchemeOrigin = {0, 0, 0};
    const FloatVec3 lengthPerPartition = {6, 6, 6};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createAdvancedPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_RectGridGeomTestFilePath});
  }
  SECTION("Test Bounding Box Rect Grid Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 5, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    const FloatVec3 lowerLeftCoord = {0, 0, 0};
    const FloatVec3 upperRightCoord = {30, 30, 30};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBoundingBoxPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_RectGridGeomTestFilePath});
  }
  SECTION("Test Existing Partitioning Scheme Rect Grid Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "CellData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_RectGridGeomTestFilePath});
  }
  SECTION("Test Basic Triangle Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 4, 4};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TriangleGeomTestFilePath});
  }
  SECTION("Test Advanced Triangle Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 4, 4};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 lengthPerPartition = {0.398984, 0.49873, 0.247939};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createAdvancedPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TriangleGeomTestFilePath});
  }
  SECTION("Test Bounding Box Triangle Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 4, 4};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 upperRightCoord = {0.997463, 0.997462, 0.991746};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBoundingBoxPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TriangleGeomTestFilePath});
  }
  SECTION("Test Existing Partitioning Scheme Triangle Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TriangleGeomTestFilePath});
  }
  SECTION("Test Masked Triangle Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {5, 4, 4};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const DataPath maskPath = {{"DataContainer", "VertexData", "Mask"}};
    exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, maskPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TriangleGeomTestFilePath});
  }
  SECTION("Test Basic Edge Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {4, 4, 4};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_EdgeGeomTestFilePath});
  }
  SECTION("Test Advanced Edge Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {4, 4, 4};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 lengthPerPartition = {0.49873, 0.49873, 0.247939};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createAdvancedPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_EdgeGeomTestFilePath});
  }
  SECTION("Test Bounding Box Edge Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {4, 4, 4};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 upperRightCoord = {0.997462, 0.997462, 0.991746};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBoundingBoxPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_EdgeGeomTestFilePath});
  }
  SECTION("Test Existing Partitioning Scheme Edge Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_EdgeGeomTestFilePath});
  }
  SECTION("Test Masked Edge Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {4, 4, 4};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const DataPath maskPath = {{"DataContainer", "VertexData", "Mask"}};
    exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, maskPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_EdgeGeomTestFilePath});
  }
  SECTION("Test Basic Vertex Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {20, 10, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_VertexGeomTestFilePath});
  }
  SECTION("Test Advanced Vertex Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {20, 10, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 lengthPerPartition = {0.099746, 0.199492, 0.198351};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createAdvancedPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_VertexGeomTestFilePath});
  }
  SECTION("Test Bounding Box Vertex Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {20, 10, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 upperRightCoord = {0.997462, 0.997458, 0.991745};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBoundingBoxPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_VertexGeomTestFilePath});
  }
  SECTION("Test Existing Partitioning Scheme Vertex Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_VertexGeomTestFilePath});
  }
  SECTION("Test Masked Vertex Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {20, 10, 5};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const DataPath maskPath = {{"DataContainer", "VertexData", "Mask"}};
    exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, maskPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_VertexGeomTestFilePath});
  }
  SECTION("Test Basic Quad Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {10, 5, 3};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_QuadGeomTestFilePath});
  }
  SECTION("Test Advanced Quad Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {10, 5, 3};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 lengthPerPartition = {0.199492, 0.398984, 0.330585333333333};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createAdvancedPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_QuadGeomTestFilePath});
  }
  SECTION("Test Bounding Box Quad Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {10, 5, 3};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 upperRightCoord = {0.997462, 0.997462, 0.991746};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBoundingBoxPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_QuadGeomTestFilePath});
  }
  SECTION("Test Existing Partitioning Scheme Quad Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_QuadGeomTestFilePath});
  }
  SECTION("Test Masked Quad Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {10, 5, 3};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const DataPath maskPath = {{"DataContainer", "VertexData", "Mask"}};
    exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, maskPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_QuadGeomTestFilePath});
  }
  SECTION("Test Basic Tetrahedral Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {100, 45, 8};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TetrahedralGeomTestFilePath});
  }
  SECTION("Test Advanced Tetrahedral Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {100, 45, 8};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 lengthPerPartition = {0.0199492, 0.044331555555556, 0.12397};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createAdvancedPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TetrahedralGeomTestFilePath});
  }
  SECTION("Test Bounding Box Tetrahedral Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {100, 45, 8};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    const FloatVec3 upperRightCoord = {0.997458, 0.99746, 0.99175};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBoundingBoxPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TetrahedralGeomTestFilePath});
  }
  SECTION("Test Existing Partitioning Scheme Tetrahedral Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TetrahedralGeomTestFilePath});
  }
  SECTION("Test Masked Tetrahedral Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {100, 45, 8};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const DataPath maskPath = {{"DataContainer", "VertexData", "Mask"}};
    exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, maskPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_TetrahedralGeomTestFilePath});
  }
  SECTION("Test Basic Hexahedral Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {6, 7, 8};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_HexahedralGeomTestFilePath});
  }
  SECTION("Test Advanced Hexahedral Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {6, 7, 8};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 partitioningSchemeOrigin = {0.9999989867210388, 0.9999989867210388, 1.5499989986419678};
    const FloatVec3 lengthPerPartition = {1.105000376701355, 0.2857145667076111, 0.2500002384185791};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createAdvancedPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_HexahedralGeomTestFilePath});
  }
  SECTION("Test Bounding Box Hexahedral Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {6, 7, 8};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const FloatVec3 lowerLeftCoord = {0.9999989867210388, 0.9999989867210388, 1.5499989986419678};
    const FloatVec3 upperRightCoord = {7.630001068115234, 3.0000009536743164, 3.5500009059906006};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBoundingBoxPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_HexahedralGeomTestFilePath});
  }
  SECTION("Test Existing Partitioning Scheme Hexahedral Geometry")
  {
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createExistingPartitioningSchemeGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, existingPSGeometryPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_HexahedralGeomTestFilePath});
  }
  SECTION("Test Masked Hexahedral Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {6, 7, 8};
    const DataPath inputGeometryPath = {{"DataContainer"}};
    const DataPath attrMatrixPath = {{"DataContainer", "VertexData"}};
    const DataPath maskPath = {{"DataContainer", "VertexData", "Mask"}};
    exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, maskPath);
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_HexahedralGeomTestFilePath});
  }

  const ImportDREAM3DFilter importD3DFilter;
  DataStructure dataStructure;
  const PartitionGeometryFilter filter;

  // Preflight the filter and check result
  auto executeResult = importD3DFilter.execute(dataStructure, importD3DArgs);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  // Execute the filter and check the result
  executeResult = filter.execute(dataStructure, partitionGeometryArgs);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto attrMatrixPath = partitionGeometryArgs.value<DataPath>(PartitionGeometryFilter::k_InputGeometryCellAttributeMatrixPath_Key);

  const Int32Array& partitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(partitionIdsArrayName));
  const Int32Array& exemplaryPartitionIds = dataStructure.getDataRefAs<Int32Array>(attrMatrixPath.createChildPath(exemplaryArrayName));

  REQUIRE(partitionIds.getSize() == exemplaryPartitionIds.getSize());

  const AbstractDataStore<int32>& partitionIdsStore = partitionIds.getDataStoreRef();
  const AbstractDataStore<int32>& exemplaryPartitionIdsStore = exemplaryPartitionIds.getDataStoreRef();
  for(size_t i = 0; i < partitionIds.getSize(); i++)
  {
    const int32_t partitionId = partitionIdsStore[i];
    const int32_t exemplaryId = exemplaryPartitionIdsStore[i];
    REQUIRE(partitionId == exemplaryId);
  }
}

TEST_CASE("ComplexCore::PartitionGeometryFilter: Invalid filter execution")
{
  const std::string kDataInputArchive = "PartitionGeometryTest.tar.gz";
  const std::string kExpectedOutputTopLevel = "PartitionGeometryTest";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  Arguments partitionGeometryArgs;
  Arguments importD3DArgs;

  const std::string partitionIdsArrayName = "PartitioningSchemeIds";
  const DataPath existingPSGeometryPath = {{"ExemplaryPSDataContainer"}};
  int expectedErrorCode = -1;

  SECTION("Test Planal XY Node Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {3, 3, 3};
    const DataPath inputGeometryPath = {{"VertexDataContainer"}};
    const DataPath attrMatrixPath = {{"VertexDataContainer", "AttributeMatrix"}};
    expectedErrorCode = -3042;

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_PlanalXYNodeGeomTestFilePath});
  }
  SECTION("Test Planal XZ Node Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {3, 3, 3};
    const DataPath inputGeometryPath = {{"VertexDataContainer"}};
    const DataPath attrMatrixPath = {{"VertexDataContainer", "AttributeMatrix"}};
    expectedErrorCode = -3041;

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_PlanalXZNodeGeomTestFilePath});
  }
  SECTION("Test Planal YZ Node Geometry")
  {
    const IntVec3 numOfPartitionsPerAxis = {3, 3, 3};
    const DataPath inputGeometryPath = {{"VertexDataContainer"}};
    const DataPath attrMatrixPath = {{"VertexDataContainer", "AttributeMatrix"}};
    expectedErrorCode = -3040;

    partitionGeometryArgs = createBasicPartitionGeometryArguments(inputGeometryPath, attrMatrixPath, partitionIdsArrayName, numOfPartitionsPerAxis, {});
    importD3DArgs.insert(ImportDREAM3DFilter::k_ImportFileData, Dream3dImportParameter::ImportData{k_PlanalYZNodeGeomTestFilePath});
  }

  const ImportDREAM3DFilter importD3DFilter;
  DataStructure dataStructure;
  const PartitionGeometryFilter filter;

  auto executeResult = importD3DFilter.execute(dataStructure, importD3DArgs);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  executeResult = filter.execute(dataStructure, partitionGeometryArgs);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == expectedErrorCode);
}

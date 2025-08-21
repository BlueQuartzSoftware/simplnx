#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "SimplnxCore/Filters/CombineTransformationMatricesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const nx::core::ChoicesParameter::ValueType k_PrecomputedTransformationMatrixIdx = 1ULL;
const nx::core::ChoicesParameter::ValueType k_ManualTransformationMatrixIdx = 2ULL;
const nx::core::ChoicesParameter::ValueType k_RotationIdx = 3ULL;
const nx::core::ChoicesParameter::ValueType k_TranslationIdx = 4ULL;
const nx::core::ChoicesParameter::ValueType k_ScaleIdx = 5ULL;

const nx::core::ChoicesParameter::ValueType k_NearestNeighborInterpolationIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_LinearInterpolationIdx = 1ULL;
const nx::core::ChoicesParameter::ValueType k_NoInterpolationIdx = 2ULL;

const std::string k_StepByStepGeomName = "Step-By-Step Geometry";
const DataPath k_StepByStepGeomPath({k_StepByStepGeomName});
const std::string k_CombinedGeomName = "Combined Geometry";
const DataPath k_CombinedGeomPath({k_CombinedGeomName});
const std::string k_CombinedGeom2Name = "Combined Geometry 2";
const DataPath k_CombinedGeom2Path({k_CombinedGeom2Name});
const std::string k_CombinedGeom3Name = "Combined Geometry 3";
const DataPath k_CombinedGeom3Path({k_CombinedGeom3Name});
const std::string k_CombinedGeom4Name = "Combined Geometry 4";
const DataPath k_CombinedGeom4Path({k_CombinedGeom4Name});
const DataPath k_ExemplaryGeomPath({"Exemplary Geometry"});
const DataPath k_ExemplaryGeom2Path({"Exemplary Geometry 2"});
const DataPath k_ExemplaryGeom3Path({"Exemplary Geometry 3"});
const DataPath k_ExemplaryGeom4Path({"Exemplary Geometry 4"});
const std::string k_CellAttrMatrixName = "Cell Data";
const std::string k_CellArrayName = "Test Array";
const DataPath k_StepByStepCellAttrMatrixPath = k_StepByStepGeomPath.createChildPath(k_CellAttrMatrixName);
const DataPath k_StepByStepCellArrayPath = k_StepByStepCellAttrMatrixPath.createChildPath(k_CellArrayName);
const DataPath k_CombinedCellAttrMatrixPath = k_CombinedGeomPath.createChildPath(k_CellAttrMatrixName);
const DataPath k_CombinedCellArrayPath = k_CombinedCellAttrMatrixPath.createChildPath(k_CellArrayName);
const std::string k_ImportedTransformsGroupName = "Imported Transforms";
const std::string k_Scalex2TransformName = "Transform Scale x2";
const DataPath k_Scalex2TransformPath({k_ImportedTransformsGroupName, k_Scalex2TransformName});
const std::string k_ScaleReduceTransformName = "Transform Scale Reduce";
const DataPath k_ScaleReduceTransformPath({k_ImportedTransformsGroupName, k_ScaleReduceTransformName});
const std::string k_Translation1TransformName = "Transform Translation 1";
const DataPath k_Translation1TransformPath({k_ImportedTransformsGroupName, k_Translation1TransformName});
const std::string k_Translation2TransformName = "Transform Translation 2";
const DataPath k_Translation2TransformPath({k_ImportedTransformsGroupName, k_Translation2TransformName});
const std::string k_X45TransformName = "Transform X-45";
const DataPath k_X45TransformPath({k_ImportedTransformsGroupName, k_X45TransformName});
const std::string k_Y45TransformName = "Transform Y-45";
const DataPath k_Y45TransformPath({k_ImportedTransformsGroupName, k_Y45TransformName});
const std::string k_Z45TransformName = "Transform Z-45";
const DataPath k_Z45TransformPath({k_ImportedTransformsGroupName, k_Z45TransformName});
const std::string k_FreestyleTransformName = "Transform Freestyle";
const DataPath k_FreestyleTransformPath({k_ImportedTransformsGroupName, k_FreestyleTransformName});
const std::string k_CombinedTransformName = "Transform Combined";
const DataPath k_CombinedTransformPath({k_CombinedTransformName});
const std::string k_Combined2TransformName = "Transform Combined 2";
const DataPath k_Combined2TransformPath({k_Combined2TransformName});
const std::string k_Combined3TransformName = "Transform Combined 3";
const DataPath k_Combined3TransformPath({k_Combined3TransformName});
const std::string k_Combined4TransformName = "Transform Combined 4";
const DataPath k_Combined4TransformPath({k_Combined4TransformName});

void CreateTestImageGeometry(DataStructure& dataStructure, const std::string& geomName, const Vec3<usize>& dims, const Vec3<float32>& origin, const Vec3<float32>& spacing,
                             const std::vector<usize>& cDims)
{
  ImageGeom* geom = ImageGeom::Create(dataStructure, geomName);
  geom->setDimensions(dims);
  geom->setSpacing(spacing);
  geom->setOrigin(origin);

  auto dimsVec = dims.toContainer<std::vector<usize>>();
  auto amDims = std::vector<usize>(dimsVec.rbegin(), dimsVec.rend());
  AttributeMatrix* am = AttributeMatrix::Create(dataStructure, k_CellAttrMatrixName, amDims, geom->getId());
  geom->setCellData(*am);
  UInt32Array* testArray = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_CellArrayName, amDims, cDims, am->getId());

  auto numComps = std::accumulate(cDims.begin(), cDims.end(), 1, std::multiplies<>());
  std::generate(testArray->begin(), testArray->end(), [n = 0, numComps]() mutable { return 1 + (n++ / numComps); });
}

void CreateTestVertexGeometry(DataStructure& dataStructure, const std::string& geomName)
{
  VertexGeom* geom = VertexGeom::Create(dataStructure, geomName);
  Float32Array* vertices = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, VertexGeom::k_SharedVertexListName, {16}, {3}, geom->getId());
  std::iota(vertices->begin(), vertices->end(), 0.0f);
  geom->setVertices(*vertices);
  AttributeMatrix* am = AttributeMatrix::Create(dataStructure, VertexGeom::k_VertexAttributeMatrixName, {16}, geom->getId());
  geom->setVertexAttributeMatrix(*am);
  Int64Array* testArray = Int64Array::CreateWithStore<Int64DataStore>(dataStructure, k_CellArrayName, {16}, {2}, am->getId());
  std::iota(testArray->begin(), testArray->end(), 0.0f);
}

void CompareGeometries(const DataStructure& dataStructure, const DataPath& exemplaryGeomPath, const DataPath& resultGeomPath)
{
  auto exemplaryGeom = dataStructure.getDataAs<ImageGeom>(exemplaryGeomPath);
  auto resultGeom = dataStructure.getDataAs<ImageGeom>(resultGeomPath);
  UnitTest::CompareImageGeometry(exemplaryGeom, resultGeom, UnitTest::EPSILON);

  REQUIRE_NOTHROW(exemplaryGeom->getCellDataRef());
  REQUIRE_NOTHROW(resultGeom->getCellDataRef());
  auto exemplaryAM = exemplaryGeom->getCellDataRef();
  auto resultAM = resultGeom->getCellDataRef();
  REQUIRE(exemplaryAM.getShape() == resultAM.getShape());

  auto exemplaryCellArrayPath = exemplaryGeomPath.createChildPath(k_CellAttrMatrixName).createChildPath(k_CellArrayName);
  auto resultCellArrayPath = resultGeomPath.createChildPath(k_CellAttrMatrixName).createChildPath(k_CellArrayName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(exemplaryCellArrayPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(resultCellArrayPath));
  auto exemplaryCellArray = dataStructure.getDataRefAs<UInt32Array>(exemplaryCellArrayPath);
  auto resultCellArray = dataStructure.getDataRefAs<UInt32Array>(resultCellArrayPath);
  UnitTest::CompareDataArrays<uint32>(exemplaryCellArray, resultCellArray);
}
} // namespace

TEST_CASE("SimplnxCore::CombineTransformationMatricesFilter: Image Geometries - Scaling & Translating", "[SimplnxCore][CombineTransformationMatricesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "combine_transformation_matrices_test.tar.gz",
                                                              "combine_transformation_matrices_test.dream3d");

  auto exemplarFilePath = fs::path(fmt::format("{}/combine_transformation_matrices_test.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  CreateTestImageGeometry(dataStructure, k_StepByStepGeomName, {10, 15, 20}, {20, -10, 5}, {1, 2, 1}, {3});
  CreateTestImageGeometry(dataStructure, k_CombinedGeomName, {10, 15, 20}, {20, -10, 5}, {1, 2, 1}, {3});

  std::vector<DataPath> testTransformPaths = {k_Scalex2TransformPath, k_ScaleReduceTransformPath, k_Translation1TransformPath, k_Translation2TransformPath};
  for(const auto& testTransformPath : testTransformPaths)
  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_StepByStepGeomPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(k_StepByStepCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(testTransformPath));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Combine the transforms
  {
    CombineTransformationMatricesFilter filter;
    Arguments args;

    args.insertOrAssign(CombineTransformationMatricesFilter::k_InputArrays_Key, std::make_any<nx::core::MultiArraySelectionParameter::ValueType>(testTransformPaths));
    args.insertOrAssign(CombineTransformationMatricesFilter::k_OutputArray_Key, std::make_any<nx::core::ArrayCreationParameter::ValueType>(k_CombinedTransformPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Apply combined transform
  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_CombinedGeomPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(k_StepByStepCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(k_CombinedTransformPath));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  auto stepByStepGeom = dataStructure.getDataAs<ImageGeom>(k_StepByStepGeomPath);
  auto combinedGeom = dataStructure.getDataAs<ImageGeom>(k_CombinedGeomPath);
  UnitTest::CompareImageGeometry(stepByStepGeom, combinedGeom);

  REQUIRE_NOTHROW(stepByStepGeom->getCellDataRef());
  REQUIRE_NOTHROW(combinedGeom->getCellDataRef());
  auto stepByStepAM = stepByStepGeom->getCellDataRef();
  auto combinedAM = combinedGeom->getCellDataRef();
  REQUIRE(stepByStepAM.getShape() == combinedAM.getShape());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(k_StepByStepCellArrayPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(k_CombinedCellArrayPath));
  auto stepByStepCellArray = dataStructure.getDataRefAs<UInt32Array>(k_StepByStepCellArrayPath);
  auto combinedCellArray = dataStructure.getDataRefAs<UInt32Array>(k_CombinedCellArrayPath);
  UnitTest::CompareDataArrays<uint32>(stepByStepCellArray, combinedCellArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CombineTransformationMatricesFilter: Image Geometries - Rotating & Freestyle", "[SimplnxCore][CombineTransformationMatricesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "combine_transformation_matrices_test.tar.gz",
                                                              "combine_transformation_matrices_test.dream3d");

  auto exemplarFilePath = fs::path(fmt::format("{}/combine_transformation_matrices_test.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  CreateTestImageGeometry(dataStructure, k_CombinedGeomName, {10, 15, 20}, {20, -10, 5}, {1, 2, 1}, {3});
  CreateTestImageGeometry(dataStructure, k_CombinedGeom2Name, {10, 15, 20}, {20, -10, 5}, {1, 2, 1}, {3});
  CreateTestImageGeometry(dataStructure, k_CombinedGeom3Name, {10, 15, 20}, {20, -10, 5}, {1, 2, 1}, {3});
  CreateTestImageGeometry(dataStructure, k_CombinedGeom4Name, {10, 15, 20}, {20, -10, 5}, {1, 2, 1}, {3});

  // Create Combined Transform 1 and Apply
  std::vector<DataPath> testTransformPaths = {k_Z45TransformPath, k_Y45TransformPath, k_X45TransformPath};
  {
    CombineTransformationMatricesFilter filter;
    Arguments args;

    args.insertOrAssign(CombineTransformationMatricesFilter::k_InputArrays_Key, std::make_any<nx::core::MultiArraySelectionParameter::ValueType>(testTransformPaths));
    args.insertOrAssign(CombineTransformationMatricesFilter::k_OutputArray_Key, std::make_any<nx::core::ArrayCreationParameter::ValueType>(k_CombinedTransformPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_CombinedGeomPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(k_CombinedCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(k_CombinedTransformPath));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Create Combined Transform 2
  testTransformPaths = {k_Scalex2TransformPath, k_Y45TransformPath};
  {
    CombineTransformationMatricesFilter filter;
    Arguments args;

    args.insertOrAssign(CombineTransformationMatricesFilter::k_InputArrays_Key, std::make_any<nx::core::MultiArraySelectionParameter::ValueType>(testTransformPaths));
    args.insertOrAssign(CombineTransformationMatricesFilter::k_OutputArray_Key, std::make_any<nx::core::ArrayCreationParameter::ValueType>(k_Combined2TransformPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_CombinedGeom2Path));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(k_CombinedGeom2Path.createChildPath(k_CellAttrMatrixName)));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(k_Combined2TransformPath));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Create Combined Transform 3
  testTransformPaths = {k_Translation1TransformPath, k_X45TransformPath};
  {
    CombineTransformationMatricesFilter filter;
    Arguments args;

    args.insertOrAssign(CombineTransformationMatricesFilter::k_InputArrays_Key, std::make_any<nx::core::MultiArraySelectionParameter::ValueType>(testTransformPaths));
    args.insertOrAssign(CombineTransformationMatricesFilter::k_OutputArray_Key, std::make_any<nx::core::ArrayCreationParameter::ValueType>(k_Combined3TransformPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_CombinedGeom3Path));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(k_CombinedGeom3Path.createChildPath(k_CellAttrMatrixName)));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(k_Combined3TransformPath));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Create Combined Transform 4
  testTransformPaths = {k_Translation2TransformPath, k_Scalex2TransformPath, k_X45TransformPath};
  {
    CombineTransformationMatricesFilter filter;
    Arguments args;

    args.insertOrAssign(CombineTransformationMatricesFilter::k_InputArrays_Key, std::make_any<nx::core::MultiArraySelectionParameter::ValueType>(testTransformPaths));
    args.insertOrAssign(CombineTransformationMatricesFilter::k_OutputArray_Key, std::make_any<nx::core::ArrayCreationParameter::ValueType>(k_Combined4TransformPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_CombinedGeom4Path));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(k_CombinedGeom4Path.createChildPath(k_CellAttrMatrixName)));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(k_Combined4TransformPath));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  CompareGeometries(dataStructure, k_ExemplaryGeomPath, k_CombinedGeomPath);
  CompareGeometries(dataStructure, k_ExemplaryGeom2Path, k_CombinedGeom2Path);
  CompareGeometries(dataStructure, k_ExemplaryGeom3Path, k_CombinedGeom3Path);
  CompareGeometries(dataStructure, k_ExemplaryGeom4Path, k_CombinedGeom4Path);

  auto exemplaryGeom = dataStructure.getDataAs<ImageGeom>(k_ExemplaryGeomPath);
  auto combinedGeom = dataStructure.getDataAs<ImageGeom>(k_CombinedGeomPath);
  UnitTest::CompareImageGeometry(exemplaryGeom, combinedGeom);

  REQUIRE_NOTHROW(exemplaryGeom->getCellDataRef());
  REQUIRE_NOTHROW(combinedGeom->getCellDataRef());
  auto exemplaryAM = exemplaryGeom->getCellDataRef();
  auto combinedAM = combinedGeom->getCellDataRef();
  REQUIRE(exemplaryAM.getShape() == combinedAM.getShape());

  auto exemplaryCellArrayPath = k_ExemplaryGeomPath.createChildPath(k_CellAttrMatrixName).createChildPath(k_CellArrayName);
  auto combinedCellArrayPath = k_CombinedGeomPath.createChildPath(k_CellAttrMatrixName).createChildPath(k_CellArrayName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(exemplaryCellArrayPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(combinedCellArrayPath));
  auto exemplaryCellArray = dataStructure.getDataRefAs<UInt32Array>(exemplaryCellArrayPath);
  auto combinedCellArray = dataStructure.getDataRefAs<UInt32Array>(combinedCellArrayPath);
  UnitTest::CompareDataArrays<uint32>(exemplaryCellArray, combinedCellArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CombineTransformationMatricesFilter: Vertex Geometries", "[SimplnxCore][CombineTransformationMatricesFilter]")
{
  // This test case only tests vertex geometries because the transformations only affect the vertices, NOT the topology.

  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "combine_transformation_matrices_test.tar.gz",
                                                              "combine_transformation_matrices_test.dream3d");

  auto exemplarFilePath = fs::path(fmt::format("{}/combine_transformation_matrices_test.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  CreateTestVertexGeometry(dataStructure, k_StepByStepGeomName);
  CreateTestVertexGeometry(dataStructure, k_CombinedGeomName);

  std::vector<DataPath> testTransformPaths = {k_X45TransformPath,          k_Y45TransformPath,          k_Z45TransformPath,         k_Scalex2TransformPath,
                                              k_Translation1TransformPath, k_Translation2TransformPath, k_ScaleReduceTransformPath, k_FreestyleTransformPath};
  for(const auto& testTransformPath : testTransformPaths)
  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_StepByStepGeomPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_NoInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(testTransformPath));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Combine the transforms
  {
    CombineTransformationMatricesFilter filter;
    Arguments args;

    args.insertOrAssign(CombineTransformationMatricesFilter::k_InputArrays_Key, std::make_any<nx::core::MultiArraySelectionParameter::ValueType>(testTransformPaths));
    args.insertOrAssign(CombineTransformationMatricesFilter::k_OutputArray_Key, std::make_any<nx::core::ArrayCreationParameter::ValueType>(k_CombinedTransformPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Apply combined transform
  {
    ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_CombinedGeomPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(k_NoInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(k_CombinedTransformPath));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  auto stepByStepGeom = dataStructure.getDataAs<VertexGeom>(k_StepByStepGeomPath);
  auto combinedGeom = dataStructure.getDataAs<VertexGeom>(k_CombinedGeomPath);

  REQUIRE_NOTHROW(stepByStepGeom->getVertexAttributeMatrixRef());
  REQUIRE_NOTHROW(combinedGeom->getVertexAttributeMatrixRef());
  auto stepByStepAM = stepByStepGeom->getVertexAttributeMatrixRef();
  auto combinedAM = combinedGeom->getVertexAttributeMatrixRef();
  REQUIRE(stepByStepAM.getShape() == combinedAM.getShape());

  REQUIRE_NOTHROW(stepByStepGeom->getVerticesRef());
  REQUIRE_NOTHROW(combinedGeom->getVerticesRef());
  auto stepByStepVertices = stepByStepGeom->getVerticesRef();
  auto combinedVertices = combinedGeom->getVerticesRef();
  UnitTest::CompareDataArrays<float32>(stepByStepVertices, combinedVertices);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(k_StepByStepGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(k_CellArrayName)));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(k_CombinedGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(k_CellArrayName)));
  auto stepByStepCellArray = dataStructure.getDataRefAs<Int64Array>(k_StepByStepGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(k_CellArrayName));
  auto combinedCellArray = dataStructure.getDataRefAs<Int64Array>(k_CombinedGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(k_CellArrayName));
  UnitTest::CompareDataArrays<int64>(stepByStepCellArray, combinedCellArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

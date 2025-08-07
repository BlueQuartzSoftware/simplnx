#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "SimplnxCore/Filters/CombineTransformationMatricesFilter.hpp"
#include "SimplnxCore/Filters/ConvertDataFilter.hpp"
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
const std::string k_CellAttrMatrixName = "Cell Data";
const std::string k_CellArrayName = "Test Array";
const DataPath k_StepByStepCellAttrMatrixPath = k_StepByStepGeomPath.createChildPath(k_CellAttrMatrixName);
const DataPath k_StepByStepCellArrayPath = k_StepByStepCellAttrMatrixPath.createChildPath(k_CellArrayName);
const DataPath k_CombinedCellAttrMatrixPath = k_CombinedGeomPath.createChildPath(k_CellAttrMatrixName);
const DataPath k_CombinedCellArrayPath = k_CombinedCellAttrMatrixPath.createChildPath(k_CellArrayName);
const std::string k_Rotation90TransformName = "Rotation90";
const DataPath k_Rotation90TransformPath({k_Rotation90TransformName});
const std::string k_Rotation45TransformName = "Rotation45";
const DataPath k_Rotation45TransformPath({k_Rotation45TransformName});
const std::string k_Scale1TransformName = "Scale1";
const DataPath k_Scale1TransformPath({k_Scale1TransformName});
const std::string k_Scale2TransformName = "Scale2";
const DataPath k_Scale2TransformPath({k_Scale2TransformName});
const std::string k_Translation1TransformName = "Translation1";
const DataPath k_Translation1TransformPath({k_Translation1TransformName});
const std::string k_Translation2TransformName = "Translation2";
const DataPath k_Translation2TransformPath({k_Translation2TransformName});
const std::string k_FreestyleTransformName = "Freestyle";
const DataPath k_FreestyleTransformPath({k_FreestyleTransformName});
const std::string k_CombinedTransformName = "Combined Transform";
const DataPath k_CombinedTransformPath({k_CombinedTransformName});

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
  Float64Array* testArray = Float64Array::CreateWithStore<Float64DataStore>(dataStructure, k_CellArrayName, amDims, cDims, am->getId());
  std::iota(testArray->begin(), testArray->end(), 0.0f);
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

std::vector<DataPath> CreateTestTransforms(DataStructure& dataStructure)
{
  Float32Array::Create(dataStructure, k_Rotation90TransformName,
                       std::make_shared<Float32DataStore>(
                           Float32DataStore(std::unique_ptr<float32[]>{new float32[16]{0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0}}, {16}, {1})));
  Float32Array::Create(
      dataStructure, k_Rotation45TransformName,
      std::make_shared<Float32DataStore>(Float32DataStore(
          std::unique_ptr<float32[]>{new float32[16]{0.707107f, -0.707107f, 0.0f, 0.0f, 0.707107f, 0.707107f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0}}, {16}, {1})));
  Float32Array::Create(dataStructure, k_Scale1TransformName,
                       std::make_shared<Float32DataStore>(
                           Float32DataStore(std::unique_ptr<float32[]>{new float32[16]{2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0}}, {16}, {1})));
  Float32Array::Create(dataStructure, k_Scale2TransformName,
                       std::make_shared<Float32DataStore>(
                           Float32DataStore(std::unique_ptr<float32[]>{new float32[16]{0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0}}, {16}, {1})));
  Float32Array::Create(dataStructure, k_Translation1TransformName,
                       std::make_shared<Float32DataStore>(Float32DataStore(
                           std::unique_ptr<float32[]>{new float32[16]{1.0f, 0.0f, 0.0f, -12.0f, 0.0f, 1.0f, 0.0f, 57.0f, 0.0f, 0.0f, 1.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0}}, {16}, {1})));
  Float32Array::Create(dataStructure, k_Translation2TransformName,
                       std::make_shared<Float32DataStore>(Float32DataStore(
                           std::unique_ptr<float32[]>{new float32[16]{1.0f, 0.0f, 0.0f, 42.0f, 0.0f, 1.0f, 0.0f, -67.0f, 0.0f, 0.0f, 1.0f, -89.0f, 0.0f, 0.0f, 0.0f, 1.0}}, {16}, {1})));

  // Rotate 32 degrees in Y direction, rotate 12 degrees in X direction, scale by [0.5, 0.3, 2.0], translate [-12.0, 50.0, 2.0]
  Float32Array::Create(dataStructure, k_FreestyleTransformName,
                       std::make_shared<Float32DataStore>(Float32DataStore(std::unique_ptr<float32[]>{new float32[16]{0.42402405f, 0.0f, 0.26495963f, -12.0f, 0.033052925f, 0.2934443f, -0.05289574f,
                                                                                                                      50.0f, -1.0366786f, 0.4158234f, 1.6590325f, 2.0f, 0.0f, 0.0f, 0.0f, 1.0}},
                                                                           {16}, {1})));

  return {k_Rotation90TransformPath, k_Rotation45TransformPath, k_Scale1TransformPath, k_Scale2TransformPath, k_Translation1TransformPath, k_Translation2TransformPath, k_FreestyleTransformPath};
}
} // namespace

TEST_CASE("SimplnxCore::CombineTransformationMatricesFilter: Valid Filter Execution - Vertex Geometry", "[SimplnxCore][CombineTransformationMatricesFilter]")
{
  DataStructure dataStructure;
  CreateTestVertexGeometry(dataStructure, k_StepByStepGeomName);
  CreateTestVertexGeometry(dataStructure, k_CombinedGeomName);
  std::vector<DataPath> testTransformPaths = CreateTestTransforms(dataStructure);

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
    REQUIRE(executeResult.result.valid());
  }

  // Combine the transforms
  {
    CombineTransformationMatricesFilter filter;
    Arguments args;

    args.insertOrAssign(CombineTransformationMatricesFilter::k_InputArrays_Key, std::make_any<nx::core::MultiArraySelectionParameter::ValueType>(testTransformPaths));
    args.insertOrAssign(CombineTransformationMatricesFilter::k_OutputArray_Key, std::make_any<nx::core::ArrayCreationParameter::ValueType>(k_CombinedTransformPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
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
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::WriteTestDataStructure(dataStructure, "/tmp/test.dream3d");

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
}

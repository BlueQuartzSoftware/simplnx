#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "SimplnxCore/Filters/CombineTransformationMatricesFilter.hpp"
#include "SimplnxCore/Filters/ConvertDataFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

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
  UInt32Array* testArray = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_CellArrayName, amDims, cDims, am->getId());
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

Float32Array* ConvertToTransformArray(DataStructure& dataStructure, const std::string& arrayName, const ImageRotationUtilities::Matrix4fR& transformMatrix)
{
  auto transformMatrixArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, arrayName, {4, 4}, {1});
  std::copy(transformMatrix.data(), transformMatrix.data() + transformMatrix.size(), transformMatrixArray->begin());
  return transformMatrixArray;
}

void CreateScaleTransformationMatrix(DataStructure& dataStructure, const std::string& arrayName, const VectorFloat32Parameter::ValueType& pScaleValue)
{
  auto scaleMatrix = ImageRotationUtilities::GenerateScaleTransformationMatrix(pScaleValue);
  ConvertToTransformArray(dataStructure, arrayName, scaleMatrix);
}

void CreateRotationTransformationMatrix(DataStructure& dataStructure, const std::string& arrayName, const VectorFloat32Parameter::ValueType& pRotationValue)
{
  auto rotationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(pRotationValue);
  ConvertToTransformArray(dataStructure, arrayName, rotationMatrix);
}

void CreateTranslationTransformationMatrix(DataStructure& dataStructure, const std::string& arrayName, const VectorFloat32Parameter::ValueType& pTranslationValue)
{
  auto translationMatrix = ImageRotationUtilities::GenerateTranslationTransformationMatrix(pTranslationValue);
  ConvertToTransformArray(dataStructure, arrayName, translationMatrix);
}

void CreateManualTransformationMatrix(DataStructure& dataStructure, const std::string& arrayName, const VectorFloat32Parameter::ValueType& pManualValue)
{
  auto transformMatrixArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, arrayName, {4, 4}, {1});
  std::copy(pManualValue.begin(), pManualValue.end(), transformMatrixArray->begin());
}
} // namespace

TEST_CASE("SimplnxCore::CombineTransformationMatricesFilter: Image Geometries - Scaling & Translating", "[SimplnxCore][CombineTransformationMatricesFilter]")
{
  DataStructure dataStructure;
  CreateTestImageGeometry(dataStructure, k_StepByStepGeomName, {10, 15, 20}, {20, -10, 5}, {1, 2, 1}, {3});
  CreateTestImageGeometry(dataStructure, k_CombinedGeomName, {10, 15, 20}, {20, -10, 5}, {1, 2, 1}, {3});

  CreateScaleTransformationMatrix(dataStructure, k_Scale1TransformName, {2, 3, 4});
  CreateScaleTransformationMatrix(dataStructure, k_Scale2TransformName, {0.5, 0.3, 0.2});
  CreateTranslationTransformationMatrix(dataStructure, k_Translation1TransformName, {-12, 57, 32});
  CreateTranslationTransformationMatrix(dataStructure, k_Translation2TransformName, {42, -67, -89});

  std::vector<DataPath> testTransformPaths = {k_Scale1TransformPath, k_Scale2TransformPath, k_Translation1TransformPath, k_Translation2TransformPath};
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
}

TEST_CASE("SimplnxCore::CombineTransformationMatricesFilter: Image Geometries - Rotating & Freestyle", "[SimplnxCore][CombineTransformationMatricesFilter]")
{
}

TEST_CASE("SimplnxCore::CombineTransformationMatricesFilter: Vertex Geometries", "[SimplnxCore][CombineTransformationMatricesFilter]")
{
  DataStructure dataStructure;
  CreateTestVertexGeometry(dataStructure, k_StepByStepGeomName);
  CreateTestVertexGeometry(dataStructure, k_CombinedGeomName);

  CreateRotationTransformationMatrix(dataStructure, k_Rotation90TransformName, {0, 0, 1, 90});
  CreateRotationTransformationMatrix(dataStructure, k_Rotation45TransformName, {0, 0, 1, 45});
  CreateScaleTransformationMatrix(dataStructure, k_Scale1TransformName, {2, 3, 4});
  CreateScaleTransformationMatrix(dataStructure, k_Scale2TransformName, {0.5, 0.3, 0.2});
  CreateTranslationTransformationMatrix(dataStructure, k_Translation1TransformName, {-12, 57, 32});
  CreateTranslationTransformationMatrix(dataStructure, k_Translation2TransformName, {42, -67, -89});
  CreateManualTransformationMatrix(dataStructure, k_FreestyleTransformName,
                                   {0.42402405f, 0.0f, 0.26495963f, -12.0f, 0.033052925f, 0.2934443f, -0.05289574f, 50.0f, -1.0366786f, 0.4158234f, 1.6590325f, 2.0f, 0.0f, 0.0f, 0.0f, 1.0});

  std::vector<DataPath> testTransformPaths = {k_Rotation90TransformPath,   k_Rotation45TransformPath,   k_Scale1TransformPath,   k_Scale2TransformPath,
                                              k_Translation1TransformPath, k_Translation2TransformPath, k_FreestyleTransformPath};
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
}

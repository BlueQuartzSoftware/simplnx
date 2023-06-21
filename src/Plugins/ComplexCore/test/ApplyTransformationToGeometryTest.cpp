#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ApplyTransformationToGeometryFilter.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

namespace
{

} // namespace

namespace apply_transformation_to_geometry
{
const complex::ChoicesParameter::ValueType k_NoTransformIdx = 0ULL;
const complex::ChoicesParameter::ValueType k_PrecomputedTransformationMatrixIdx = 1ULL;
const complex::ChoicesParameter::ValueType k_ManualTransformationMatrixIdx = 2ULL;
const complex::ChoicesParameter::ValueType k_RotationIdx = 3ULL;
const complex::ChoicesParameter::ValueType k_TranslationIdx = 4ULL;
const complex::ChoicesParameter::ValueType k_ScaleIdx = 5ULL;

const complex::ChoicesParameter::ValueType k_NearestNeighborInterpolationIdx = 0ULL;
const complex::ChoicesParameter::ValueType k_LinearInterpolationIdx = 1ULL;

const std::string k_InputGeometryName("InputData");
const DataPath k_InputCellAttrMatrixPath(DataPath({k_InputGeometryName, "VertexData"}));
const std::string k_RotationGeometryName("6_6_Rotation");
const std::string k_ScaleGeometryName("6_6_Scale");
const std::string k_TranslationGeometryName("6_6_Translation");
const std::string k_ManualGeometryName("6_6_Manual");
const std::string k_PrecomputedGeometryName("6_6_Precomputed");

const std::string k_InputGeometryName66("InputData");
const std::string k_RotationGeometryName66("6_6_Rotation");
const std::string k_ScaleGeometryName66("6_6_Scale");
const std::string k_TranslationGeometryName66("6_6_Translation");
const std::string k_ManualGeometryName66("6_6_Manual");
const std::string k_PrecomputedGeometryName66("6_6_Precomputed");

const std::string k_SharedVertexListName("SharedVertexList");

} // namespace apply_transformation_to_geometry

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Translation_Node", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({100.0F, 50.0F, -100.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_translation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_TranslationGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<complex::HDF5::FileWriter> result = complex::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_translation.dream3d", unit_test::k_BinaryTestOutputDir));
  complex::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  COMPLEX_RESULT_REQUIRE_VALID(resultH5);
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Rotation_Node", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F, 45.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_rotation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_RotationGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<complex::HDF5::FileWriter> result = complex::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_rotation.dream3d", unit_test::k_BinaryTestOutputDir));
  complex::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  COMPLEX_RESULT_REQUIRE_VALID(resultH5);
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Scale_Node", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({0.5F, 1.5F, 10.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_scale.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ScaleGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<complex::HDF5::FileWriter> result = complex::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_scale.dream3d", unit_test::k_BinaryTestOutputDir));
  complex::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  COMPLEX_RESULT_REQUIRE_VALID(resultH5);
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Manual_Node", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
    // This should reflect the geometry across the x-axis.
    const DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}};
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<complex::DynamicTableParameter::ValueType>(dynamicTable));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ManualGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<complex::HDF5::FileWriter> result = complex::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_manual.dream3d", unit_test::k_BinaryTestOutputDir));
  complex::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  COMPLEX_RESULT_REQUIRE_VALID(resultH5);
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Precomputed_Node", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
    const DataPath precomputedPath({apply_transformation_to_geometry::k_InputGeometryName, "Precomputed AM", "TransformationMatrix"});
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(precomputedPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_PrecomputedGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<complex::HDF5::FileWriter> result = complex::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_precomputed.dream3d", unit_test::k_BinaryTestOutputDir));
  complex::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  COMPLEX_RESULT_REQUIRE_VALID(resultH5);
}

/*******************************************************************************
 * @brief This section is for Image Geometry with Linear Interpolation
 ******************************************************************************/
TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Translation_Image_Linear", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({100.0F, 50.0F, -100.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_translation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_TranslationGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Rotation_Image_Linear", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F, 45.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_rotation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_RotationGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Scale_Image_Linear", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({0.5F, 1.5F, 10.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_scale.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ScaleGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Manual_Image_Linear", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath)); // This should reflect the geometry across the x-axis.
    const DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}};
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<complex::DynamicTableParameter::ValueType>(dynamicTable));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ManualGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Precomputed_Image_Linear", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    const DataPath precomputedPath({apply_transformation_to_geometry::k_InputGeometryName, "Precomputed AM", "TransformationMatrix"});
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(precomputedPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_PrecomputedGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

/*******************************************************************************
 * @brief This section is for Image Geometry with Nearest Neighbor Interpolation
 ******************************************************************************/
TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Translation_Image_NN", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({100.0F, 50.0F, -100.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_translation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_TranslationGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Rotation_Image_NN", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F, 45.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_rotation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_RotationGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Scale_Image_NN", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<complex::VectorFloat32Parameter::ValueType>({0.5F, 1.5F, 10.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_scale.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ScaleGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Manual_Image_NN", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath)); // This should reflect the geometry across the x-axis.
    const DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}};
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<complex::DynamicTableParameter::ValueType>(dynamicTable));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ManualGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

TEST_CASE("ComplexCore::ApplyTransformationToGeometryFilter:Precomputed_Image_NN", "[ComplexCore][ApplyTransformationToGeometryFilter]")
{
  //  {
  //    DataStructure ds;
  //    ImageGeom* imageGeom = ImageGeom::Create(ds, "IG");
  //    imageGeom->setDimensions({3, 4, 5});
  //    imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
  //    imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
  //
  //    Point3Df coordsOld = {3.0F, 3.0F, 4.1667F};
  //    SizeVec3 oldGeomIndices = {0, 0, 0};
  //    auto result = imageGeom->computeCellIndex(coordsOld, oldGeomIndices);
  //    std::cout << oldGeomIndices[0] << ", " << oldGeomIndices[1] << ", " << oldGeomIndices[2] << std::endl;
  //  }

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<complex::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    const DataPath precomputedPath({apply_transformation_to_geometry::k_InputGeometryName, "Precomputed AM", "TransformationMatrix"});
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(precomputedPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_PrecomputedGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

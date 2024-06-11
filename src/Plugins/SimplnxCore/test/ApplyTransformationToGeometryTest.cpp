#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <xtensor/xio.hpp>

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{

} // namespace

namespace apply_transformation_to_geometry
{
const nx::core::ChoicesParameter::ValueType k_PrecomputedTransformationMatrixIdx = 1ULL;
const nx::core::ChoicesParameter::ValueType k_ManualTransformationMatrixIdx = 2ULL;
const nx::core::ChoicesParameter::ValueType k_RotationIdx = 3ULL;
const nx::core::ChoicesParameter::ValueType k_TranslationIdx = 4ULL;
const nx::core::ChoicesParameter::ValueType k_ScaleIdx = 5ULL;

const nx::core::ChoicesParameter::ValueType k_NearestNeighborInterpolationIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_LinearInterpolationIdx = 1ULL;

const std::string k_InputGeometryName("InputData");
const DataPath k_InputCellAttrMatrixPath(DataPath({k_InputGeometryName, "VertexData"}));
const std::string k_RotationGeometryName("6_6_Rotation");
const std::string k_ScaleGeometryName("6_6_Scale");
const std::string k_TranslationGeometryName("6_6_Translation");
const std::string k_ManualGeometryName("6_6_Manual");
const std::string k_PrecomputedGeometryName("6_6_Precomputed");

const std::string k_RotationGeometryName66("6_6_Rotation");
const std::string k_ScaleGeometryName66("6_6_Scale");
const std::string k_TranslationGeometryName66("6_6_Translation");
const std::string k_ManualGeometryName66("6_6_Manual");
const std::string k_PrecomputedGeometryName66("6_6_Precomputed");

const std::string k_SharedVertexListName("SharedVertexList");

const int32 k_CellAttrMatrixUnusedWarning = -5555;

} // namespace apply_transformation_to_geometry

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Translation_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({100.0F, 50.0F, -100.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_translation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_TranslationGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_translation.dream3d", unit_test::k_BinaryTestOutputDir));
  nx::core::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Rotation_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F, 45.0F}));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_rotation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_RotationGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_rotation.dream3d", unit_test::k_BinaryTestOutputDir));
  nx::core::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({0.5F, 1.5F, 10.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_scale.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ScaleGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_scale.dream3d", unit_test::k_BinaryTestOutputDir));
  nx::core::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Manual_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
    // This should reflect the geometry across the x-axis.
    const DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}};
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<nx::core::DynamicTableParameter::ValueType>(dynamicTable));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ManualGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_manual.dream3d", unit_test::k_BinaryTestOutputDir));
  nx::core::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Precomputed_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
    const DataPath precomputedPath({apply_transformation_to_geometry::k_InputGeometryName, "Precomputed AM", "TransformationMatrix"});
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(precomputedPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    REQUIRE(preflightResult.outputActions.warnings()[0].code == apply_transformation_to_geometry::k_CellAttrMatrixUnusedWarning);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_PrecomputedGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/ApplyTransformationToGeometryFilter_precomputed.dream3d", unit_test::k_BinaryTestOutputDir));
  nx::core::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
}

/*******************************************************************************
 * @brief This section is for Image Geometry with Linear Interpolation
 ******************************************************************************/
TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Translation_Image_Linear", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({100.0F, 50.0F, -100.0F}));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
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

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Rotation_Image_Linear", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F, 45.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
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

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale_Image_Linear", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({0.5F, 1.5F, 10.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
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

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Manual_Image_Linear", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath)); // This should reflect the geometry across the x-axis.
    const DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}};
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<nx::core::DynamicTableParameter::ValueType>(dynamicTable));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
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

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Precomputed_Image_Linear", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_linear.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_LinearInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    const DataPath precomputedPath({apply_transformation_to_geometry::k_InputGeometryName, "Precomputed AM", "TransformationMatrix"});
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(precomputedPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
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
TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Translation_Image_NN", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({100.0F, 50.0F, -100.0F}));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
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

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Rotation_Image_NN", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F, 45.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
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

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale_Image_NN", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({0.5F, 1.5F, 10.0F}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
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

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Manual_Image_NN", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath)); // This should reflect the geometry across the x-axis.
    const DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}};
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<nx::core::DynamicTableParameter::ValueType>(dynamicTable));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
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

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Precomputed_Image_NN", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
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
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_apply_transformation_to_geometry.tar.gz",
                                                               "6_6_apply_transformation_to_geometry");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_apply_transformation_to_geometry/6_6_apply_transformation_to_geometry_nn.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({"InputData"});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_CellData);
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
    const DataPath precomputedPath({apply_transformation_to_geometry::k_InputGeometryName, "Precomputed AM", "TransformationMatrix"});
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(precomputedPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/apply_transformation_to_geometry_manual.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    const DataPath exemplarPath({apply_transformation_to_geometry::k_PrecomputedGeometryName66, k_CellData, "Data"});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputGeometryName, k_CellData, "Data"});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);

    {
      const auto& exemplarArr = dataStructure.getDataRefAs<DataArray<int32>>(exemplarPath);
      std::cout << "ApplyTransformationToGeometryFilter: Exemplar: " << exemplarArr.getDataStoreRef().xarray() << std::endl;

      const auto& calculatedDataArr = dataStructure.getDataRefAs<DataArray<int32>>(calculatedPath);
      std::cout << "ApplyTransformationToGeometryFilter: Data: " << calculatedDataArr.getDataStoreRef().xarray() << std::endl;
    }
    UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
  }
}

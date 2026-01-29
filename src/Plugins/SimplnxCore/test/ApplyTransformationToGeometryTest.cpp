#include "SimplnxCore/Filters/ApplyTransformationToGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

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
const std::string k_InputNodeGeometryName("InputNodeData");
const DataPath k_InputCellAttrMatrixPath(DataPath({k_InputGeometryName, "VertexData"}));
const std::string k_Rotation45XGeometryName("Rotation45X");
const std::string k_Rotation45YGeometryName("Rotation45Y");
const std::string k_Rotation45ZGeometryName("Rotation45Z");
const std::string k_Rotation90XGeometryName("Rotation90X");
const std::string k_Rotation90YGeometryName("Rotation90Y");
const std::string k_Rotation90ZGeometryName("Rotation90Z");
const std::string k_ScaleGeometryName("Scale");
const std::string k_ScaleNodeGeometryName("Scale_Node");
const std::string k_TranslationGeometryName("Translation");
const std::string k_TranslationNodeGeometryName("Translation_Node");
const std::string k_ManualGeometryName("Manual");
const std::string k_ManualNodeGeometryName("Manual_Node");
const std::string k_PrecomputedGeometryName("Precomputed");
const std::string k_PrecomputedNodeGeometryName("Precomputed_Node");
const std::string k_Rotation45XNodeGeometryName("Rotation_45X_Node");
const std::string k_Rotation45YNodeGeometryName("Rotation_45Y_Node");
const std::string k_Rotation45ZNodeGeometryName("Rotation_45Z_Node");
const std::string k_Rotation90XNodeGeometryName("Rotation_90X_Node");
const std::string k_Rotation90YNodeGeometryName("Rotation_90Y_Node");
const std::string k_Rotation90ZNodeGeometryName("Rotation_90Z_Node");
const std::string k_Rotation45XGlobalGeometryName("Rotation45X_Global");
const std::string k_Rotation45YGlobalGeometryName("Rotation45Y_Global");
const std::string k_Rotation45ZGlobalGeometryName("Rotation45Z_Global");
const std::string k_Rotation90XGlobalGeometryName("Rotation90X_Global");
const std::string k_Rotation90YGlobalGeometryName("Rotation90Y_Global");
const std::string k_Rotation90ZGlobalGeometryName("Rotation90Z_Global");
const std::string k_ScaleGlobalGeometryName("Scale_Global");
const std::string k_TranslationGlobalGeometryName("Translation_Global");
const std::string k_ManualGlobalGeometryName("Manual_Global");
const std::string k_PrecomputedGlobalGeometryName("Precomputed_Global");
const DataPath k_PrecomputedTransformationMatrixPath({"Transformation Matrices", "Precomputed"});
const std::string k_ExemplaryNNDataName("Data_NN");
const std::string k_ExemplaryLinearDataName("Data_L");

const std::string k_SharedVertexListName("SharedVertexList");

const int32 k_CellAttrMatrixUnusedWarning = -5555;

void CompareImageGeometries(const DataStructure& dataStructure, const ImageGeom& exemplaryGeom, const ImageGeom& calculatedGeom, const std::string& exemplaryDataName)
{
  UnitTest::CompareImageGeometry(&exemplaryGeom, &calculatedGeom);

  REQUIRE_NOTHROW(exemplaryGeom.getCellDataRef());
  REQUIRE_NOTHROW(calculatedGeom.getCellDataRef());
  auto exemplaryAM = exemplaryGeom.getCellDataRef();
  auto calculatedAM = calculatedGeom.getCellDataRef();
  REQUIRE(exemplaryAM.getShape() == calculatedAM.getShape());

  const DataPath exemplarPath({exemplaryGeom.getName(), k_Cell_Data, exemplaryDataName});
  const DataPath calculatedPath({calculatedGeom.getName(), k_Cell_Data, "Data"});
  const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
  const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
  UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
}

} // namespace apply_transformation_to_geometry

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Translation_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
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
    const DataPath exemplarPath({apply_transformation_to_geometry::k_TranslationNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_translation.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Rotation_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});

  VectorFloat32Parameter::ValueType rotation45X = {1.0F, 0.0F, 0.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation45Y = {0.0F, 1.0F, 0.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation45Z = {0.0F, 0.0F, 1.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation90X = {1.0F, 0.0F, 0.0F, 90.0F};
  VectorFloat32Parameter::ValueType rotation90Y = {0.0F, 1.0F, 0.0F, 90.0F};
  VectorFloat32Parameter::ValueType rotation90Z = {0.0F, 0.0F, 1.0F, 90.0F};
  auto [exemplaryGeomName, rotation] = GENERATE_REF(
      std::make_tuple(apply_transformation_to_geometry::k_Rotation45XNodeGeometryName, rotation45X), std::make_tuple(apply_transformation_to_geometry::k_Rotation45YNodeGeometryName, rotation45Y),
      std::make_tuple(apply_transformation_to_geometry::k_Rotation45ZNodeGeometryName, rotation45Z), std::make_tuple(apply_transformation_to_geometry::k_Rotation90XNodeGeometryName, rotation90X),
      std::make_tuple(apply_transformation_to_geometry::k_Rotation90YNodeGeometryName, rotation90Y), std::make_tuple(apply_transformation_to_geometry::k_Rotation90ZNodeGeometryName, rotation90Z));

  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>(rotation));
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
    const DataPath exemplarPath({exemplaryGeomName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_rotation.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
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
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ScaleNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_scale.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale: Origin_And_Spacing Check", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const std::string k_SelectedImageGeomName = "Image Geometry";
  const DataPath k_SelectedImageGeomPath = DataPath({k_SelectedImageGeomName});
  const std::string k_SelectedAttrMatrixName = "Cell Data";
  const DataPath k_SelectedAttrMatrixPath = DataPath({k_SelectedImageGeomName, k_SelectedAttrMatrixName});

  DataStructure ds;
  ImageGeom* geom = ImageGeom::Create(ds, "Image Geometry");

  Vec3<float32> origin = {51.0F, 23.0F, -64.0F};
  Vec3<float32> spacing = {3.0F, 5.0F, 8.0F};
  std::vector<float32> scaleFactor;
  SECTION("Scale Increasing")
  {
    scaleFactor = {2.0F, 2.0F, 2.0F};
  }
  SECTION("Scale Decreasing")
  {
    scaleFactor = {0.4F, 0.4F, 0.4F};
  }
  geom->setOrigin(origin);
  geom->setSpacing(spacing);
  AttributeMatrix* cellAM = AttributeMatrix::Create(ds, "Cell Data", {}, geom->getId());

  const ApplyTransformationToGeometryFilter filter;
  Arguments args;

  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedImageGeomPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(k_SelectedAttrMatrixPath));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>(scaleFactor));
  args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(0));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  Vec3<float32> newOrigin = {1.0f, 1.0f, 1.0f};
  std::transform(origin.begin(), origin.end(), scaleFactor.begin(), newOrigin.begin(), std::multiplies<>());
  Vec3<float32> newSpacing = {1.0f, 1.0f, 1.0f};
  std::transform(spacing.begin(), spacing.end(), scaleFactor.begin(), newSpacing.begin(), std::multiplies<>());
  REQUIRE(geom->getOrigin() == newOrigin);
  REQUIRE(geom->getSpacing() == newSpacing);

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Manual_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
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
    const DataPath exemplarPath({apply_transformation_to_geometry::k_ManualNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_manual.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Precomputed_Node", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputNodeGeometryName});
  {
    const ApplyTransformationToGeometryFilter filter;
    Arguments args;

    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_InputCellAttrMatrixPath));
    args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                        std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
    const DataPath precomputedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, "Precomputed AM", "TransformationMatrix"});
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
    const DataPath exemplarPath({apply_transformation_to_geometry::k_PrecomputedNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const DataPath calculatedPath({apply_transformation_to_geometry::k_InputNodeGeometryName, apply_transformation_to_geometry::k_SharedVertexListName});
    const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
    const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
    UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
  }
  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/ApplyTransformationToGeometryFilter_precomputed.dream3d", unit_test::k_BinaryTestOutputDir));

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  SIMPLNX_RESULT_REQUIRE_VALID(resultH5);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/*******************************************************************************
 * @brief This section is for Image Geometry with Nearest Neighbor Interpolation
 ******************************************************************************/
TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Translation_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  auto [translateGeomToGlobalOrigin, exemplaryGeomName, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_TranslationGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_TranslationGlobalGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_TranslationGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_TranslationGlobalGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName, interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
    const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_TranslationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Translation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({-10.0F, 10.0F, 20.0F}));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Rotation_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

  VectorFloat32Parameter::ValueType rotation45X = {1.0F, 0.0F, 0.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation45Y = {0.0F, 1.0F, 0.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation45Z = {0.0F, 0.0F, 1.0F, 45.0F};
  VectorFloat32Parameter::ValueType rotation90X = {1.0F, 0.0F, 0.0F, 90.0F};
  VectorFloat32Parameter::ValueType rotation90Y = {0.0F, 1.0F, 0.0F, 90.0F};
  VectorFloat32Parameter::ValueType rotation90Z = {0.0F, 0.0F, 1.0F, 90.0F};
  auto [translateGeomToGlobalOrigin, exemplaryGeomName, rotation, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45XGeometryName, rotation45X, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45XGlobalGeometryName, rotation45X, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45YGeometryName, rotation45Y, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45YGlobalGeometryName, rotation45Y, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45ZGeometryName, rotation45Z, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45ZGlobalGeometryName, rotation45Z, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90XGeometryName, rotation90X, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90XGlobalGeometryName, rotation90X, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90YGeometryName, rotation90Y, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90YGlobalGeometryName, rotation90Y, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90ZGeometryName, rotation90Z, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90ZGlobalGeometryName, rotation90Z, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45XGeometryName, rotation45X, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45XGlobalGeometryName, rotation45X, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45YGeometryName, rotation45Y, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45YGlobalGeometryName, rotation45Y, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation45ZGeometryName, rotation45Z, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation45ZGlobalGeometryName, rotation45Z, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90XGeometryName, rotation90X, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90XGlobalGeometryName, rotation90X, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90YGeometryName, rotation90Y, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90YGlobalGeometryName, rotation90Y, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_Rotation90ZGeometryName, rotation90Z, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_Rotation90ZGlobalGeometryName, rotation90Z, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Rotation = [{}, {}, {}, {}], Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName, rotation[0], rotation[1], rotation[2],
                              rotation[3], interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_RotationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Rotation_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>(rotation));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Scale_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

  auto [translateGeomToGlobalOrigin, exemplaryGeomName, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_ScaleGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_ScaleGlobalGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_ScaleGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_ScaleGlobalGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName, interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ScaleIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_Scale_Key, std::make_any<nx::core::VectorFloat32Parameter::ValueType>({0.05F, 0.05F, 0.05F}));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Manual_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

  auto [translateGeomToGlobalOrigin, exemplaryGeomName, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_ManualGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_ManualGlobalGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_ManualGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_ManualGlobalGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName, interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                          std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_ManualTransformationMatrixIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath)); // This should reflect the geometry across the x-axis.
      const DynamicTableParameter::ValueType dynamicTable{{{-1.0, 0, 0, 0}, {0, 1.0, 0, 0}, {0, 0, 1.0, 0}, {0, 0, 0, 1.0}}};
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key, std::make_any<nx::core::DynamicTableParameter::ValueType>(dynamicTable));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ApplyTransformationToGeometryFilter:Precomputed_Image", "[SimplnxCore][ApplyTransformationToGeometryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "apply_transformation_to_geometry.tar.gz",
                                                               "apply_transformation_to_geometry.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/apply_transformation_to_geometry.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath inputGeometryPath({apply_transformation_to_geometry::k_InputGeometryName});
  const DataPath inputCellAMPath = inputGeometryPath.createChildPath(k_Cell_Data);

  auto [translateGeomToGlobalOrigin, exemplaryGeomName, interpolationIdx] =
      GENERATE_REF(std::make_tuple(false, apply_transformation_to_geometry::k_PrecomputedGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_PrecomputedGlobalGeometryName, apply_transformation_to_geometry::k_LinearInterpolationIdx),
                   std::make_tuple(false, apply_transformation_to_geometry::k_PrecomputedGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx),
                   std::make_tuple(true, apply_transformation_to_geometry::k_PrecomputedGlobalGeometryName, apply_transformation_to_geometry::k_NearestNeighborInterpolationIdx));

  std::string interpolationTypeStr;
  std::string exemplaryDataName;
  if(interpolationIdx == apply_transformation_to_geometry::k_LinearInterpolationIdx)
  {
    interpolationTypeStr = "Linear";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryLinearDataName;
  }
  else
  {
    interpolationTypeStr = "Nearest Neighbor";
    exemplaryDataName = apply_transformation_to_geometry::k_ExemplaryNNDataName;
  }

  DYNAMIC_SECTION(fmt::format("Geometry Name = {}, Precomputed Matrix Path = {}, Interpolation Type = {}, Translate To Global Origin = {}", exemplaryGeomName,
                              apply_transformation_to_geometry::k_PrecomputedTransformationMatrixPath.toString(), interpolationTypeStr, translateGeomToGlobalOrigin))
  {
    {
      const ApplyTransformationToGeometryFilter filter;
      Arguments args;

      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(inputGeometryPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TransformationType_Key,
                          std::make_any<nx::core::ChoicesParameter::ValueType>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_InterpolationType_Key, std::make_any<nx::core::ChoicesParameter::ValueType>(interpolationIdx));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key, std::make_any<DataPath>(inputCellAMPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key, std::make_any<DataPath>(apply_transformation_to_geometry::k_PrecomputedTransformationMatrixPath));
      args.insertOrAssign(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key, std::make_any<nx::core::BoolParameter::ValueType>(translateGeomToGlobalOrigin));

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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
    auto exemplaryGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({exemplaryGeomName}));
    auto calculatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
    apply_transformation_to_geometry::CompareImageGeometries(dataStructure, exemplaryGeom, calculatedGeom, exemplaryDataName);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

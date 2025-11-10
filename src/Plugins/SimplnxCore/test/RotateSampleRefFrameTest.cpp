#include "SimplnxCore/Filters/RotateSampleRefFrameFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <Eigen/Dense>
#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{

void CompareImageGeometryAlt(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, float32 threshold = 0.0f)
{
  INFO(fmt::format("Comparing Image Geometries. {} and {}", exemplaryDataPath.toString(), computedPath.toString()));

  const auto* exemplarGeom = dataStructure.getDataAs<ImageGeom>(exemplaryDataPath);
  const auto* computedGeom = dataStructure.getDataAs<ImageGeom>(computedPath);
  REQUIRE(exemplarGeom != nullptr);
  REQUIRE(computedGeom != nullptr);

  const auto exemplarDims = exemplarGeom->getDimensions();
  const auto computedDims = computedGeom->getDimensions();
  REQUIRE(exemplarDims == computedDims);

  const auto exemplarSpacing = exemplarGeom->getSpacing();
  const auto computedSpacing = computedGeom->getSpacing();
  REQUIRE(std::fabs(exemplarSpacing[0] - computedSpacing[0]) <= threshold);
  REQUIRE(std::fabs(exemplarSpacing[1] - computedSpacing[1]) <= threshold);
  REQUIRE(std::fabs(exemplarSpacing[2] - computedSpacing[2]) <= threshold);
}

std::vector<std::vector<float64>> ConvertMatrixToTable(const Eigen::Matrix3f& matrix)
{
  std::vector<std::vector<float64>> data;

  for(Eigen::Index i = 0; i < matrix.rows(); i++)
  {
    std::vector<float64> row;
    for(Eigen::Index j = 0; j < matrix.cols(); j++)
    {
      row.push_back(matrix(i, j));
    }
    row.push_back(0.0);
    data.push_back(row);
  }

  data.push_back({0.0l, 0.0, 0.0, 1.0});
  return data;
}
} // namespace

TEST_CASE("SimplnxCore::RotateSampleRefFrame", "[Core][RotateSampleRefFrameFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Rotate_Sample_Ref_Frame_Test_v3.tar.gz",
                                                              "Rotate_Sample_Ref_Frame_Test_v3");

  const DataPath k_OriginalGeomPath({"Original"});

  Result<DataStructure> dataStructureResult =
      DREAM3D::ImportDataStructureFromFile(fs::path(fmt::format("{}/Rotate_Sample_Ref_Frame_Test_v3/Rotate_Sample_Ref_Frame_Test_v3.dream3d", nx::core::unit_test::k_TestFilesDir)), false);
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructureResult);

  DataStructure dataStructure = std::move(dataStructureResult.value());

  const auto* originalImageGeom = dataStructure.getDataAs<ImageGeom>(k_OriginalGeomPath);
  REQUIRE(originalImageGeom != nullptr);

  auto [sectionName, exemplaryGeomPath, axisAngle, keepInputGeometryOrigin] =
      GENERATE(std::make_tuple("180 degrees around X", DataPath({"180_100"}), VectorFloat32Parameter::ValueType{1.0F, 0.0F, 0.0F, 180.0F}, false),
               std::make_tuple("180 degrees around Y", DataPath({"180_010"}), VectorFloat32Parameter::ValueType{0.0F, 1.0F, 0.0F, 180.0F}, false),
               std::make_tuple("180 degrees around Z", DataPath({"180_001"}), VectorFloat32Parameter::ValueType{0.0F, 0.0F, 1.0F, 180.0F}, false),
               std::make_tuple("90 degrees around X", DataPath({"90_100"}), VectorFloat32Parameter::ValueType{1.0F, 0.0F, 0.0F, 90.0F}, false),
               std::make_tuple("90 degrees around Y", DataPath({"90_010"}), VectorFloat32Parameter::ValueType{0.0F, 1.0F, 0.0F, 90.0F}, false),
               std::make_tuple("90 degrees around Z", DataPath({"90_001"}), VectorFloat32Parameter::ValueType{0.0F, 0.0F, 1.0F, 90.0F}, false),
               std::make_tuple("45 degrees around X", DataPath({"45_100"}), VectorFloat32Parameter::ValueType{1.0F, 0.0F, 0.0F, 45.0F}, false),
               std::make_tuple("45 degrees around Y", DataPath({"45_010"}), VectorFloat32Parameter::ValueType{0.0F, 1.0F, 0.0F, 45.0F}, false),
               std::make_tuple("45 degrees around Z", DataPath({"45_001"}), VectorFloat32Parameter::ValueType{0.0F, 0.0F, 1.0F, 45.0F}, false),
               std::make_tuple("180 degrees around X - Keep Origin", DataPath({"180_100_KeepOrigin"}), VectorFloat32Parameter::ValueType{1.0F, 0.0F, 0.0F, 180.0F}, true),
               std::make_tuple("180 degrees around Y - Keep Origin", DataPath({"180_010_KeepOrigin"}), VectorFloat32Parameter::ValueType{0.0F, 1.0F, 0.0F, 180.0F}, true),
               std::make_tuple("180 degrees around Z - Keep Origin", DataPath({"180_001_KeepOrigin"}), VectorFloat32Parameter::ValueType{0.0F, 0.0F, 1.0F, 180.0F}, true),
               std::make_tuple("90 degrees around X - Keep Origin", DataPath({"90_100_KeepOrigin"}), VectorFloat32Parameter::ValueType{1.0F, 0.0F, 0.0F, 90.0F}, true),
               std::make_tuple("90 degrees around Y - Keep Origin", DataPath({"90_010_KeepOrigin"}), VectorFloat32Parameter::ValueType{0.0F, 1.0F, 0.0F, 90.0F}, true),
               std::make_tuple("90 degrees around Z - Keep Origin", DataPath({"90_001_KeepOrigin"}), VectorFloat32Parameter::ValueType{0.0F, 0.0F, 1.0F, 90.0F}, true),
               std::make_tuple("45 degrees around X - Keep Origin", DataPath({"45_100_KeepOrigin"}), VectorFloat32Parameter::ValueType{1.0F, 0.0F, 0.0F, 45.0F}, true),
               std::make_tuple("45 degrees around Y - Keep Origin", DataPath({"45_010_KeepOrigin"}), VectorFloat32Parameter::ValueType{0.0F, 1.0F, 0.0F, 45.0F}, true),
               std::make_tuple("45 degrees around Z - Keep Origin", DataPath({"45_001_KeepOrigin"}), VectorFloat32Parameter::ValueType{0.0F, 0.0F, 1.0F, 45.0F}, true));

  SECTION(sectionName)
  {
    fmt::print("Testing {}\n", sectionName);

    RotateSampleRefFrameFilter filter;
    Arguments args;

    DataPath outputImageGeomPath = DataPath({fmt::format("{}_Test_AxisAngle", exemplaryGeomPath.getTargetName())});

    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrameFilter::RotationRepresentation::AxisAngle)));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_OriginalGeomPath));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false)); // We need to keep the geometries around.
    args.insertOrAssign(RotateSampleRefFrameFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(outputImageGeomPath));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationAxisAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>(axisAngle));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_KeepInputGeometryOrigin_Key, std::make_any<bool>(keepInputGeometryOrigin));

    auto preflightAxisAngleResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightAxisAngleResult.outputActions);

    auto executeAxisAngleResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeAxisAngleResult.result);

    auto* outputImageGeom = dataStructure.getDataAs<ImageGeom>(outputImageGeomPath);
    REQUIRE(outputImageGeom != nullptr);

    {
      UnitTest::CompareImageGeometry(dataStructure, exemplaryGeomPath, outputImageGeomPath, UnitTest::EPSILON);

      DataPath exemplarAMDataPath = exemplaryGeomPath.createChildPath("CellData");
      DataPath outputAMDataPath = outputImageGeomPath.createChildPath("CellData");
      UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAMDataPath, dataStructure, outputAMDataPath);
    }

    /* This section will convert the Axis Angle into a Rotation Matrix and send that into the
     * filter as the RotateSampleRefFrameFilter::RotationRepresentation::RotationMatrix type
     */
    Eigen::Vector3f axis(axisAngle[0], axisAngle[1], axisAngle[2]);
    float32 angleRadians = axisAngle[3] * (numbers::pi / 180.0F);
    Eigen::Matrix3f rotationMatrix = Eigen::AngleAxisf(angleRadians, axis).toRotationMatrix();

    std::vector<std::vector<float64>> table = ConvertMatrixToTable(rotationMatrix);

    outputImageGeomPath = DataPath({fmt::format("{}_Test_RotationMatrix", exemplaryGeomPath.getTargetName())});

    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrameFilter::RotationRepresentation::RotationMatrix)));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(table));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(outputImageGeomPath));

    auto preflightRotationMatrixResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightRotationMatrixResult.outputActions);

    auto executeRotationMatrixResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeRotationMatrixResult.result);

    {
      UnitTest::CompareImageGeometry(dataStructure, exemplaryGeomPath, outputImageGeomPath, UnitTest::EPSILON);

      DataPath exemplarAMDataPath = exemplaryGeomPath.createChildPath("CellData");
      DataPath outputAMDataPath = outputImageGeomPath.createChildPath("CellData");
      UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAMDataPath, dataStructure, outputAMDataPath);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

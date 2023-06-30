#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/RotateSampleRefFrameFilter.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <Eigen/Dense>
#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
bool AreImageGeomsEqual(const ImageGeom& geom1, const ImageGeom& geom2)
{
  SizeVec3 dims1 = geom1.getDimensions();
  FloatVec3 origin1 = geom1.getOrigin();
  FloatVec3 spacing1 = geom1.getSpacing();

  SizeVec3 dims2 = geom2.getDimensions();
  FloatVec3 origin2 = geom2.getOrigin();
  FloatVec3 spacing2 = geom2.getSpacing();

  return (dims1 == dims2) && (origin1 == origin2) && (spacing1 == spacing2);
}

struct AreArraysEqualFunctor
{
  template <class T>
  bool operator()(const IDataStore& array1, const IDataStore& array2) const
  {
    const auto& dataStore1 = dynamic_cast<const AbstractDataStore<T>&>(array1);
    const auto& dataStore2 = dynamic_cast<const AbstractDataStore<T>&>(array2);

    usize size = dataStore1.getSize();
    bool failed = false;
    for(usize i = 0; i < size; i++)
    {
      T value1 = dataStore1[i];
      T value2 = dataStore2[i];
      if(value1 != value2)
      {
        UNSCOPED_INFO(fmt::format("index: {}    value1 != value2. {} != {}", i, value1, value2));
        failed = true;
        break;
      }
    }
    REQUIRE(!failed);
    return !failed;
  }
};

bool AreArraysEqual(const IDataArray& array1, const IDataArray& array2)
{
  const IDataStore& dataStore1 = array1.getIDataStoreRef();
  const IDataStore& dataStore2 = array1.getIDataStoreRef(); // revert to broken state ---- update coming

  DataType dataType1 = dataStore1.getDataType();

  if(dataType1 != dataStore2.getDataType())
  {
    return false;
  }

  if(dataStore1.getTupleShape() != dataStore2.getTupleShape())
  {
    return false;
  }

  if(dataStore1.getComponentShape() != dataStore2.getComponentShape())
  {
    return false;
  }
  INFO(fmt::format("Input Data Array:'{}'  Output DataArray: '{}' bad comparison", array1.getName(), array2.getName()));

  return ExecuteDataFunction(AreArraysEqualFunctor{}, dataType1, dataStore1, dataStore2);
}

std::vector<std::vector<float64>> ConvertMatrixToTable(const Eigen::Matrix3f& matrix)
{
  std::vector<std::vector<float64>> data;

  for(Eigen::Index i = 0; i < matrix.rows(); i++)
  {
    std::vector<float64> row;
    for(Eigen::Index j = 0; j < matrix.cols(); j++)
    {
      row.push_back(static_cast<float64>(matrix(i, j)));
    }
    data.push_back(row);
  }

  return data;
}
} // namespace

TEST_CASE("ComplexCore::RotateSampleRefFrame", "[Core][RotateSampleRefFrameFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "Rotate_Sample_Ref_Frame_Test.tar.gz",
                                                             "Rotate_Sample_Ref_Frame_Test.dream3d");

  const DataPath k_OriginalGeomPath({"Original"});
  const DataPath k_OriginalCellArrayPath = k_OriginalGeomPath.createChildPath("CellData").createChildPath("Data");

  Result<DataStructure> dataStructureResult = DREAM3D::ImportDataStructureFromFile(fs::path(fmt::format("{}/Rotate_Sample_Ref_Frame_Test.dream3d", complex::unit_test::k_TestFilesDir)));
  COMPLEX_RESULT_REQUIRE_VALID(dataStructureResult);

  DataStructure dataStructure = std::move(dataStructureResult.value());

  const auto* originalImageGeom = dataStructure.getDataAs<ImageGeom>(k_OriginalGeomPath);
  REQUIRE(originalImageGeom != nullptr);

  std::vector<DataObject*> dataContainers = dataStructure.getTopLevelData();

  std::map<std::string, std::vector<float>> axisAngleMapping = {
      {"Test_1", {1.0F, 0.0F, 0.0F, 90.0F}},
      {"Test_2", {0.0F, 1.0F, 0.0F, 90.0F}},
      {"Test_3", {0.0F, 0.0F, 1.0F, 90.0F}},
      {"Test_4", {0.357407F, 0.862856F, 0.357407F, 64.7368F}},
  };

  for(auto* dc : dataContainers)
  {
    RotateSampleRefFrameFilter filter;
    Arguments args;
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrameFilter::RotationRepresentation::AxisAngle)));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_OriginalGeomPath));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false)); // We need to keep the geometries around.

    REQUIRE(dc != nullptr);

    const std::string name = dc->getName();
    if(name.find("Rotate_") != 0) // Skip the "Original" Image Geometry
    {
      continue;
    }

    auto* expectedImageGeom = dynamic_cast<ImageGeom*>(dc);
    REQUIRE(expectedImageGeom != nullptr);
    DataPath expectedRotatedCellArrayPath({name, "CellData", "Data"});

    const auto* expectedRotatedArray = dataStructure.getDataAs<IDataArray>(expectedRotatedCellArrayPath);
    REQUIRE(expectedRotatedArray != nullptr);

    const DataPath testAxisAngleGeomPath({fmt::format("{}_Test_AxisAngle", name)});
    args.insertOrAssign(RotateSampleRefFrameFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(testAxisAngleGeomPath));

    const std::vector<std::string> parts = StringUtilities::split(name, '_');
    REQUIRE(parts.size() == 5);

    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationAxisAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>(axisAngleMapping[name]));

    auto preflightAxisAngleResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightAxisAngleResult.outputActions);

    auto executeAxisAngleResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeAxisAngleResult.result);

    auto* testAxisAngleGeom = dataStructure.getDataAs<ImageGeom>(testAxisAngleGeomPath);
    REQUIRE(testAxisAngleGeom != nullptr);

    REQUIRE(AreImageGeomsEqual(*testAxisAngleGeom, *expectedImageGeom));

    DataPath testAxisAngleArrayPath = testAxisAngleGeomPath.createChildPath("CellData").createChildPath(k_OriginalCellArrayPath.getTargetName());
    auto* testAxisAngleArray = dataStructure.getDataAs<IDataArray>(testAxisAngleArrayPath);
    REQUIRE(testAxisAngleArray != nullptr);

    REQUIRE(AreArraysEqual(*testAxisAngleArray, *expectedRotatedArray));

    /* This section will convert the Axis Angle into a Rotation Matrix and send that into the
     * filter as the RotateSampleRefFrameFilter::RotationRepresentation::RotationMatrix type
     */
    auto axisAngleEntry = axisAngleMapping[name];

    Eigen::Vector3f axis(axisAngleEntry[0], axisAngleEntry[1], axisAngleEntry[2]);
    float32 angleRadians = axisAngleEntry[3] * (numbers::pi / 180.0F);
    Eigen::AngleAxisf axisAngle(angleRadians, axis);

    Eigen::Matrix3f rotationMatrix = axisAngle.toRotationMatrix();

    std::vector<std::vector<float64>> table = ConvertMatrixToTable(rotationMatrix);

    DataPath testRotationMatrixGeomPath({fmt::format("{}_Test_RotationMatrix", name)});

    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrameFilter::RotationRepresentation::RotationMatrix)));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(table));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(testRotationMatrixGeomPath));

    auto preflightRotationMatrixResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightRotationMatrixResult.outputActions);

    auto executeRotationMatrixResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeRotationMatrixResult.result);

    auto* testRotationMatrixGeom = dataStructure.getDataAs<ImageGeom>(testRotationMatrixGeomPath);
    REQUIRE(testRotationMatrixGeom != nullptr);

    REQUIRE(AreImageGeomsEqual(*testRotationMatrixGeom, *expectedImageGeom));

    const DataPath testRotationMatrixArrayPath = testRotationMatrixGeomPath.createChildPath("CellData").createChildPath(k_OriginalCellArrayPath.getTargetName());
    auto* testRotationMatrixArray = dataStructure.getDataAs<IDataArray>(testRotationMatrixArrayPath);
    REQUIRE(testRotationMatrixArray != nullptr);

    REQUIRE(AreArraysEqual(*testRotationMatrixArray, *expectedRotatedArray));
  }
}

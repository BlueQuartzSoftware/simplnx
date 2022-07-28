#include <catch2/catch.hpp>

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/RotateSampleRefFrameFilter.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <Eigen/Dense>

#include <filesystem>
#include <stdexcept>
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

    for(usize i = 0; i < size; i++)
    {
      T value1 = dataStore1[i];
      T value2 = dataStore2[i];
      if(value1 != value2)
      {
        return false;
      }
    }

    return true;
  }
};

bool AreArraysEqual(const IDataArray& array1, const IDataArray& array2)
{
  const IDataStore& dataStore1 = array1.getIDataStoreRef();
  const IDataStore& dataStore2 = array1.getIDataStoreRef();

  DataType dataType1 = dataStore1.getDataType();
  DataType dataType2 = dataStore2.getDataType();
  if(dataType1 != dataType2)
  {
    return false;
  }

  IDataStore::ShapeType tupleShape1 = dataStore1.getTupleShape();
  IDataStore::ShapeType tupleShape2 = dataStore2.getTupleShape();
  if(tupleShape1 != tupleShape2)
  {
    return false;
  }

  IDataStore::ShapeType componentShape1 = dataStore1.getComponentShape();
  IDataStore::ShapeType componentShape2 = dataStore2.getComponentShape();
  if(componentShape1 != componentShape2)
  {
    return false;
  }

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

TEST_CASE("Core::RotateSampleRefFrameFilter", "[Core][RotateSampleRefFrameFilter]")
{
  const DataPath k_OriginalGeomPath({"Original"});
  const DataPath k_OriginalCellArrayPath = k_OriginalGeomPath.createChildPath("CellData").createChildPath("Data");

  Result<DataStructure> dataStructureResult =
      DREAM3D::ImportDataStructureFromFile(fs::path(fmt::format("{}/src/Plugins/ComplexCore/data/RotateSampleRefFrameTest.dream3d", complex::unit_test::k_ComplexSourceDir)));
  COMPLEX_RESULT_REQUIRE_VALID(dataStructureResult);

  DataStructure dataStructure = std::move(dataStructureResult.value());

  const auto* originalImageGeom = dataStructure.getDataAs<ImageGeom>(k_OriginalGeomPath);
  REQUIRE(originalImageGeom != nullptr);

  std::vector<DataObject*> dataContainers = dataStructure.getTopLevelData();

  RotateSampleRefFrameFilter filter;

  for(auto* dc : dataContainers)
  {
    Arguments args;
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrameFilter::RotationRepresentation::AxisAngle)));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_OriginalGeomPath));

    REQUIRE(dc != nullptr);

    std::string name = dc->getName();
    if(name.find("Rotate_") != 0)
    {
      continue;
    }

    auto* expectedImageGeom = dynamic_cast<ImageGeom*>(dc);
    REQUIRE(expectedImageGeom != nullptr);

    DataPath testAxisAngleGeomPath({fmt::format("{}_Test_AxisAngle", name)});
    DataPath expectedRotatedCellArrayPath({name, "CellData", "Data"});

    const auto* expectedRotatedArray = dataStructure.getDataAs<IDataArray>(expectedRotatedCellArrayPath);
    REQUIRE(expectedRotatedArray != nullptr);

    args.insertOrAssign(RotateSampleRefFrameFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(testAxisAngleGeomPath));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_SelectedCellArrays_Key, std::make_any<std::vector<DataPath>>({k_OriginalCellArrayPath}));

    std::vector<std::string> parts = StringUtilities::split(name, '_');
    REQUIRE(parts.size() == 5);

    float32 x = std::stof(parts[1]);
    float32 y = std::stof(parts[2]);
    float32 z = std::stof(parts[3]);
    float32 angle = std::stof(parts[4]);

    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationAngle_Key, std::make_any<Float32Parameter::ValueType>(angle));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationAxis_Key, std::make_any<VectorFloat32Parameter::ValueType>({x, y, z}));

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

    Eigen::Vector3f axis(x, y, z);
    float32 angleRadians = angle * (numbers::pi / 180.0);
    Eigen::AngleAxisf axisAngle(angleRadians, axis);

    Eigen::Matrix3f rotationMatrix = axisAngle.toRotationMatrix();

    std::vector<std::vector<float64>> table = ConvertMatrixToTable(rotationMatrix);

    DynamicTableData tableData(table, {}, {});

    DataPath testRotationMatrixGeomPath({fmt::format("{}_Test_RotationMatrix", name)});

    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrameFilter::RotationRepresentation::RotationMatrix)));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key, std::make_any<DynamicTableParameter::ValueType>(tableData));
    args.insertOrAssign(RotateSampleRefFrameFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(testRotationMatrixGeomPath));

    auto preflightRotationMatrixResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightRotationMatrixResult.outputActions);

    auto executeRotationMatrixResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeRotationMatrixResult.result);

    auto* testRotationMatrixGeom = dataStructure.getDataAs<ImageGeom>(testRotationMatrixGeomPath);
    REQUIRE(testRotationMatrixGeom != nullptr);

    REQUIRE(AreImageGeomsEqual(*testRotationMatrixGeom, *expectedImageGeom));

    DataPath testRotationMatrixArrayPath = testRotationMatrixGeomPath.createChildPath("CellData").createChildPath(k_OriginalCellArrayPath.getTargetName());
    auto* testRotationMatrixArray = dataStructure.getDataAs<IDataArray>(testRotationMatrixArrayPath);
    REQUIRE(testRotationMatrixArray != nullptr);

    REQUIRE(AreArraysEqual(*testRotationMatrixArray, *expectedRotatedArray));
  }
}

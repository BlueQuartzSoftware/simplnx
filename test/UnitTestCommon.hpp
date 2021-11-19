#pragma once

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/IDataStore.hpp"

using namespace complex;

namespace ComplexUnitTest
{

namespace Constants
{
inline constexpr StringLiteral k_SmallIN100("Small IN100");
inline constexpr StringLiteral k_EbsdScanData("EBSD Scan Data");
inline constexpr StringLiteral k_ImageGeometry("Image Geometry");
inline constexpr StringLiteral k_ConfidenceIndex("Confidence Index");

inline constexpr StringLiteral k_LevelZero("ZERO");
inline constexpr StringLiteral k_LevelOne("ONE");
inline constexpr StringLiteral k_LevelTwo("TWO");

inline constexpr StringLiteral k_Int8("int8 DataSet");
inline constexpr StringLiteral k_Uint8("uint8 DataSet");

inline constexpr StringLiteral k_Int16("int16 DataSet");
inline constexpr StringLiteral k_Uint16("uint16 DataSet");

inline constexpr StringLiteral k_Int32("int32 DataSet");
inline constexpr StringLiteral k_Uint32("uint32 DataSet");

inline constexpr StringLiteral k_Int64("int64 DataSet");
inline constexpr StringLiteral k_Uint64("uint64 DataSet");

inline constexpr StringLiteral k_Float("float DataSet");
inline constexpr StringLiteral k_Double("double DataSet");

inline constexpr StringLiteral k_ConditionalArray("Conditional [bool]");

} // namespace Constants

/**
 * @brief Creates a DataArray backed by a DataStore (in memory).
 * @tparam T The primitive type to use, i.e. int8, float, double
 * @param dataGraph The DataStructure to use
 * @param name The name of the DataArray
 * @param tupleShape  The tuple dimensions of the data. If you want to mimic an image then your shape should be {height, width} slowest to fastest dimension
 * @param componentShape The component dimensions of the data. If you want to mimic an RGB image then your component would be {3},
 * if you want to store a 3Rx4C matrix then it would be {3, 4}.
 * @param parentId The DataObject that will own the DataArray instance.
 * @return
 */
template <typename T>
DataArray<T>* CreateTestDataArray(DataStructure& dataGraph, const std::string& name, typename DataStore<T>::ShapeType tupleShape, typename DataStore<T>::ShapeType componentShape,
                                  DataObject::IdType parentId)
{
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  ArrayType* dataArray = ArrayType::template CreateWithStore<DataStoreType>(dataGraph, name, tupleShape, componentShape, parentId);
  dataArray->fill(static_cast<T>(0.0));
  return dataArray;
}

/**
 * @brief Creates a DataStructure that mimics an EBSD data set
 * @return
 */

static DataStructure CreateDataStructure()
{
  DataStructure dataGraph;
  DataGroup* topLevelGroup = DataGroup::Create(dataGraph, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataGraph, Constants::k_EbsdScanData, topLevelGroup->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, Constants::k_ImageGeometry, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.55f, 1.86});
  imageGeom->setOrigin({0.0f, 20.0f, 66.0f});
  SizeVec3 imageGeomDims = {40, 60, 80};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  size_t numComponents = 1;
  std::vector<size_t> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  Float32Array* ci_data = CreateTestDataArray<float>(dataGraph, ComplexUnitTest::Constants::k_ConfidenceIndex, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = CreateTestDataArray<int32>(dataGraph, "FeatureIds", tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = CreateTestDataArray<int32>(dataGraph, "Phases", tupleShape, {numComponents}, scanData->getId());

  BoolArray* conditionalArray = CreateTestDataArray<bool>(dataGraph, "ConditionalArray", tupleShape, {1}, scanData->getId());
  conditionalArray->fill(true);

  numComponents = 3;
  UInt8Array* ipf_color_data = CreateTestDataArray<uint8_t>(dataGraph, "IPF Colors", tupleShape, {numComponents}, scanData->getId());
  Float32Array* euler_data = CreateTestDataArray<float>(dataGraph, "Euler", tupleShape, {numComponents}, scanData->getId());

  // Add in another group that holds the phase data such as Laue Class, Lattice Constants, etc.
  DataGroup* ensembleGroup = DataGroup::Create(dataGraph, "Phase Data", topLevelGroup->getId());
  numComponents = 1;
  size_t numTuples = 2;
  Int32Array* laue_data = CreateTestDataArray<int32_t>(dataGraph, "Laue Class", {numTuples}, {numComponents}, ensembleGroup->getId());

  return std::move(dataGraph);
}

/**
 * @brief Creates a DataStructure with 2 groups. one group has a DataArray of each primitive type with 1 component and the
 * other group has a DataArray of each primitive type with 3 components.
 * @return
 */
static DataStructure CreateAllPrimitiveTypes(const std::vector<usize>& tupleShape)
{
  DataStructure dataGraph;
  DataGroup* levelZeroGroup = DataGroup::Create(dataGraph, Constants::k_LevelZero);
  // auto levelZeroId = levelZeroGroup->getId();
  DataGroup* levelOneGroup = DataGroup::Create(dataGraph, Constants::k_LevelOne, levelZeroGroup->getId());
  auto levelOneId = levelOneGroup->getId();
  DataGroup* levelTwoGroup = DataGroup::Create(dataGraph, Constants::k_LevelTwo, levelZeroGroup->getId());
  auto levelTwoId = levelTwoGroup->getId();

  //  // Create an Image Geometry grid for the Scan Data
  //  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, Constants::k_ImageGeometry, levelOneGroup->getId());
  //  SizeVec3 imageGeomDims = {40, 60, 80};
  //  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)
  //  imageGeom->setSpacing({0.25f, 0.55f, 1.86});
  //  imageGeom->setOrigin({0.0f, 20.0f, 66.0f});

  // DataStore<size_t>::ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  // Create Scalar type data
  DataStore<size_t>::ShapeType componentShape = {1ULL};

  CreateTestDataArray<int8_t>(dataGraph, ComplexUnitTest::Constants::k_Int8, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint8_t>(dataGraph, ComplexUnitTest::Constants::k_Uint8, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int16_t>(dataGraph, ComplexUnitTest::Constants::k_Int16, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint16_t>(dataGraph, ComplexUnitTest::Constants::k_Uint16, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int32_t>(dataGraph, ComplexUnitTest::Constants::k_Int32, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint32_t>(dataGraph, ComplexUnitTest::Constants::k_Uint32, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int64_t>(dataGraph, ComplexUnitTest::Constants::k_Int64, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint64_t>(dataGraph, ComplexUnitTest::Constants::k_Uint64, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<float>(dataGraph, ComplexUnitTest::Constants::k_Float, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<double>(dataGraph, ComplexUnitTest::Constants::k_Double, tupleShape, componentShape, levelOneId);

  // Create Vector/RGB type of data
  componentShape = {3ULL};
  CreateTestDataArray<int8_t>(dataGraph, ComplexUnitTest::Constants::k_Int8, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint8_t>(dataGraph, ComplexUnitTest::Constants::k_Uint8, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int16_t>(dataGraph, ComplexUnitTest::Constants::k_Int16, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint16_t>(dataGraph, ComplexUnitTest::Constants::k_Uint16, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int32_t>(dataGraph, ComplexUnitTest::Constants::k_Int32, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint32_t>(dataGraph, ComplexUnitTest::Constants::k_Uint32, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int64_t>(dataGraph, ComplexUnitTest::Constants::k_Int64, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint64_t>(dataGraph, ComplexUnitTest::Constants::k_Uint64, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<float>(dataGraph, ComplexUnitTest::Constants::k_Float, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<double>(dataGraph, ComplexUnitTest::Constants::k_Double, tupleShape, componentShape, levelTwoId);

  return std::move(dataGraph);
}

/**
 * @brief Adds an ImageGeometry of the prescribed size to a group in the DataStructure.
 */
static void AddImageGeometry(DataStructure& dataGraph, const SizeVec3& imageGeomDims, const FloatVec3& spacing, const FloatVec3& origin, DataGroup* dataGroup)
{
  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, Constants::k_ImageGeometry, dataGroup->getId());
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)
  imageGeom->setSpacing(spacing);
  imageGeom->setOrigin(origin);
}

} // namespace ComplexUnitTest

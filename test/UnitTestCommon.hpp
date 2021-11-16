#pragma once

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

namespace ComplexUnitTest
{

namespace Constants
{
inline constexpr StringLiteral k_SmallIN100("Small IN100");
inline constexpr StringLiteral k_EbsdScanData("EBSD Scan Data");
inline constexpr StringLiteral k_ImageGeometry("Image Geometry");
inline constexpr StringLiteral k_ConfidenceIndex("Confidence Index");
} // namespace Constants

template <typename T>
DataArray<T>* CreateTestDataArray(const std::string& name, DataStructure& dataGraph, typename DataStore<T>::ShapeType tupleShape, typename DataStore<T>::ShapeType componentShape,
                                  DataObject::IdType parentId)
{
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  DataStoreType* data_store = new DataStoreType(tupleShape, componentShape);
  ArrayType* dataArray = ArrayType::Create(dataGraph, name, data_store, parentId);

  return dataArray;
}

DataStructure CreateDataStructure()
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

  Float32Array* ci_data = CreateTestDataArray<float>(ComplexUnitTest::Constants::k_ConfidenceIndex, dataGraph, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = CreateTestDataArray<int32>("FeatureIds", dataGraph, tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = CreateTestDataArray<int32>("Phases", dataGraph, tupleShape, {numComponents}, scanData->getId());

  numComponents = 3;
  UInt8Array* ipf_color_data = CreateTestDataArray<uint8_t>("IPF Colors", dataGraph, tupleShape, {numComponents}, scanData->getId());
  Float32Array* euler_data = CreateTestDataArray<float>("Euler", dataGraph, tupleShape, {numComponents}, scanData->getId());

  // Add in another group that holds the phase data such as Laue Class, Lattice Constants, etc.
  DataGroup* ensembleGroup = DataGroup::Create(dataGraph, "Phase Data", topLevelGroup->getId());
  numComponents = 1;
  size_t numTuples = 2;
  Int32Array* laue_data = CreateTestDataArray<int32_t>("Laue Class", dataGraph, {numTuples}, {numComponents}, ensembleGroup->getId());

  return dataGraph;
}

} // namespace ComplexUnitTest

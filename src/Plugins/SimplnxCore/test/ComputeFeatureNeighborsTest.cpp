#include "SimplnxCore/Filters/ComputeFeatureNeighborsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// Geometry Level
const std::string k_ImageGeomName = "Image";
const DataPath k_ImageGeomPath = DataPath({k_ImageGeomName});

// Cell Level
const std::string k_CellAMName = "CellData";
const DataPath k_CellAMPath = k_ImageGeomPath.createChildPath(k_CellAMName);
const std::string k_FeatureIdsName = "FeatureIds";
const DataPath k_FeatureIdsPath = k_CellAMPath.createChildPath(k_FeatureIdsName);

const std::string k_BoundaryCellsName = "BoundaryCells";
const DataPath k_BoundaryCellsPath = k_CellAMPath.createChildPath(k_BoundaryCellsName);
const std::string k_ExemplarBoundaryCellsName = "Exemplar" + k_BoundaryCellsName;
const DataPath k_ExemplarBoundaryCellsPath = k_CellAMPath.createChildPath(k_ExemplarBoundaryCellsName);

// Feature Level
const std::string k_FeatureAMName = "FeatureData";
const DataPath k_FeatureAMPath = k_ImageGeomPath.createChildPath(k_FeatureAMName);

// Created Array Names and Paths
const std::string k_SurfaceFeaturesName = "SurfaceFeatures";
const DataPath k_SurfaceFeaturesPath = k_FeatureAMPath.createChildPath(k_SurfaceFeaturesName);
const std::string k_ExemplarSurfaceFeaturesName = "Exemplar" + k_SurfaceFeaturesName;
const DataPath k_ExemplarSurfaceFeaturesPath = k_FeatureAMPath.createChildPath(k_ExemplarSurfaceFeaturesName);

const std::string k_NumNeighborsName = "NumNeighbors";
const DataPath k_NumNeighborsPath = k_FeatureAMPath.createChildPath(k_NumNeighborsName);
const std::string k_ExemplarNumNeighborsName = "Exemplar" + k_NumNeighborsName;
const DataPath k_ExemplarNumNeighborsPath = k_FeatureAMPath.createChildPath(k_ExemplarNumNeighborsName);

const std::string k_NeighborsListName = "NeighborsList";
const DataPath k_NeighborsListPath = k_FeatureAMPath.createChildPath(k_NeighborsListName);
const std::string k_ExemplarNeighborsListName = "Exemplar" + k_NeighborsListName;
const DataPath k_ExemplarNeighborsListPath = k_FeatureAMPath.createChildPath(k_ExemplarNeighborsListName);

const std::string k_SSAListName = "SharedSurfaceAreaList";
const DataPath k_SSAListPath = k_FeatureAMPath.createChildPath(k_SSAListName);
const std::string k_ExemplarSSAListName = "Exemplar" + k_SSAListName;
const DataPath k_ExemplarSSAListPath = k_FeatureAMPath.createChildPath(k_ExemplarSSAListName);

DataStructure CreateSingleVoxelDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.0f, 1.0f, 1.0f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{1, 1, 1}});

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{1, 1, 1}, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{2}, imageGeom->getId());

  featureIds->setValue(0, 1);

  // Output
  Int8Array* exemplarBoundaryCells = Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_ExemplarBoundaryCellsName, cellData->getShape(), ShapeType{1}, cellData->getId());
  exemplarBoundaryCells->setValue(0, 0);

  BoolArray* exemplarSurfaceFeatures = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, k_ExemplarSurfaceFeaturesName, featureData->getShape(), ShapeType{1}, featureData->getId());
  exemplarSurfaceFeatures->setValue(1, true);

  Int32Array* exemplarNumNeighbors = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_ExemplarNumNeighborsName, featureData->getShape(), ShapeType{1}, featureData->getId());
  exemplarNumNeighbors->setValue(1, 0);

  Int32NeighborList* exemplarNeighborsList = Int32NeighborList::Create(dataStructure, k_ExemplarNeighborsListName, featureData->getShape(), featureData->getId());
  exemplarNeighborsList->setList(1, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{}));

  Float32NeighborList* exemplarSSAList = Float32NeighborList::Create(dataStructure, k_ExemplarSSAListName, featureData->getShape(), featureData->getId());
  exemplarSSAList->setList(1, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{}));

  return dataStructure;
}

void Fill1DImage(DataStructure& dataStructure, const ShapeType& imageShape)
{
  auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, imageShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{5}, imageGeom->getId());

  // clang-format off
  const std::array<uint8, 7> featureIdsArray = {
    1, 2, 2, 2, 3, 0, 4
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  // Output
  Int8Array* exemplarBoundaryCells = Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_ExemplarBoundaryCellsName, cellData->getShape(), ShapeType{1}, cellData->getId());
  exemplarBoundaryCells->setValue(0, 1);
  exemplarBoundaryCells->setValue(1, 1);
  exemplarBoundaryCells->setValue(2, 0);
  exemplarBoundaryCells->setValue(3, 1);
  exemplarBoundaryCells->setValue(4, 1);
  exemplarBoundaryCells->setValue(5, 0);
  exemplarBoundaryCells->setValue(6, 0);

  BoolArray* exemplarSurfaceFeatures = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, k_ExemplarSurfaceFeaturesName, featureData->getShape(), ShapeType{1}, featureData->getId());
  exemplarSurfaceFeatures->setValue(1, true);
  exemplarSurfaceFeatures->setValue(2, false);
  exemplarSurfaceFeatures->setValue(3, false);
  exemplarSurfaceFeatures->setValue(4, true);

  Int32Array* exemplarNumNeighbors = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_ExemplarNumNeighborsName, featureData->getShape(), ShapeType{1}, featureData->getId());
  exemplarNumNeighbors->setValue(1, 1);
  exemplarNumNeighbors->setValue(2, 2);
  exemplarNumNeighbors->setValue(3, 1);
  exemplarNumNeighbors->setValue(4, 0);

  Int32NeighborList* exemplarNeighborsList = Int32NeighborList::Create(dataStructure, k_ExemplarNeighborsListName, featureData->getShape(), featureData->getId());
  exemplarNeighborsList->setList(1, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{2}));
  exemplarNeighborsList->setList(2, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{1, 3}));
  exemplarNeighborsList->setList(3, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{2}));
  exemplarNeighborsList->setList(4, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{}));

  Float32NeighborList* exemplarSSAList = Float32NeighborList::Create(dataStructure, k_ExemplarSSAListName, featureData->getShape(), featureData->getId());
  exemplarSSAList->setList(1, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{1.0f}));
  exemplarSSAList->setList(2, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{1.0f, 1.0f}));
  exemplarSSAList->setList(3, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{1.0f}));
  exemplarSSAList->setList(4, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{}));
}

DataStructure Create1DZDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.0f, 1.0f, 2.2f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{1, 1, 7}});

  Fill1DImage(dataStructure, ShapeType{1, 1, 7});

  return dataStructure;
}

DataStructure Create1DYDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.0f, 2.2f, 1.0f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{1, 7, 1}});

  Fill1DImage(dataStructure, ShapeType{1, 7, 1});

  return dataStructure;
}

DataStructure Create1DXDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{2.2f, 1.0f, 1.0f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{7, 1, 1}});

  Fill1DImage(dataStructure, ShapeType{7, 1, 1});

  return dataStructure;
}

void Fill2DImage(DataStructure& dataStructure, const ShapeType& imageShape)
{
  auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, imageShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{6}, imageGeom->getId());

  // clang-format off
  const std::array<uint8, 25> featureIdsArray = {
    1, 2, 2, 2, 1,
    2, 4, 2, 4, 2,
    1, 2, 3, 2, 1,
    2, 4, 2, 4, 0,
    1, 2, 1, 0, 5,
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  // Output
  Int8Array* exemplarBoundaryCells = Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_ExemplarBoundaryCellsName, cellData->getShape(), ShapeType{1}, cellData->getId());
  exemplarBoundaryCells->setValue(0, 2);
  exemplarBoundaryCells->setValue(1, 2);
  exemplarBoundaryCells->setValue(2, 0);
  exemplarBoundaryCells->setValue(3, 2);
  exemplarBoundaryCells->setValue(4, 2);
  exemplarBoundaryCells->setValue(5, 3);
  exemplarBoundaryCells->setValue(6, 4);
  exemplarBoundaryCells->setValue(7, 3);
  exemplarBoundaryCells->setValue(8, 4);
  exemplarBoundaryCells->setValue(9, 3);
  exemplarBoundaryCells->setValue(10, 3);
  exemplarBoundaryCells->setValue(11, 4);
  exemplarBoundaryCells->setValue(12, 4);
  exemplarBoundaryCells->setValue(13, 4);
  exemplarBoundaryCells->setValue(14, 2);
  exemplarBoundaryCells->setValue(15, 3);
  exemplarBoundaryCells->setValue(16, 4);
  exemplarBoundaryCells->setValue(17, 4);
  exemplarBoundaryCells->setValue(18, 2);
  exemplarBoundaryCells->setValue(19, 0);
  exemplarBoundaryCells->setValue(20, 2);
  exemplarBoundaryCells->setValue(21, 3);
  exemplarBoundaryCells->setValue(22, 2);
  exemplarBoundaryCells->setValue(23, 0);
  exemplarBoundaryCells->setValue(24, 0);

  BoolArray* exemplarSurfaceFeatures = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, k_ExemplarSurfaceFeaturesName, featureData->getShape(), ShapeType{1}, featureData->getId());
  exemplarSurfaceFeatures->setValue(1, true);
  exemplarSurfaceFeatures->setValue(2, true);
  exemplarSurfaceFeatures->setValue(3, false);
  exemplarSurfaceFeatures->setValue(4, false);
  exemplarSurfaceFeatures->setValue(5, true);

  Int32Array* exemplarNumNeighbors = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_ExemplarNumNeighborsName, featureData->getShape(), ShapeType{1}, featureData->getId());
  exemplarNumNeighbors->setValue(1, 1);
  exemplarNumNeighbors->setValue(2, 3);
  exemplarNumNeighbors->setValue(3, 1);
  exemplarNumNeighbors->setValue(4, 1);
  exemplarNumNeighbors->setValue(5, 0);

  Int32NeighborList* exemplarNeighborsList = Int32NeighborList::Create(dataStructure, k_ExemplarNeighborsListName, featureData->getShape(), featureData->getId());
  exemplarNeighborsList->setList(1, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{2}));
  exemplarNeighborsList->setList(2, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{1, 3, 4}));
  exemplarNeighborsList->setList(3, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{2}));
  exemplarNeighborsList->setList(4, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{2}));
  exemplarNeighborsList->setList(5, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{}));

  Float32NeighborList* exemplarSSAList = Float32NeighborList::Create(dataStructure, k_ExemplarSSAListName, featureData->getShape(), featureData->getId());
  exemplarSSAList->setList(1, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{22.6f}));
  exemplarSSAList->setList(2, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{22.6f, 6.8f, 23.8f}));
  exemplarSSAList->setList(3, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{6.8f}));
  exemplarSSAList->setList(4, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{23.8f}));
  exemplarSSAList->setList(5, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{}));
}

DataStructure Create2DEmptyZDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{2.2f, 1.2f, 1.0f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{5, 5, 1}});

  Fill2DImage(dataStructure, ShapeType{5, 5, 1});

  return dataStructure;
}

DataStructure Create2DEmptyYDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{2.2f, 1.0f, 1.2f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{5, 1, 5}});

  Fill2DImage(dataStructure, ShapeType{5, 1, 5});

  return dataStructure;
}

DataStructure Create2DEmptyXDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.0f, 2.2f, 1.2f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{1, 5, 5}});

  Fill2DImage(dataStructure, ShapeType{1, 5, 5});

  return dataStructure;
}

DataStructure Create3DDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.8f, 2.2f, 1.2f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{5, 5, 5}});

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{5, 5, 5}, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{7}, imageGeom->getId());

  // clang-format off
  const std::array<uint8, 125> featureIdsArray = {
    5, 5, 0, 2, 2,
    5, 0, 2, 1, 1,
    0, 2, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 0,

    5, 0, 2, 1, 1,
    0, 2, 3, 3, 1,
    2, 2, 2, 1, 1,
    1, 2, 1, 1, 1,
    1, 1, 1, 1, 1,

    0, 1, 1, 1, 1,
    1, 2, 3, 3, 1,
    1, 2, 4, 3, 1,
    1, 2, 3, 1, 1,
    1, 1, 1, 1, 1,

    1, 1, 1, 1, 1,
    1, 4, 4, 4, 1,
    1, 6, 3, 6, 1,
    1, 6, 6, 6, 1,
    1, 1, 1, 1, 1,

    4, 2, 1, 1, 3,
    2, 1, 2, 1, 1,
    1, 2, 3, 4, 1,
    1, 1, 4, 4, 1,
    1, 1, 1, 1, 4
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  // Output
  Int8Array* exemplarBoundaryCells = Int8Array::CreateWithStore<Int8DataStore>(dataStructure, k_ExemplarBoundaryCellsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  // clang-format off
  const std::array<int8, 125> boundaryCellsArray = {
    0, 0, 0, 2, 2,
    0, 0, 3, 3, 1,
    0, 2, 3, 0, 0,
    0, 2, 0, 0, 0,
    0, 0, 0, 0, 0,

    0, 0, 3, 3, 1,
    0, 1, 4, 4, 1,
    2, 0, 5, 3, 0,
    2, 4, 3, 0, 0,
    0, 1, 0, 0, 0,

    0, 1, 2, 1, 0,
    1, 4, 4, 3, 1,
    2, 3, 6, 5, 1,
    1, 4, 6, 3, 0,
    0, 1, 1, 0, 0,

    1, 2, 1, 1, 1,
    2, 5, 4, 5, 1,
    1, 5, 5, 5, 1,
    1, 4, 4, 4, 1,
    0, 1, 1, 1, 1,

    3, 4, 2, 1, 3,
    4, 5, 5, 3, 1,
    2, 5, 4, 4, 1,
    0, 3, 4, 3, 2,
    0, 0, 1, 2, 3
  };
  // clang-format on

  for(usize i = 0; i < exemplarBoundaryCells->getNumberOfTuples(); i++)
  {
    exemplarBoundaryCells->setValue(i, boundaryCellsArray[i]);
  }

  BoolArray* exemplarSurfaceFeatures = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, k_ExemplarSurfaceFeaturesName, featureData->getShape(), ShapeType{1}, featureData->getId());
  exemplarSurfaceFeatures->setValue(1, true);
  exemplarSurfaceFeatures->setValue(2, true);
  exemplarSurfaceFeatures->setValue(3, true);
  exemplarSurfaceFeatures->setValue(4, true);
  exemplarSurfaceFeatures->setValue(5, true);
  exemplarSurfaceFeatures->setValue(6, false);

  Int32Array* exemplarNumNeighbors = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_ExemplarNumNeighborsName, featureData->getShape(), ShapeType{1}, featureData->getId());
  exemplarNumNeighbors->setValue(1, 4);
  exemplarNumNeighbors->setValue(2, 4);
  exemplarNumNeighbors->setValue(3, 4);
  exemplarNumNeighbors->setValue(4, 4);
  exemplarNumNeighbors->setValue(5, 0);
  exemplarNumNeighbors->setValue(6, 4);

  Int32NeighborList* exemplarNeighborsList = Int32NeighborList::Create(dataStructure, k_ExemplarNeighborsListName, featureData->getShape(), featureData->getId());
  exemplarNeighborsList->setList(1, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{2, 3, 4, 6}));
  exemplarNeighborsList->setList(2, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{1, 3, 4, 6}));
  exemplarNeighborsList->setList(3, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{1, 2, 4, 6}));
  exemplarNeighborsList->setList(4, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{1, 2, 3, 6}));
  exemplarNeighborsList->setList(5, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{}));
  exemplarNeighborsList->setList(6, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{1, 2, 3, 4}));

  Float32NeighborList* exemplarSSAList = Float32NeighborList::Create(dataStructure, k_ExemplarSSAListName, featureData->getShape(), featureData->getId());
  exemplarSSAList->setList(1, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{98.88f, 44.16f, 46.80f, 24.96f}));
  exemplarSSAList->setList(2, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{98.88f, 21.00f, 19.32f, 11.88f}));
  exemplarSSAList->setList(3, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{44.16f, 21.00f, 25.80f, 15.36f}));
  exemplarSSAList->setList(4, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{46.80f, 19.32f, 25.80f, 16.20f}));
  exemplarSSAList->setList(5, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{}));
  exemplarSSAList->setList(6, std::make_shared<Float32NeighborList::VectorType>(Float32NeighborList::VectorType{24.96f, 11.88f, 15.36f, 16.20f}));

  return dataStructure;
}

void ExecuteFilter(DataStructure& dataStructure, bool testBoundaryCells, bool testSurfaceFeatures)
{
  ComputeFeatureNeighborsFilter filter;
  Arguments args;

  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_CellFeaturesPath_Key, std::make_any<DataPath>(k_FeatureAMPath));

  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreBoundary_Key, std::make_any<bool>(testBoundaryCells));
  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_BoundaryCellsName_Key, std::make_any<std::string>(k_BoundaryCellsName));

  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreSurface_Key, std::make_any<bool>(testSurfaceFeatures));
  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SurfaceFeaturesName_Key, std::make_any<std::string>(k_SurfaceFeaturesName));

  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NumNeighborsName_Key, std::make_any<std::string>(k_NumNeighborsName));
  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NeighborListName_Key, std::make_any<std::string>(k_NeighborsListName));
  args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SharedSurfaceAreaName_Key, std::make_any<std::string>(k_SSAListName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Output
  if(testBoundaryCells)
  {
    UnitTest::CompareArrays<int8>(dataStructure, k_ExemplarBoundaryCellsPath, k_BoundaryCellsPath);
  }

  if(testSurfaceFeatures)
  {
    UnitTest::CompareArrays<bool>(dataStructure, k_ExemplarSurfaceFeaturesPath, k_SurfaceFeaturesPath);
  }

  UnitTest::CompareArrays<int32>(dataStructure, k_ExemplarNumNeighborsPath, k_NumNeighborsPath);

  UnitTest::CompareNeighborLists<int32>(dataStructure, k_ExemplarNeighborsListPath, k_NeighborsListPath);

  UnitTest::CompareNeighborLists<float32>(dataStructure, k_ExemplarSSAListPath, k_SSAListPath);
}
} // namespace

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.0: Single Voxel - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = CreateSingleVoxelDataStructure();

  ExecuteFilter(dataStructure, true, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.1: Single Voxel - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = CreateSingleVoxelDataStructure();

  ExecuteFilter(dataStructure, false, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.2: Single Voxel - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = CreateSingleVoxelDataStructure();

  ExecuteFilter(dataStructure, true, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.3: Single Voxel - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = CreateSingleVoxelDataStructure();

  ExecuteFilter(dataStructure, false, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.0: 1D Z - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DZDataStructure();

  ExecuteFilter(dataStructure, true, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.1: 1D Z - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DZDataStructure();

  ExecuteFilter(dataStructure, false, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.2: 1D Z - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DZDataStructure();

  ExecuteFilter(dataStructure, true, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.3: 1D Z - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DZDataStructure();

  ExecuteFilter(dataStructure, false, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.0: 1D Y - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DYDataStructure();

  ExecuteFilter(dataStructure, true, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.1: 1D Y - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DYDataStructure();

  ExecuteFilter(dataStructure, false, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.2: 1D Y - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DYDataStructure();

  ExecuteFilter(dataStructure, true, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.3: 1D Y - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DYDataStructure();

  ExecuteFilter(dataStructure, false, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.0: 1D X - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DXDataStructure();

  ExecuteFilter(dataStructure, true, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.1: 1D X - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DXDataStructure();

  ExecuteFilter(dataStructure, false, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.2: 1D X - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DXDataStructure();

  ExecuteFilter(dataStructure, true, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.3: 1D X - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create1DXDataStructure();

  ExecuteFilter(dataStructure, false, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.0: 2D Empty Z - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyZDataStructure();

  ExecuteFilter(dataStructure, true, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.1: 2D Empty Z - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyZDataStructure();

  ExecuteFilter(dataStructure, false, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.2: 2D Empty Z - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyZDataStructure();

  ExecuteFilter(dataStructure, true, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.3: 2D Empty Z - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyZDataStructure();

  ExecuteFilter(dataStructure, false, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.0: 2D Empty Y - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyYDataStructure();

  ExecuteFilter(dataStructure, true, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.1: 2D Empty Y - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyYDataStructure();

  ExecuteFilter(dataStructure, false, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.2: 2D Empty Y - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyYDataStructure();

  ExecuteFilter(dataStructure, true, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.3: 2D Empty Y - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyYDataStructure();

  ExecuteFilter(dataStructure, false, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.0: 2D Empty X - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyXDataStructure();

  ExecuteFilter(dataStructure, true, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.1: 2D Empty X - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyXDataStructure();

  ExecuteFilter(dataStructure, false, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.2: 2D Empty X - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyXDataStructure();

  ExecuteFilter(dataStructure, true, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.3: 2D Empty X - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create2DEmptyXDataStructure();

  ExecuteFilter(dataStructure, false, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.0: 3D - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create3DDataStructure();

  ExecuteFilter(dataStructure, true, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.1: 3D - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create3DDataStructure();

  ExecuteFilter(dataStructure, false, true);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.2: 3D - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create3DDataStructure();

  ExecuteFilter(dataStructure, true, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.3: 3D - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  DataStructure dataStructure = Create3DDataStructure();

  ExecuteFilter(dataStructure, false, false);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Legacy: SmallIn100", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({Constants::k_DataContainer});
  DataPath cellDataAttributeMatrix = smallIn100Group.createChildPath(Constants::k_CellData);
  DataPath featureIdsDataPath({Constants::k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  DataPath cellFeatureAttributeMatrixPath({Constants::k_DataContainer, Constants::k_CellFeatureData});
  std::string numNeighborName = "NumNeighbors_computed";
  std::string neighborListName = "NeighborList_computed";
  std::string sharedSurfaceAreaListName = "SharedSurfaceAreaList_computed";
  std::string boundaryCellsName = "BoundaryCells_computed";
  std::string surfaceFeaturesName = "SurfaceFeatures_computed";

  {
    ComputeFeatureNeighborsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_CellFeaturesPath_Key, std::make_any<DataPath>(cellFeatureAttributeMatrixPath));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreBoundary_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_BoundaryCellsName_Key, std::make_any<std::string>(boundaryCellsName));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreSurface_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SurfaceFeaturesName_Key, std::make_any<std::string>(surfaceFeaturesName));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NumNeighborsName_Key, std::make_any<std::string>(numNeighborName));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NeighborListName_Key, std::make_any<std::string>(neighborListName));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SharedSurfaceAreaName_Key, std::make_any<std::string>(sharedSurfaceAreaListName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Output
  {
    DataPath featureGroup = smallIn100Group.createChildPath(Constants::k_CellFeatureData);
    DataPath exemplaryDataPath = featureGroup.createChildPath("SurfaceFeatures");
    UnitTest::CompareArrays<bool>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(surfaceFeaturesName));

    exemplaryDataPath = featureGroup.createChildPath("NumNeighbors");
    UnitTest::CompareArrays<int32>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(numNeighborName));

    exemplaryDataPath = featureGroup.createChildPath("NeighborList");
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(neighborListName));

    // The exemplar Shared Surface Area is not valid after a bug fix, and the input
    // file is used in other test cases. Other test cases validate SSA functionality.
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighbors_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

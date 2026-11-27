#include "SimplnxCore/Filters/ComputeFeatureNeighborsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// These names and paths define the test geometry hierarchy.
const std::string k_ImageGeomName = "Image";
const DataPath k_ImageGeomPath = DataPath({k_ImageGeomName});

const std::string k_CellAMName = "CellData";
const DataPath k_CellAMPath = k_ImageGeomPath.createChildPath(k_CellAMName);
const std::string k_FeatureIdsName = "FeatureIds";
const DataPath k_FeatureIdsPath = k_CellAMPath.createChildPath(k_FeatureIdsName);

const std::string k_BoundaryCellsName = "BoundaryCells";
const DataPath k_BoundaryCellsPath = k_CellAMPath.createChildPath(k_BoundaryCellsName);
const std::string k_ExemplarBoundaryCellsName = "Exemplar" + k_BoundaryCellsName;
const DataPath k_ExemplarBoundaryCellsPath = k_CellAMPath.createChildPath(k_ExemplarBoundaryCellsName);

const std::string k_FeatureAMName = "FeatureData";
const DataPath k_FeatureAMPath = k_ImageGeomPath.createChildPath(k_FeatureAMName);

// These names and paths select generated and exemplar output arrays.
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

/**
 * @brief Builds a single-feature, single-cell fixture and its expected outputs.
 * @return The populated DataStructure.
 */
DataStructure CreateSingleVoxelDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{1.0f, 1.0f, 1.0f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{1, 1, 1}});

  AttributeMatrix* cellDataPtr = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{1, 1, 1}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);

  Int32ArrayType* featureIdsPtr = Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_FeatureIdsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());

  AttributeMatrix* featureDataPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{2}, imageGeomPtr->getId());

  featureIdsPtr->setValue(0, 1);

  // These arrays encode the complete analytical output for this fixture.
  Int8ArrayType* exemplarBoundaryCellsPtr = Int8ArrayType::CreateWithStore<Int8DataStoreType>(dataStructure, k_ExemplarBoundaryCellsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());
  exemplarBoundaryCellsPtr->setValue(0, 0);

  BoolArrayType* exemplarSurfaceFeaturesPtr =
      BoolArrayType::CreateWithStore<BoolDataStoreType>(dataStructure, k_ExemplarSurfaceFeaturesName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarSurfaceFeaturesPtr->setValue(1, true);

  Int32ArrayType* exemplarNumNeighborsPtr =
      Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_ExemplarNumNeighborsName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarNumNeighborsPtr->setValue(1, 0);

  Int32NeighborListType* exemplarNeighborsListPtr = Int32NeighborListType::Create(dataStructure, k_ExemplarNeighborsListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarNeighborsListPtr->setList(1, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{}));

  Float32NeighborListType* exemplarSsaListPtr = Float32NeighborListType::Create(dataStructure, k_ExemplarSSAListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarSsaListPtr->setList(1, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{}));

  return dataStructure;
}

/**
 * @brief Adds a one-dimensional feature layout and analytical outputs to an ImageGeom.
 * @param dataStructure Contains the ImageGeom and receives the arrays.
 * @param imageShape Cell tuple shape with two axes of size 1.
 */
void Fill1DImage(DataStructure& dataStructure, const ShapeType& imageShape)
{
  auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);

  AttributeMatrix* cellDataPtr = AttributeMatrix::Create(dataStructure, k_CellAMName, imageShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);

  Int32ArrayType* featureIdsPtr = Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_FeatureIdsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());

  AttributeMatrix* featureDataPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{5}, imageGeomPtr->getId());

  // clang-format off
  const std::array<uint8, 7> featureIdsArray = {
    1, 2, 2, 2, 3, 0, 4
  };
  // clang-format on

  for(usize i = 0; i < featureIdsPtr->getNumberOfTuples(); i++)
  {
    featureIdsPtr->setValue(i, featureIdsArray[i]);
  }

  // These arrays encode the complete analytical output for this fixture.
  Int8ArrayType* exemplarBoundaryCellsPtr = Int8ArrayType::CreateWithStore<Int8DataStoreType>(dataStructure, k_ExemplarBoundaryCellsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());
  exemplarBoundaryCellsPtr->setValue(0, 1);
  exemplarBoundaryCellsPtr->setValue(1, 1);
  exemplarBoundaryCellsPtr->setValue(2, 0);
  exemplarBoundaryCellsPtr->setValue(3, 1);
  exemplarBoundaryCellsPtr->setValue(4, 1);
  exemplarBoundaryCellsPtr->setValue(5, 0);
  exemplarBoundaryCellsPtr->setValue(6, 0);

  BoolArrayType* exemplarSurfaceFeaturesPtr =
      BoolArrayType::CreateWithStore<BoolDataStoreType>(dataStructure, k_ExemplarSurfaceFeaturesName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarSurfaceFeaturesPtr->setValue(1, true);
  exemplarSurfaceFeaturesPtr->setValue(2, false);
  exemplarSurfaceFeaturesPtr->setValue(3, false);
  exemplarSurfaceFeaturesPtr->setValue(4, true);

  Int32ArrayType* exemplarNumNeighborsPtr =
      Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_ExemplarNumNeighborsName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarNumNeighborsPtr->setValue(1, 1);
  exemplarNumNeighborsPtr->setValue(2, 2);
  exemplarNumNeighborsPtr->setValue(3, 1);
  exemplarNumNeighborsPtr->setValue(4, 0);

  Int32NeighborListType* exemplarNeighborsListPtr = Int32NeighborListType::Create(dataStructure, k_ExemplarNeighborsListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarNeighborsListPtr->setList(1, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{2}));
  exemplarNeighborsListPtr->setList(2, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{1, 3}));
  exemplarNeighborsListPtr->setList(3, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{2}));
  exemplarNeighborsListPtr->setList(4, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{}));

  Float32NeighborListType* exemplarSsaListPtr = Float32NeighborListType::Create(dataStructure, k_ExemplarSSAListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarSsaListPtr->setList(1, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{1.0f}));
  exemplarSsaListPtr->setList(2, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{1.0f, 1.0f}));
  exemplarSsaListPtr->setList(3, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{1.0f}));
  exemplarSsaListPtr->setList(4, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{}));
}

/**
 * @brief Creates the one-dimensional Z-axis fixture.
 * @return The populated DataStructure.
 */
DataStructure Create1DZDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{1.0f, 1.0f, 2.2f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{1, 1, 7}});

  Fill1DImage(dataStructure, ShapeType{1, 1, 7});

  return dataStructure;
}

/**
 * @brief Creates the one-dimensional Y-axis fixture.
 * @return The populated DataStructure.
 */
DataStructure Create1DYDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{1.0f, 2.2f, 1.0f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{1, 7, 1}});

  Fill1DImage(dataStructure, ShapeType{1, 7, 1});

  return dataStructure;
}

/**
 * @brief Creates the one-dimensional X-axis fixture.
 * @return The populated DataStructure.
 */
DataStructure Create1DXDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{2.2f, 1.0f, 1.0f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{7, 1, 1}});

  Fill1DImage(dataStructure, ShapeType{7, 1, 1});

  return dataStructure;
}

/**
 * @brief Adds a square two-dimensional feature layout and analytical outputs to an ImageGeom.
 * @param dataStructure Contains the ImageGeom and receives the arrays.
 * @param imageShape Cell tuple shape with one axis of size 1.
 */
void Fill2DImage(DataStructure& dataStructure, const ShapeType& imageShape)
{
  auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);

  AttributeMatrix* cellDataPtr = AttributeMatrix::Create(dataStructure, k_CellAMName, imageShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);

  Int32ArrayType* featureIdsPtr = Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_FeatureIdsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());

  AttributeMatrix* featureDataPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{6}, imageGeomPtr->getId());

  // clang-format off
  const std::array<uint8, 25> featureIdsArray = {
    1, 2, 2, 2, 1,
    2, 4, 2, 4, 2,
    1, 2, 3, 2, 1,
    2, 4, 2, 4, 0,
    1, 2, 1, 0, 5,
  };
  // clang-format on

  for(usize i = 0; i < featureIdsPtr->getNumberOfTuples(); i++)
  {
    featureIdsPtr->setValue(i, featureIdsArray[i]);
  }

  // These arrays encode the complete analytical output for this fixture.
  Int8ArrayType* exemplarBoundaryCellsPtr = Int8ArrayType::CreateWithStore<Int8DataStoreType>(dataStructure, k_ExemplarBoundaryCellsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());
  exemplarBoundaryCellsPtr->setValue(0, 2);
  exemplarBoundaryCellsPtr->setValue(1, 2);
  exemplarBoundaryCellsPtr->setValue(2, 0);
  exemplarBoundaryCellsPtr->setValue(3, 2);
  exemplarBoundaryCellsPtr->setValue(4, 2);
  exemplarBoundaryCellsPtr->setValue(5, 3);
  exemplarBoundaryCellsPtr->setValue(6, 4);
  exemplarBoundaryCellsPtr->setValue(7, 3);
  exemplarBoundaryCellsPtr->setValue(8, 4);
  exemplarBoundaryCellsPtr->setValue(9, 3);
  exemplarBoundaryCellsPtr->setValue(10, 3);
  exemplarBoundaryCellsPtr->setValue(11, 4);
  exemplarBoundaryCellsPtr->setValue(12, 4);
  exemplarBoundaryCellsPtr->setValue(13, 4);
  exemplarBoundaryCellsPtr->setValue(14, 2);
  exemplarBoundaryCellsPtr->setValue(15, 3);
  exemplarBoundaryCellsPtr->setValue(16, 4);
  exemplarBoundaryCellsPtr->setValue(17, 4);
  exemplarBoundaryCellsPtr->setValue(18, 2);
  exemplarBoundaryCellsPtr->setValue(19, 0);
  exemplarBoundaryCellsPtr->setValue(20, 2);
  exemplarBoundaryCellsPtr->setValue(21, 3);
  exemplarBoundaryCellsPtr->setValue(22, 2);
  exemplarBoundaryCellsPtr->setValue(23, 0);
  exemplarBoundaryCellsPtr->setValue(24, 0);

  BoolArrayType* exemplarSurfaceFeaturesPtr =
      BoolArrayType::CreateWithStore<BoolDataStoreType>(dataStructure, k_ExemplarSurfaceFeaturesName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarSurfaceFeaturesPtr->setValue(1, true);
  exemplarSurfaceFeaturesPtr->setValue(2, true);
  exemplarSurfaceFeaturesPtr->setValue(3, false);
  exemplarSurfaceFeaturesPtr->setValue(4, false);
  exemplarSurfaceFeaturesPtr->setValue(5, true);

  Int32ArrayType* exemplarNumNeighborsPtr =
      Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_ExemplarNumNeighborsName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarNumNeighborsPtr->setValue(1, 1);
  exemplarNumNeighborsPtr->setValue(2, 3);
  exemplarNumNeighborsPtr->setValue(3, 1);
  exemplarNumNeighborsPtr->setValue(4, 1);
  exemplarNumNeighborsPtr->setValue(5, 0);

  Int32NeighborListType* exemplarNeighborsListPtr = Int32NeighborListType::Create(dataStructure, k_ExemplarNeighborsListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarNeighborsListPtr->setList(1, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{2}));
  exemplarNeighborsListPtr->setList(2, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{1, 3, 4}));
  exemplarNeighborsListPtr->setList(3, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{2}));
  exemplarNeighborsListPtr->setList(4, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{2}));
  exemplarNeighborsListPtr->setList(5, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{}));

  Float32NeighborListType* exemplarSsaListPtr = Float32NeighborListType::Create(dataStructure, k_ExemplarSSAListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarSsaListPtr->setList(1, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{22.6f}));
  exemplarSsaListPtr->setList(2, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{22.6f, 6.8f, 23.8f}));
  exemplarSsaListPtr->setList(3, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{6.8f}));
  exemplarSsaListPtr->setList(4, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{23.8f}));
  exemplarSsaListPtr->setList(5, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{}));
}

/**
 * @brief Creates the square two-dimensional fixture with an empty Z axis.
 * @return The populated DataStructure.
 */
DataStructure Create2DEmptyZDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{2.2f, 1.2f, 1.0f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{5, 5, 1}});

  Fill2DImage(dataStructure, ShapeType{5, 5, 1});

  return dataStructure;
}

/**
 * @brief Creates the square two-dimensional fixture with an empty Y axis.
 * @return The populated DataStructure.
 */
DataStructure Create2DEmptyYDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{2.2f, 1.0f, 1.2f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{5, 1, 5}});

  Fill2DImage(dataStructure, ShapeType{5, 1, 5});

  return dataStructure;
}

/**
 * @brief Creates the square two-dimensional fixture with an empty X axis.
 * @return The populated DataStructure.
 */
DataStructure Create2DEmptyXDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{1.0f, 2.2f, 1.2f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{1, 5, 5}});

  Fill2DImage(dataStructure, ShapeType{1, 5, 5});

  return dataStructure;
}

/**
 * @brief Adds a non-square two-feature layout and analytical outputs to an ImageGeom.
 * @param dataStructure Contains the ImageGeom and receives the arrays.
 * @param imageShape Non-square cell tuple shape with one axis of size 1.
 * @param expectedBoundaryFaceArea Area of one face between the two features.
 *
 * Square fixtures cannot distinguish the two possible row strides. This 3 by
 * 2 layout puts one feature in each row and creates three equal boundary faces.
 */
void FillNonSquare2DFeatures(DataStructure& dataStructure, const ShapeType& imageShape, float32 expectedBoundaryFaceArea)
{
  auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);

  AttributeMatrix* cellDataPtr = AttributeMatrix::Create(dataStructure, k_CellAMName, imageShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);

  Int32ArrayType* featureIdsPtr = Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_FeatureIdsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());
  AttributeMatrix* featureDataPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{3}, imageGeomPtr->getId());

  // The first three indices form feature row 1. The next three form feature row 2.
  const usize totalVoxels = featureIdsPtr->getNumberOfTuples();
  REQUIRE(totalVoxels == 6);
  for(usize i = 0; i < 3; i++)
  {
    featureIdsPtr->setValue(i, 1);
  }
  for(usize i = 3; i < 6; i++)
  {
    featureIdsPtr->setValue(i, 2);
  }

  // Each cell touches the feature interface through exactly one face.
  Int8ArrayType* exemplarBoundaryCellsPtr = Int8ArrayType::CreateWithStore<Int8DataStoreType>(dataStructure, k_ExemplarBoundaryCellsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());
  for(usize i = 0; i < 6; i++)
  {
    exemplarBoundaryCellsPtr->setValue(i, 1);
  }

  // Both features touch the image boundary and are surface features.
  BoolArrayType* exemplarSurfaceFeaturesPtr =
      BoolArrayType::CreateWithStore<BoolDataStoreType>(dataStructure, k_ExemplarSurfaceFeaturesName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarSurfaceFeaturesPtr->setValue(1, true);
  exemplarSurfaceFeaturesPtr->setValue(2, true);

  Int32ArrayType* exemplarNumNeighborsPtr =
      Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_ExemplarNumNeighborsName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarNumNeighborsPtr->setValue(1, 1);
  exemplarNumNeighborsPtr->setValue(2, 1);

  Int32NeighborListType* exemplarNeighborsListPtr = Int32NeighborListType::Create(dataStructure, k_ExemplarNeighborsListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarNeighborsListPtr->setList(1, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{2}));
  exemplarNeighborsListPtr->setList(2, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{1}));

  // Three cells on each side contribute one face, so shared area equals three face areas.
  // An incorrect row stride misses one face and produces only two face areas.
  const float32 expectedSSA = 3.0f * expectedBoundaryFaceArea;
  Float32NeighborListType* exemplarSsaListPtr = Float32NeighborListType::Create(dataStructure, k_ExemplarSSAListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarSsaListPtr->setList(1, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{expectedSSA}));
  exemplarSsaListPtr->setList(2, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{expectedSSA}));
}

/**
 * @brief Creates the non-square two-dimensional fixture with an empty Z axis.
 * @return The populated DataStructure.
 */
DataStructure Create2DNonSquareEmptyZDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  // The correct row stride is dimension 0 with length 3, not dimension 1 with length 2.
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{2.0f, 3.0f, 1.0f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{3, 2, 1}});
  // A Y-normal boundary face has area `spacing[0] * spacing[2]`, which is 2.
  FillNonSquare2DFeatures(dataStructure, ShapeType{3, 2, 1}, 2.0f);
  return dataStructure;
}

/**
 * @brief Creates the non-square two-dimensional fixture with an empty Y axis.
 * @return The populated DataStructure.
 */
DataStructure Create2DNonSquareEmptyYDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{2.0f, 1.0f, 3.0f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{3, 1, 2}});
  // A Z-normal boundary face has area `spacing[0] * spacing[1]`, which is 2.
  FillNonSquare2DFeatures(dataStructure, ShapeType{3, 1, 2}, 2.0f);
  return dataStructure;
}

/**
 * @brief Creates the non-square two-dimensional fixture with an empty X axis.
 * @return The populated DataStructure.
 */
DataStructure Create2DNonSquareEmptyXDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{1.0f, 2.0f, 3.0f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{1, 3, 2}});
  // A Z-normal boundary face has area `spacing[0] * spacing[1]`, which is 2.
  FillNonSquare2DFeatures(dataStructure, ShapeType{1, 3, 2}, 2.0f);
  return dataStructure;
}

/**
 * @brief Creates a three-dimensional feature layout and its analytical outputs.
 * @return The populated DataStructure.
 */
DataStructure Create3DDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{1.8f, 2.2f, 1.2f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(SizeVec3Type{std::array<usize, 3>{5, 5, 5}});

  AttributeMatrix* cellDataPtr = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{5, 5, 5}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);

  Int32ArrayType* featureIdsPtr = Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_FeatureIdsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());

  AttributeMatrix* featureDataPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{7}, imageGeomPtr->getId());

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

  for(usize i = 0; i < featureIdsPtr->getNumberOfTuples(); i++)
  {
    featureIdsPtr->setValue(i, featureIdsArray[i]);
  }

  // These arrays encode the complete analytical output for this fixture.
  Int8ArrayType* exemplarBoundaryCellsPtr = Int8ArrayType::CreateWithStore<Int8DataStoreType>(dataStructure, k_ExemplarBoundaryCellsName, cellDataPtr->getShape(), ShapeType{1}, cellDataPtr->getId());

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

  for(usize i = 0; i < exemplarBoundaryCellsPtr->getNumberOfTuples(); i++)
  {
    exemplarBoundaryCellsPtr->setValue(i, boundaryCellsArray[i]);
  }

  BoolArrayType* exemplarSurfaceFeaturesPtr =
      BoolArrayType::CreateWithStore<BoolDataStoreType>(dataStructure, k_ExemplarSurfaceFeaturesName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarSurfaceFeaturesPtr->setValue(1, true);
  exemplarSurfaceFeaturesPtr->setValue(2, true);
  exemplarSurfaceFeaturesPtr->setValue(3, true);
  exemplarSurfaceFeaturesPtr->setValue(4, true);
  exemplarSurfaceFeaturesPtr->setValue(5, true);
  exemplarSurfaceFeaturesPtr->setValue(6, false);

  Int32ArrayType* exemplarNumNeighborsPtr =
      Int32ArrayType::CreateWithStore<Int32DataStoreType>(dataStructure, k_ExemplarNumNeighborsName, featureDataPtr->getShape(), ShapeType{1}, featureDataPtr->getId());
  exemplarNumNeighborsPtr->setValue(1, 4);
  exemplarNumNeighborsPtr->setValue(2, 4);
  exemplarNumNeighborsPtr->setValue(3, 4);
  exemplarNumNeighborsPtr->setValue(4, 4);
  exemplarNumNeighborsPtr->setValue(5, 0);
  exemplarNumNeighborsPtr->setValue(6, 4);

  Int32NeighborListType* exemplarNeighborsListPtr = Int32NeighborListType::Create(dataStructure, k_ExemplarNeighborsListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarNeighborsListPtr->setList(1, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{2, 3, 4, 6}));
  exemplarNeighborsListPtr->setList(2, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{1, 3, 4, 6}));
  exemplarNeighborsListPtr->setList(3, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{1, 2, 4, 6}));
  exemplarNeighborsListPtr->setList(4, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{1, 2, 3, 6}));
  exemplarNeighborsListPtr->setList(5, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{}));
  exemplarNeighborsListPtr->setList(6, std::make_shared<Int32NeighborListType::VectorType>(Int32NeighborListType::VectorType{1, 2, 3, 4}));

  Float32NeighborListType* exemplarSsaListPtr = Float32NeighborListType::Create(dataStructure, k_ExemplarSSAListName, featureDataPtr->getShape(), featureDataPtr->getId());
  exemplarSsaListPtr->setList(1, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{98.88f, 44.16f, 46.80f, 24.96f}));
  exemplarSsaListPtr->setList(2, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{98.88f, 21.00f, 19.32f, 11.88f}));
  exemplarSsaListPtr->setList(3, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{44.16f, 21.00f, 25.80f, 15.36f}));
  exemplarSsaListPtr->setList(4, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{46.80f, 19.32f, 25.80f, 16.20f}));
  exemplarSsaListPtr->setList(5, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{}));
  exemplarSsaListPtr->setList(6, std::make_shared<Float32NeighborListType::VectorType>(Float32NeighborListType::VectorType{24.96f, 11.88f, 15.36f, 16.20f}));

  return dataStructure;
}

/**
 * @brief Executes ComputeFeatureNeighbors and compares each requested output.
 * @param dataStructure Contains a generated fixture and its exemplar arrays.
 * @param testBoundaryCells True to create and compare BoundaryCells.
 * @param testSurfaceFeatures True to create and compare SurfaceFeatures.
 */
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

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Optional arrays exist only when their related option is enabled.
  if(testBoundaryCells)
  {
    unit_test::CompareArrays<int8>(dataStructure, k_ExemplarBoundaryCellsPath, k_BoundaryCellsPath);
  }

  if(testSurfaceFeatures)
  {
    unit_test::CompareArrays<bool>(dataStructure, k_ExemplarSurfaceFeaturesPath, k_SurfaceFeaturesPath);
  }

  unit_test::CompareArrays<int32>(dataStructure, k_ExemplarNumNeighborsPath, k_NumNeighborsPath);

  unit_test::CompareNeighborLists<int32>(dataStructure, k_ExemplarNeighborsListPath, k_NeighborsListPath);

  unit_test::CompareNeighborLists<float32>(dataStructure, k_ExemplarSSAListPath, k_SSAListPath);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: real HDF5 five-slice analytical oracle", "[SimplnxCore][ComputeFeatureNeighborsFilter][.OocStoreContract]")
{
  unit_test::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const unit_test::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  auto dataStructure = Create3DDataStructure();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(k_FeatureIdsPath));
  auto& featureIds = dataStructure.getDataRefAs<Int32ArrayType>(k_FeatureIdsPath);
  auto store = data_store_utilities::ConvertDataStore<int32>(featureIds.getDataStoreRef(), "HDF5-OOC");
  REQUIRE(store != nullptr);
  auto replaceResult = featureIds.setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  REQUIRE(featureIds.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  // The existing 5x5x5 fixture independently specifies all five outputs and
  // contains feature contacts across each of the rolling 25-cell slices.
  const auto before = GetAlgorithmPathExecutionCounts();
  ExecuteFilter(dataStructure, true, true);
  const auto after = GetAlgorithmPathExecutionCounts();
  REQUIRE(after.out_of_core_on_out_of_core_store == before.out_of_core_on_out_of_core_store + 1);
  REQUIRE(after.in_core == before.in_core);
  for(const auto& path : {k_BoundaryCellsPath, k_SurfaceFeaturesPath, k_NumNeighborsPath})
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(path));
    REQUIRE(dataStructure.getDataRefAs<IDataArray>(path).getIDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32NeighborListType>(k_NeighborsListPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32NeighborListType>(k_SSAListPath));
  REQUIRE(dataStructure.getDataRefAs<Int32NeighborListType>(k_NeighborsListPath).getStore()->isOutOfCore());
  REQUIRE(dataStructure.getDataRefAs<Float32NeighborListType>(k_SSAListPath).getStore()->isOutOfCore());
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.0: Single Voxel - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateSingleVoxelDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.1: Single Voxel - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateSingleVoxelDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.2: Single Voxel - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateSingleVoxelDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.3: Single Voxel - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateSingleVoxelDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.0: 1D Z - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DZDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.1: 1D Z - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DZDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.2: 1D Z - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DZDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.3: 1D Z - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DZDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.0: 1D Y - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DYDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.1: 1D Y - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DYDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.2: 1D Y - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DYDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.3: 1D Y - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DYDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.0: 1D X - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DXDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.1: 1D X - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DXDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.2: 1D X - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DXDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.3: 1D X - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create1DXDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.0: 2D Empty Z - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyZDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.1: 2D Empty Z - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyZDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.2: 2D Empty Z - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyZDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.3: 2D Empty Z - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyZDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.0: 2D Empty Y - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyYDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.1: 2D Empty Y - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyYDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.2: 2D Empty Y - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyYDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.3: 2D Empty Y - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyYDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.0: 2D Empty X - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyXDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.1: 2D Empty X - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyXDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.2: 2D Empty X - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyXDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.3: 2D Empty X - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create2DEmptyXDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.0: 3D - Full Execution", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create3DDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.1: 3D - No Boundary", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create3DDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, true); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.2: 3D - No Surface Features", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create3DDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, true, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.3: 3D - No Optionals", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Create3DDataStructure();

  scope.execute([&] { ExecuteFilter(dataStructure, false, false); });

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// Square fixtures cannot detect an exchange of the two nonempty dimensions.
// These non-square cases require the correct row stride in each Empty2D dispatch.
// An incorrect stride misses a boundary face or accesses beyond the feature buffer.
TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.4: 2D Empty Z - Non-Square {3,2,1}", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = Create2DNonSquareEmptyZDataStructure();
  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.4: 2D Empty Y - Non-Square {3,1,2}", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = Create2DNonSquareEmptyYDataStructure();
  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.4: 2D Empty X - Non-Square {1,3,2}", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = Create2DNonSquareEmptyXDataStructure();
  scope.execute([&] { ExecuteFilter(dataStructure, true, true); });
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Legacy: SmallIn100", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");
  // Load the Small IN100 input before computing neighbor arrays.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({constants::k_DataContainer});
  DataPath cellDataAttributeMatrix = smallIn100Group.createChildPath(constants::k_CellData);
  DataPath featureIdsDataPath({constants::k_DataContainer, constants::k_CellData, constants::k_FeatureIds});
  DataPath cellFeatureAttributeMatrixPath({constants::k_DataContainer, constants::k_CellFeatureData});
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
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the generated arrays that have valid exemplar values.
  {
    DataPath featureGroup = smallIn100Group.createChildPath(constants::k_CellFeatureData);
    DataPath exemplaryDataPath = featureGroup.createChildPath("SurfaceFeatures");
    unit_test::CompareArrays<bool>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(surfaceFeaturesName));

    exemplaryDataPath = featureGroup.createChildPath("NumNeighbors");
    unit_test::CompareArrays<int32>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(numNeighborName));

    exemplaryDataPath = featureGroup.createChildPath("NeighborList");
    unit_test::CompareNeighborLists<int32>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(neighborListName));

    // This shared input has a stale shared-area exemplar, so this case omits that array.
    // Generated fixtures in this file validate the shared-area calculation independently.
  }

// The optional output supports manual inspection of the generated neighbor arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighbors_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeatureNeighborsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureNeighborsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureNeighborsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<ComputeFeatureNeighborsFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<bool>(ComputeFeatureNeighborsFilter::k_StoreBoundary_Key) == true);
      CHECK(args.value<bool>(ComputeFeatureNeighborsFilter::k_StoreSurface_Key) == true);
      CHECK(args.value<DataPath>(ComputeFeatureNeighborsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeFeatureNeighborsFilter::k_FeatureIdsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureNeighborsFilter::k_CellFeaturesPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeFeatureNeighborsFilter::k_BoundaryCellsName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureNeighborsFilter::k_NumNeighborsName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureNeighborsFilter::k_NeighborListName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureNeighborsFilter::k_SharedSurfaceAreaName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureNeighborsFilter::k_SurfaceFeaturesName_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Geometry cell tuple count", "[SimplnxCore][ComputeFeatureNeighborsFilter][TupleCount]")
{
  using FilterType = ComputeFeatureNeighborsFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 3, 2}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // Matching arrays remain valid outside the geometry's Cell Attribute Matrix, with a different tuple shape.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 0), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_FeatureIdsPath_Key, std::make_any<DataPath>(inputPath));
  REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{2}, imageGeomPtr->getId()) != nullptr);
  args.insertOrAssign(FilterType::k_CellFeaturesPath_Key, std::make_any<DataPath>(DataPath({"Image", "Feature Data"})));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    const auto& errorRef = result.output_actions.errors()[0];
    REQUIRE(errorRef.code == -24501);
    REQUIRE(errorRef.message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", tuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}

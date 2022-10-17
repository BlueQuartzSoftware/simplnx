#pragma once

#include "complex/Common/Result.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/IDataStore.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <algorithm>

namespace fs = std::filesystem;

#define COMPLEX_RESULT_CATCH_PRINT(result)                                                                                                                                                             \
  for(const auto& warning : result.warnings())                                                                                                                                                         \
  {                                                                                                                                                                                                    \
    WARN(fmt::format("{} : {}", warning.code, warning.message));                                                                                                                                       \
  }                                                                                                                                                                                                    \
  if(result.invalid())                                                                                                                                                                                 \
  {                                                                                                                                                                                                    \
    for(const auto& error : result.errors())                                                                                                                                                           \
    {                                                                                                                                                                                                  \
      UNSCOPED_INFO(fmt::format("{} : {}", error.code, error.message));                                                                                                                                \
    }                                                                                                                                                                                                  \
  }

#define COMPLEX_RESULT_REQUIRE_VALID(result)                                                                                                                                                           \
  COMPLEX_RESULT_CATCH_PRINT(result);                                                                                                                                                                  \
  REQUIRE(result.valid());

#define COMPLEX_RESULT_REQUIRE_INVALID(result)                                                                                                                                                         \
  COMPLEX_RESULT_CATCH_PRINT(result);                                                                                                                                                                  \
  REQUIRE(result.invalid());

namespace complex
{
namespace Constants
{

inline constexpr StringLiteral k_DataContainer("DataContainer");
inline constexpr StringLiteral k_CellData("CellData");
inline constexpr StringLiteral k_CellFeatureData("CellFeatureData");

inline constexpr StringLiteral k_SmallIN1002("Small IN1002");
inline constexpr StringLiteral k_SmallIN100("Small IN100");
inline constexpr StringLiteral k_EbsdScanData("EBSD Scan Data");
inline constexpr StringLiteral k_ImageGeometry("Image Geometry");
inline constexpr StringLiteral k_VertexGeometry("Vertex Geometry");
inline constexpr StringLiteral k_Confidence_Index("Confidence Index");
inline constexpr StringLiteral k_ConfidenceIndex("ConfidenceIndex");

inline constexpr StringLiteral k_EulerAngles("EulerAngles");
inline constexpr StringLiteral k_AxisAngles("AxisAngles");
inline constexpr StringLiteral k_Quats("Quats");
inline constexpr StringLiteral k_FZQuats("FZQuats");
inline constexpr StringLiteral k_FeatureGroupName("Feature Data");
inline constexpr StringLiteral k_ActiveName("Active");
inline constexpr StringLiteral k_SlipVector("SlipVector");

inline constexpr StringLiteral k_FeatureIds("FeatureIds");
inline constexpr StringLiteral k_Image_Quality("Image Quality");
inline constexpr StringLiteral k_ImageQuality("ImageQuality");
inline constexpr StringLiteral k_Phases("Phases");
inline constexpr StringLiteral k_Ipf_Colors("IPF Colors");
inline constexpr StringLiteral k_IPFColors("IPFColors");
inline constexpr StringLiteral k_PhaseData("Phase Data");
inline constexpr StringLiteral k_LaueClass("Laue Class");
inline constexpr StringLiteral k_SmallIn100ImageGeom("[Image Geometry]");

inline constexpr StringLiteral k_TriangleGeometryName("[Triangle Geometry]");
inline constexpr StringLiteral k_VertexDataGroupName("Vertex Data");
inline constexpr StringLiteral k_NodeTypeArrayName("Node Type");
inline constexpr StringLiteral k_FaceDataGroupName("Face Data");
inline constexpr StringLiteral k_FaceLabels("Face Labels");
inline constexpr StringLiteral k_NormalsLabels("Normals");
inline constexpr StringLiteral k_TriangleAreas("Triangle Areas");

inline constexpr StringLiteral k_LevelZero("ZERO");
inline constexpr StringLiteral k_LevelOne("ONE");
inline constexpr StringLiteral k_LevelTwo("TWO");

inline constexpr StringLiteral k_Int8DataSet("int8 DataSet");
inline constexpr StringLiteral k_Uint8DataSet("uint8 DataSet");

inline constexpr StringLiteral k_Int16DataSet("int16 DataSet");
inline constexpr StringLiteral k_Uint16DataSet("uint16 DataSet");

inline constexpr StringLiteral k_Int32DataSet("int32 DataSet");
inline constexpr StringLiteral k_Uint32DataSet("uint32 DataSet");

inline constexpr StringLiteral k_Int64DataSet("int64 DataSet");
inline constexpr StringLiteral k_Uint64DataSet("uint64 DataSet");

inline constexpr StringLiteral k_Float32DataSet("float32 DataSet");
inline constexpr StringLiteral k_Float64DataSet("float64 DataSet");

inline constexpr StringLiteral k_ConditionalArray("Conditional [bool]");
inline constexpr StringLiteral k_ReducedGeometry("Reduced Geometry");
} // namespace Constants

namespace UnitTest
{

inline constexpr float EPSILON = 0.0001;

/**
 * @brief Loads a .dream3d file into a DataStructure. Checks are made to ensure the filepath does exist
 * @param filepath
 * @return DataStructure
 */
inline DataStructure LoadDataStructure(const fs::path& filepath)
{
  DataStructure exemplarDataStructure;
  INFO(fmt::format("Error loading file: '{}'  ", filepath.string()));
  REQUIRE(fs::exists(filepath));
  auto result = DREAM3D::ImportDataStructureFromFile(filepath);
  COMPLEX_RESULT_REQUIRE_VALID(result);
  return result.value();
}

/**
 * @brief Writes out a DataStructure to a .dream3d file at the given file path
 * @param dataStructure
 * @param filepath
 */
inline void WriteTestDataStructure(const DataStructure& dataStructure, const fs::path& filepath)
{
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filepath);
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataStructure.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

/**
 * @brief Compares IDataArray
 * @tparam T
 * @param left
 * @param right
 */
template <typename T>
void CompareDataArrays(const IDataArray& left, const IDataArray& right, usize start = 0)
{
  const auto& oldDataStore = left.getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto& newDataStore = right.getIDataStoreRefAs<AbstractDataStore<T>>();
  usize end = oldDataStore.getSize();
  INFO(fmt::format("Input Data Array:'{}'  Output DataArray: '{}' bad comparison", left.getName(), right.getName()));
  T oldVal;
  T newVal;
  bool failed = false;
  for(usize i = start; i < end; i++)
  {
    oldVal = oldDataStore[i];
    newVal = newDataStore[i];
    if(oldVal != newVal)
    {
      UNSCOPED_INFO(fmt::format("oldValue != newValue. {} != {}", oldVal, newVal));

      if constexpr(std::is_floating_point_v<T>)
      {
        float diff = std::fabs(static_cast<float>(oldVal - newVal));
        if(diff > EPSILON)
        {
          failed = true;
          break;
        }
      }
      else
      {
        failed = true;
      }
      break;
    }
  }
  REQUIRE(!failed);
}

/**
 * @brief Compares 2 DataArrays using an EPSILON value. Useful for floating point comparisons
 * @tparam T
 * @param dataStructure
 * @param exemplaryDataPath
 * @param computedPath
 */
template <typename T>
void CompareArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
  // DataPath exemplaryDataPath = featureGroup.createChildPath("SurfaceFeatures");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(computedPath));

  const auto& exemplaryDataArray = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);
  const auto& generatedDataArray = dataStructure.getDataRefAs<DataArray<T>>(computedPath);
  REQUIRE(generatedDataArray.getNumberOfTuples() == exemplaryDataArray.getNumberOfTuples());

  INFO(fmt::format("Input Data Array:'{}'  Output DataArray: '{}' bad comparison", exemplaryDataPath.toString(), computedPath.toString()));

  usize start = 0;
  usize end = exemplaryDataArray.getSize();
  for(usize i = start; i < end; i++)
  {
    auto oldVal = exemplaryDataArray[i];
    auto newVal = generatedDataArray[i];
    if(oldVal != newVal)
    {
      float diff = std::fabs(static_cast<float>(oldVal - newVal));
      REQUIRE(diff < EPSILON);
      break;
    }
  }
}

/**
 * @brief
 * @tparam T
 * @param dataStructure
 * @param exemplaryDataPath
 * @param computedPath
 */
template <typename T>
void CompareNeighborLists(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
  // DataPath exemplaryDataPath = featureGroup.createChildPath("SurfaceFeatures");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(computedPath));

  const auto& exemplaryList = dataStructure.getDataRefAs<NeighborList<T>>(exemplaryDataPath);
  const auto& computedNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(computedPath);
  REQUIRE(computedNeighborList.getNumberOfTuples() == exemplaryList.getNumberOfTuples());

  for(usize i = 0; i < exemplaryList.getNumberOfTuples(); i++)
  {
    const auto exemplary = exemplaryList.getList(i);
    const auto computed = computedNeighborList.getList(i);
    if(exemplary.get() != nullptr && computed.get() != nullptr)
    {
      REQUIRE(exemplary->size() == computed->size());
      std::sort(exemplary->begin(), exemplary->end());
      std::sort(computed->begin(), computed->end());
      for(usize j = 0; j < exemplary->size(); ++j)
      {
        auto exemplaryVal = exemplary->at(j);
        auto computedVal = computed->at(j);
        if(exemplaryVal != computedVal)
        {
          float diff = std::fabs(static_cast<float>(exemplaryVal - computedVal));
          INFO(fmt::format("Bad Neighborlist Comparison\n  Exemplary NeighborList:'{}'  size:{}\n  Computed NeighborList: '{}' size:{} ", exemplaryDataPath.toString(), exemplary->size(),
                           computedPath.toString(), computed->size()));
          INFO(fmt::format("  NeighborList {}, Index {} Exemplary Value: {} Computed Value: {}", i, j, exemplaryVal, computedVal))

          REQUIRE(diff < EPSILON);
          break;
        }
      }
    }
  }
}

inline void CompareStringArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
  // DataPath exemplaryDataPath = featureGroup.createChildPath("SurfaceFeatures");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(computedPath));

  const auto& exemplaryDataArray = dataStructure.getDataRefAs<StringArray>(exemplaryDataPath);
  const auto& generatedDataArray = dataStructure.getDataRefAs<StringArray>(computedPath);
  REQUIRE(generatedDataArray.getNumberOfTuples() == exemplaryDataArray.getNumberOfTuples());

  INFO(fmt::format("Input Data Array:'{}'  Output StringArray: '{}' bad comparison", exemplaryDataPath.toString(), computedPath.toString()));

  usize start = 0;
  usize end = exemplaryDataArray.getSize();
  for(usize i = start; i < end; i++)
  {
    auto oldVal = exemplaryDataArray[i];
    auto newVal = generatedDataArray[i];
    REQUIRE(oldVal == newVal);
  }
}

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
                                  DataObject::IdType parentId = {})
{
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  ArrayType* dataArray = ArrayType::template CreateWithStore<DataStoreType>(dataGraph, name, tupleShape, componentShape, parentId);
  dataArray->fill(static_cast<T>(0.0));
  return dataArray;
}

template <typename T>
NeighborList<T>* CreateTestNeighborList(DataStructure& dataGraph, const std::string& name, usize numTuples, DataObject::IdType parentId)
{
  using NeighborListType = NeighborList<T>;
  auto* neighborList = NeighborListType::Create(dataGraph, name, numTuples, parentId);
  neighborList->resizeTotalElements(numTuples);
  return neighborList;
}

/**
 * @brief Creates a DataStructure that mimics an EBSD data set
 * @return
 */
inline DataStructure CreateDataStructure()
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
  usize numComponents = 1;
  std::vector<usize> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  Float32Array* ci_data = CreateTestDataArray<float>(dataGraph, Constants::k_ConfidenceIndex, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = CreateTestDataArray<int32>(dataGraph, Constants::k_FeatureIds, tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = CreateTestDataArray<int32>(dataGraph, "Phases", tupleShape, {numComponents}, scanData->getId());
  USizeArray* voxelIndices = CreateTestDataArray<usize>(dataGraph, "Voxel Indices", tupleShape, {numComponents}, scanData->getId());

  BoolArray* conditionalArray = CreateTestDataArray<bool>(dataGraph, Constants::k_ConditionalArray, tupleShape, {1}, scanData->getId());
  conditionalArray->fill(true);

  numComponents = 3;
  UInt8Array* ipf_color_data = CreateTestDataArray<uint8>(dataGraph, "IPF Colors", tupleShape, {numComponents}, scanData->getId());
  Float32Array* euler_data = CreateTestDataArray<float>(dataGraph, "Euler", tupleShape, {numComponents}, scanData->getId());

  // Add in another group that holds the phase data such as Laue Class, Lattice Constants, etc.
  DataGroup* ensembleGroup = DataGroup::Create(dataGraph, "Phase Data", topLevelGroup->getId());
  numComponents = 1;
  usize numTuples = 2;
  Int32Array* laue_data = CreateTestDataArray<int32>(dataGraph, "Laue Class", {numTuples}, {numComponents}, ensembleGroup->getId());

  // Create a Vertex Geometry grid for the Scan Data
  VertexGeom* vertexGeom = VertexGeom::Create(dataGraph, Constants::k_VertexGeometry, scanData->getId());
  vertexGeom->setVertices(*euler_data);

  // NeighborList<float32>* neighborList = CreateTestNeighborList<float32>(dataGraph, "Neighbor List", numTuples, scanData->getId());

  return dataGraph;
}

/**
 * @brief Creates a DataStructure with 2 groups. one group has a DataArray of each primitive type with 1 component and the
 * other group has a DataArray of each primitive type with 3 components.
 * @return
 */
inline DataStructure CreateAllPrimitiveTypes(const std::vector<usize>& tupleShape)
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

  // DataStore<usize>::ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  // Create Scalar type data
  DataStore<usize>::ShapeType componentShape = {1ULL};

  CreateTestDataArray<int8>(dataGraph, Constants::k_Int8DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint8>(dataGraph, Constants::k_Uint8DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int16>(dataGraph, Constants::k_Int16DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint16>(dataGraph, Constants::k_Uint16DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int32>(dataGraph, Constants::k_Int32DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint32>(dataGraph, Constants::k_Uint32DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int64>(dataGraph, Constants::k_Int64DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint64>(dataGraph, Constants::k_Uint64DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<float32>(dataGraph, Constants::k_Float32DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<float64>(dataGraph, Constants::k_Float64DataSet, tupleShape, componentShape, levelOneId);

  // Create Vector/RGB type of data
  componentShape = {3ULL};
  CreateTestDataArray<int8>(dataGraph, Constants::k_Int8DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint8>(dataGraph, Constants::k_Uint8DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int16>(dataGraph, Constants::k_Int16DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint16>(dataGraph, Constants::k_Uint16DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int32>(dataGraph, Constants::k_Int32DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint32>(dataGraph, Constants::k_Uint32DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int64>(dataGraph, Constants::k_Int64DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint64>(dataGraph, Constants::k_Uint64DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<float32>(dataGraph, Constants::k_Float32DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<float64>(dataGraph, Constants::k_Float64DataSet, tupleShape, componentShape, levelTwoId);

  return dataGraph;
}

/**
 * @brief Adds an ImageGeometry of the prescribed size to a group in the DataStructure.
 */
inline void AddImageGeometry(DataStructure& dataGraph, const SizeVec3& imageGeomDims, const FloatVec3& spacing, const FloatVec3& origin, const DataGroup& dataGroup)
{
  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, Constants::k_ImageGeometry, dataGroup.getId());
  if(imageGeom == nullptr)
  {
    throw std::runtime_error("UnitTestCommon: Unable to create ImageGeom");
  }
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)
  imageGeom->setSpacing(spacing);
  imageGeom->setOrigin(origin);
}
} // namespace UnitTest
} // namespace complex

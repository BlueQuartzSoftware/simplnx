#pragma once

#include "complex/Common/Result.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/IDataStore.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <reproc++/run.hpp>

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

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
inline constexpr StringLiteral k_Cell_Data("Cell Data");
inline constexpr StringLiteral k_GrainData("GrainData");
inline constexpr StringLiteral k_Grain_Data("Grain Data");
inline constexpr StringLiteral k_FeatureData("FeatureData");
inline constexpr StringLiteral k_CellFeatureData("CellFeatureData");
inline constexpr StringLiteral k_CellEnsembleData("CellEnsembleData");
inline constexpr StringLiteral k_Phase_Data("Phase Data");

inline constexpr StringLiteral k_TriangleDataContainerName("TriangleDataContainer");
inline constexpr StringLiteral k_VertexDataContainerName("VertexDataContainer");
inline constexpr StringLiteral k_PointCloudContainerName("Point Cloud");
inline constexpr StringLiteral k_FaceData("FaceData");
inline constexpr StringLiteral k_Face_Data("Face Data");
inline constexpr StringLiteral k_FaceFeatureData("FaceFeatureData");
inline constexpr StringLiteral k_VertexData("VertexData");
inline constexpr StringLiteral k_GBCD_Name("GBCD");

inline constexpr StringLiteral k_Centroids("Centroids");
inline constexpr StringLiteral k_EnsembleAttributeMatrix("CellEnsembleData");
inline constexpr StringLiteral k_ExemplarDataContainer("Exemplar Data");
inline constexpr StringLiteral k_CrystalStructures("CrystalStructures");
inline constexpr StringLiteral k_Fit("Fit");
inline constexpr StringLiteral k_SEMSignal("SEM Signal");

inline constexpr StringLiteral k_SmallIN1002("Small IN100 2D");
inline constexpr StringLiteral k_SmallIN100("Small IN100");
inline constexpr StringLiteral k_EbsdScanData("EBSD Scan Data");
inline constexpr StringLiteral k_ImageGeometry("Image Geometry");
inline constexpr StringLiteral k_VertexGeometry("Vertex Geometry");
inline constexpr StringLiteral k_Confidence_Index("Confidence Index");
inline constexpr StringLiteral k_ConfidenceIndex("ConfidenceIndex");
inline constexpr StringLiteral k_CalculatedShifts("Calculated Shifts");
inline constexpr StringLiteral k_FaceLabels("FaceLabels");
inline constexpr StringLiteral k_FaceNormals("FaceNormals");
inline constexpr StringLiteral k_FaceAreas("FaceAreas");

inline constexpr StringLiteral k_EulerAngles("EulerAngles");
inline constexpr StringLiteral k_AxisAngles("AxisAngles");
inline constexpr StringLiteral k_AvgQuats("AvgQuats");
inline constexpr StringLiteral k_Quats("Quats");
inline constexpr StringLiteral k_Mask("Mask");
inline constexpr StringLiteral k_FZQuats("FZQuats");
inline constexpr StringLiteral k_FeatureGroupName("Feature Data");
inline constexpr StringLiteral k_ActiveName("Active");
inline constexpr StringLiteral k_NumElements("NumElements");
inline constexpr StringLiteral k_SlipVector("SlipVector");
inline constexpr StringLiteral k_AvgEulerAngles("AvgEulerAngles");
inline constexpr StringLiteral k_SurfaceFeatures("SurfaceFeatures");
inline constexpr StringLiteral k_RectCoords("RectCoords");
inline constexpr StringLiteral k_Omega1("Omega1");
inline constexpr StringLiteral k_Omega2("Omega2");
inline constexpr StringLiteral k_CentralMoments("CentralMoments");

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
inline constexpr StringLiteral k_Face_Labels("Face Labels");
inline constexpr StringLiteral k_NormalsLabels("Normals");
inline constexpr StringLiteral k_TriangleAreas("Triangle Areas");
inline constexpr StringLiteral k_VoxelIndices = "VoxelIndices";
inline constexpr StringLiteral k_NodeType("NodeType");

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

inline constexpr StringLiteral k_GroupAName("A");
inline constexpr StringLiteral k_GroupBName("B");
inline constexpr StringLiteral k_GroupCName("C");
inline constexpr StringLiteral k_GroupDName("D");
inline constexpr StringLiteral k_GroupEName("E");
inline constexpr StringLiteral k_GroupFName("F");
inline constexpr StringLiteral k_GroupGName("G");
inline constexpr StringLiteral k_GroupHName("H");
inline constexpr StringLiteral k_ArrayIName("I");
inline constexpr StringLiteral k_ArrayJName("J");
inline constexpr StringLiteral k_ArrayKName("K");
inline constexpr StringLiteral k_ArrayLName("L");
inline constexpr StringLiteral k_ArrayMName("M");
inline constexpr StringLiteral k_ArrayNName("N");

// Data Container DataPath
const DataPath k_DataContainerPath({k_DataContainer});

// Cell Attribute Matrix DataPaths
const DataPath k_CellAttributeMatrix = k_DataContainerPath.createChildPath(k_CellData);
const DataPath k_EulersArrayPath = k_CellAttributeMatrix.createChildPath(k_EulerAngles);
const DataPath k_QuatsArrayPath = k_CellAttributeMatrix.createChildPath(k_Quats);
const DataPath k_PhasesArrayPath = k_CellAttributeMatrix.createChildPath(k_Phases);
const DataPath k_FeatureIdsArrayPath = k_CellAttributeMatrix.createChildPath(k_FeatureIds);
const DataPath k_ConfidenceIndexArrayPath = k_CellAttributeMatrix.createChildPath(k_Confidence_Index);
const DataPath k_ImageQualityArrayPath = k_CellAttributeMatrix.createChildPath(k_Image_Quality);
const DataPath k_MaskArrayPath = k_CellAttributeMatrix.createChildPath(k_Mask);
const DataPath k_FitArrayPath = k_CellAttributeMatrix.createChildPath(k_Fit);
const DataPath k_SEMSignalArrayPath = k_CellAttributeMatrix.createChildPath(k_SEMSignal);

// Cell Ensemble Data DataPaths
const DataPath k_CellEnsembleAttributeMatrixPath = k_DataContainerPath.createChildPath(k_EnsembleAttributeMatrix);
const DataPath k_CrystalStructuresArrayPath = k_CellEnsembleAttributeMatrixPath.createChildPath(k_CrystalStructures);
const DataPath k_CalculatedShiftsPath = k_DataContainerPath.createChildPath(k_CalculatedShifts);

// Cell Feature Attribute Matrix DataPaths
const DataPath k_CellFeatureAttributeMatrix = k_DataContainerPath.createChildPath(k_Grain_Data);
const DataPath k_ActiveArrayPath = k_CellFeatureAttributeMatrix.createChildPath(k_ActiveName);
const DataPath k_NumCellsPath = k_CellFeatureAttributeMatrix.createChildPath(k_NumElements);
const DataPath k_FeaturePhasesPath = k_CellFeatureAttributeMatrix.createChildPath(k_Phases);

const DataPath k_CellFeatureDataPath = k_DataContainerPath.createChildPath(k_CellFeatureData);

// Exemplar DataStructure
const DataPath k_ExemplarDataContainerPath({k_ExemplarDataContainer});

} // namespace Constants

namespace UnitTest
{
inline constexpr float EPSILON = 0.0001;

struct make_shared_enabler : public complex::Application
{
};

/**
 * @brief This class will decompress a tar.gz file using the locally installed copy of cmake and when
 * then class goes out of scope the extracted contents will be deleted from disk.
 */
class TestFileSentinel
{
public:
  /**
   * @brief Construct a File Sentinel object that will decompress on construction and remove the
   * contents on destruction.
   *
   * This class uses the actual CMake executable to do the decompression and the removal.
   *
   * @param cmakeExecutable The absolute path to the cmake(.exe) executable.
   * @param testFilesDir The directory where the archive is located
   * @param inputArchiveName The full name of the archive. The location is assumed to be in the TestFiles directory
   * @param expectedTopLevelOutput The name of the decompressed folder or file. WARNING: This assumes
   * that only a single file or single directory are part of the archive. In the case of a directory, the
   * directory itself can have as many subdirectories as needed.
   */
  TestFileSentinel(std::string cmakeExecutable, std::string testFilesDir, std::string inputArchiveName, std::string expectedTopLevelOutput)
  : m_CMakeExecutable(std::move(cmakeExecutable))
  , m_TestFilesDir(std::move(testFilesDir))
  , m_InputArchiveName(std::move(inputArchiveName))
  , m_ExpectedTopLevelOutput(std::move(expectedTopLevelOutput))
  {
    const auto result = decompress();
    REQUIRE(result);
  }

  ~TestFileSentinel()
  {
    std::error_code errorCode;
    std::filesystem::remove_all(fmt::format("{}/{}", m_TestFilesDir, m_ExpectedTopLevelOutput), errorCode);
    if(errorCode)
    {
      std::cout << "Removing decompressed data failed: " << errorCode.message() << std::endl;
    }
  }

  TestFileSentinel(const TestFileSentinel&) = delete;            // Copy Constructor Not Implemented
  TestFileSentinel(TestFileSentinel&&) = delete;                 // Move Constructor Not Implemented
  TestFileSentinel& operator=(const TestFileSentinel&) = delete; // Copy Assignment Not Implemented
  TestFileSentinel& operator=(TestFileSentinel&&) = delete;      // Move Assignment Not Implemented

  /**
   * @brief Does the actual decompression of the archive.
   * @return
   */
  bool decompress()
  {
    reproc::options options;
    options.redirect.parent = true;
    options.deadline = reproc::milliseconds(600000);
    options.working_directory = m_TestFilesDir.c_str();

    std::vector<std::string> args = {m_CMakeExecutable, "-E", "tar", "xvzf", fmt::format("{}/{}", m_TestFilesDir, m_InputArchiveName)};

    auto&& [status, ec] = reproc::run(args, options);

    return !ec;
  }

private:
  std::string m_CMakeExecutable;
  std::string m_TestFilesDir;
  std::string m_InputArchiveName;
  std::string m_ExpectedTopLevelOutput;
};

/**
 * @brief closeEnough
 * @param a
 * @param b
 * @param epsilon
 * @return
 */
template <typename K>
bool CloseEnough(const K& a, const K& b, const K& epsilon = EPSILON)
{
  return (epsilon > fabs(a - b));
}

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
  Pipeline pipeline;
  const Result<> result2 = DREAM3D::WriteFile(filepath, dataStructure, pipeline, true);
  COMPLEX_RESULT_REQUIRE_VALID(result2);
}

/**
 * @brief Compares two Image Geometries
 * @param dataStructure
 * @param exemplaryDataPath
 * @param computedPath
 */
inline void CompareImageGeometry(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
  const auto* exemplarGeom = dataStructure.getDataAs<ImageGeom>(exemplaryDataPath);
  const auto* computedGeom = dataStructure.getDataAs<ImageGeom>(computedPath);
  REQUIRE(exemplarGeom != nullptr);
  REQUIRE(computedGeom != nullptr);

  const auto exemplarDims = exemplarGeom->getDimensions();
  const auto computedDims = computedGeom->getDimensions();
  REQUIRE(exemplarDims == computedDims);

  const auto exemplarSpacing = exemplarGeom->getSpacing();
  const auto computedSpacing = computedGeom->getSpacing();
  REQUIRE(exemplarSpacing == computedSpacing);

  const auto exemplarOrigin = exemplarGeom->getOrigin();
  const auto computedOrigin = computedGeom->getOrigin();
  REQUIRE(exemplarOrigin == computedOrigin);
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
 * @brief Compares 2 DataArrays using an EPSILON value. Useful for floating point comparisons
 * @tparam T
 * @param dataStructure
 * @param exemplaryDataPath
 * @param computedPath
 */
template <typename T>
void CompareFloatArraysWithNans(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, const T& epsilon = EPSILON, bool checkNans = true)
{
  static_assert(std::is_floating_point_v<T>);

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
    if(!checkNans && (std::isnan(newVal) || std::isnan(oldVal)))
    {
      continue;
    }
    if(std::isnan(oldVal) && std::isnan(newVal))
    {
      // https://stackoverflow.com/questions/38798791/nan-comparison-rule-in-c-c
      continue;
    }
    if(oldVal != newVal)
    {
      float diff = std::fabs(static_cast<float>(oldVal - newVal));
      REQUIRE(diff < epsilon);
      break;
    }
  }
}

/**
 * @brief Compares 2 NeighborList arrays using an EPSILON value. Useful for floating point comparisons
 * @tparam T
 * @param dataStructure
 * @param exemplaryDataPath
 * @param computedPath
 */
template <typename T>
void CompareNeighborListFloatArraysWithNans(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, const T& epsilon = EPSILON, bool checkNans = true)
{
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
        if(!checkNans && (std::isnan(computedVal) || std::isnan(exemplaryVal)))
        {
          continue;
        }
        if(std::isnan(exemplaryVal) && std::isnan(computedVal))
        {
          // https://stackoverflow.com/questions/38798791/nan-comparison-rule-in-c-c
          continue;
        }
        if(exemplaryVal != computedVal)
        {
          float diff = std::fabs(static_cast<float>(exemplaryVal - computedVal));
          INFO(fmt::format("Bad Neighborlist Comparison\n  Exemplary NeighborList:'{}'  size:{}\n  Computed NeighborList: '{}' size:{} ", exemplaryDataPath.toString(), exemplary->size(),
                           computedPath.toString(), computed->size()));
          INFO(fmt::format("  NeighborList {}, Index {} Exemplary Value: {} Computed Value: {}", i, j, exemplaryVal, computedVal))

          REQUIRE(diff < epsilon);
          break;
        }
      }
    }
  }
}

/**
 * @brief
 * @tparam T
 * @param exemplaryNeighborList
 * @param computedNeighborList
 */
template <typename T>
void CompareNeighborLists(const INeighborList* exemplaryData, const INeighborList* computedData)
{
  if constexpr(std::is_same_v<T, bool>)
  {
    INFO("Invalid data type (bool) for NeighborList array. Cannot compare values.")
  }
  else
  {
    const auto* generatedListArray = dynamic_cast<const NeighborList<T>*>(computedData);
    const auto* exemplarListArray = dynamic_cast<const NeighborList<T>*>(exemplaryData);
    const auto& computedList = *generatedListArray;
    const auto& exemplaryList = *exemplarListArray;

    REQUIRE(computedList.getNumberOfTuples() == exemplaryList.getNumberOfTuples());

    for(usize i = 0; i < exemplaryList.getNumberOfTuples(); i++)
    {
      const auto exemplary = exemplaryList.getList(i);
      const auto computed = computedList.getList(i);
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
            INFO(fmt::format("Bad Neighborlist Comparison\n  Exemplary NeighborList:'{}'  size:{}\n  Computed NeighborList: '{}' size:{} ", exemplaryList.getDataPaths()[0].toString(),
                             exemplary->size(), computedList.getDataPaths()[0].toString(), computed->size()));
            INFO(fmt::format("  NeighborList {}, Index {} Exemplary Value: {} Computed Value: {}", i, j, exemplaryVal, computedVal))

            REQUIRE(diff < EPSILON);
            break;
          }
        }
      }
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

/**
 * @brief Compares the referenced StringArray objects in the dataStructure for any differences
 * @param dataStructure
 * @param exemplaryDataPath
 * @param computedPath
 */
inline void CompareStringArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
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
 * @brief Compares the referenced StringArray objects for any differences
 * @param exemplar
 * @param computed
 */
inline void CompareStringArrays(const StringArray& exemplar, const StringArray& computed)
{
  REQUIRE(exemplar.getNumberOfTuples() == computed.getNumberOfTuples());

  INFO(fmt::format("Input Data Array:'{}'  Output StringArray: '{}' bad comparison", exemplar.getDataPaths()[0].toString(), computed.getDataPaths()[0].toString()));

  constexpr usize start = 0;
  const usize end = exemplar.getSize();
  for(usize i = start; i < end; i++)
  {
    REQUIRE(exemplar[i] == computed[i]);
  }
}

/**
 * @brief Compares the referenced DynamicListArray objects in the dataStructure for any differences
 * @tparam T index type
 * @tparam K value type
 * @param dataStructure
 * @param exemplaryDataPath
 * @param computedPath
 */
template <typename T, typename K>
void CompareDynamicListArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DynamicListArray<T, K>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DynamicListArray<T, K>>(computedPath));

  const auto& exemplaryArray = dataStructure.getDataRefAs<DynamicListArray<T, K>>(exemplaryDataPath);
  const auto& generatedArray = dataStructure.getDataRefAs<DynamicListArray<T, K>>(computedPath);
  REQUIRE(exemplaryArray.size() == generatedArray.size());

  INFO(fmt::format("Input Data Array:'{}'  Output DynamicListArray: '{}' bad comparison", exemplaryDataPath.toString(), computedPath.toString()));

  usize start = 0;
  usize end = exemplaryArray.size();
  for(usize i = start; i < end; i++)
  {
    auto oldEltList = exemplaryArray.getElementList(i);
    auto newEltList = generatedArray.getElementList(i);
    T oldNumCells = oldEltList.numCells;
    T newNumCells = newEltList.numCells;
    if(oldNumCells != newNumCells)
    {
      float diff = std::fabs(static_cast<float>(oldNumCells - newNumCells));
      REQUIRE(diff < EPSILON);
    }
    for(T j = 0; j < oldNumCells; ++j)
    {
      auto oldVal = oldEltList.cells[j];
      auto newVal = newEltList.cells[j];
      if(oldVal != newVal)
      {
        float diff = std::fabs(static_cast<float>(oldVal - newVal));
        REQUIRE(diff < EPSILON);
      }
    }
  }
}

template <typename T>
void CompareArrays(const IArray* generatedArray, const IArray* exemplarArray)
{
  const IArray::ArrayType arrayType = generatedArray->getArrayType();
  const IArray::ArrayType exemplarArrayType = exemplarArray->getArrayType();
  if(arrayType != exemplarArrayType)
  {
    std::cout << fmt::format("Generated array {} and exemplar array {} do not have the same array type: {} vs {}. Data Will not be compared.", generatedArray->getName(), exemplarArray->getName(),
                             arrayType, exemplarArrayType)
              << std::endl;
    return;
  }

  DataType type;
  if(arrayType == IArray::ArrayType::DataArray)
  {
    const auto* generatedDataArray = dynamic_cast<const IDataArray*>(generatedArray);
    const auto* exemplarDataArray = dynamic_cast<const IDataArray*>(exemplarArray);
    type = generatedDataArray->getDataType();
    DataType exemplarType = exemplarDataArray->getDataType();

    if(type != exemplarType)
    {
      std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray->getName(), exemplarDataArray->getName(), type, exemplarType)
                << std::endl;
      return;
    }
    CompareDataArrays<T>(*exemplarDataArray, *generatedDataArray);
  }
  if(arrayType == IArray::ArrayType::NeighborListArray)
  {
    const auto* generatedDataArray = dynamic_cast<const INeighborList*>(generatedArray);
    const auto* exemplarDataArray = dynamic_cast<const INeighborList*>(exemplarArray);
    type = generatedDataArray->getDataType();
    DataType exemplarType = exemplarDataArray->getDataType();
    if(type != exemplarType)
    {
      std::cout << fmt::format("NeighborList {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray->getName(), exemplarDataArray->getName(), type,
                               exemplarType)
                << std::endl;
      return;
    }
    CompareNeighborLists<T>(exemplarDataArray, generatedDataArray);
  }
  if(arrayType == IArray::ArrayType::StringArray)
  {
    const auto* generatedDataArray = dynamic_cast<const StringArray*>(generatedArray);
    const auto* exemplarDataArray = dynamic_cast<const StringArray*>(exemplarArray);
    CompareStringArrays(*exemplarDataArray, *generatedDataArray);
  }
}

/**
 * @brief Creates a DataArray backed by a DataStore (in memory).
 * @tparam T The primitive type to use, i.e. int8, float, double
 * @param dataStructure The DataStructure to use
 * @param name The name of the DataArray
 * @param tupleShape  The tuple dimensions of the data. If you want to mimic an image then your shape should be {height, width} slowest to fastest dimension
 * @param componentShape The component dimensions of the data. If you want to mimic an RGB image then your component would be {3},
 * if you want to store a 3Rx4C matrix then it would be {3, 4}.
 * @param parentId The DataObject that will own the DataArray instance.
 * @return
 */
template <typename T>
DataArray<T>* CreateTestDataArray(DataStructure& dataStructure, const std::string& name, typename DataStore<T>::ShapeType tupleShape, typename DataStore<T>::ShapeType componentShape,
                                  DataObject::IdType parentId = {})
{
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  ArrayType* dataArray = ArrayType::template CreateWithStore<DataStoreType>(dataStructure, name, tupleShape, componentShape, parentId);
  dataArray->fill(static_cast<T>(0.0));
  return dataArray;
}

template <typename T>
NeighborList<T>* CreateTestNeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples, DataObject::IdType parentId)
{
  using NeighborListType = NeighborList<T>;
  auto* neighborList = NeighborListType::Create(dataStructure, name, numTuples, parentId);
  neighborList->resizeTotalElements(numTuples);
  return neighborList;
}

/**
 * @brief Creates a DataStructure that mimics an EBSD data set
 * @return
 */
inline DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroup->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.55f, 1.86});
  imageGeom->setOrigin({0.0f, 20.0f, 66.0f});
  SizeVec3 imageGeomDims = {40, 60, 80};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  usize numComponents = 1;
  std::vector<usize> tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  Float32Array* ci_data = CreateTestDataArray<float>(dataStructure, Constants::k_ConfidenceIndex, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = CreateTestDataArray<int32>(dataStructure, Constants::k_FeatureIds, tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = CreateTestDataArray<int32>(dataStructure, "Phases", tupleShape, {numComponents}, scanData->getId());
  UInt64Array* voxelIndices = CreateTestDataArray<uint64>(dataStructure, "Voxel Indices", tupleShape, {numComponents}, scanData->getId());

  BoolArray* conditionalArray = CreateTestDataArray<bool>(dataStructure, Constants::k_ConditionalArray, tupleShape, {1}, scanData->getId());
  conditionalArray->fill(true);

  numComponents = 3;
  UInt8Array* ipf_color_data = CreateTestDataArray<uint8>(dataStructure, "IPF Colors", tupleShape, {numComponents}, scanData->getId());
  Float32Array* euler_data = CreateTestDataArray<float>(dataStructure, "Euler", tupleShape, {numComponents}, scanData->getId());

  // Add in another group that holds the phase data such as Laue Class, Lattice Constants, etc.
  DataGroup* ensembleGroup = DataGroup::Create(dataStructure, "Phase Data", topLevelGroup->getId());
  numComponents = 1;
  usize numTuples = 2;
  Int32Array* laue_data = CreateTestDataArray<int32>(dataStructure, "Laue Class", {numTuples}, {numComponents}, ensembleGroup->getId());

  // Create a Vertex Geometry grid for the Scan Data
  VertexGeom* vertexGeom = VertexGeom::Create(dataStructure, Constants::k_VertexGeometry, scanData->getId());
  vertexGeom->setVertices(*euler_data);

  // NeighborList<float32>* neighborList = CreateTestNeighborList<float32>(dataStructure, "Neighbor List", numTuples, scanData->getId());

  return dataStructure;
}

/**
 * @brief Creates a DataStructure with 2 groups. one group has a DataArray of each primitive type with 1 component and the
 * other group has a DataArray of each primitive type with 3 components.
 * @return
 */
inline DataStructure CreateAllPrimitiveTypes(const std::vector<usize>& tupleShape)
{
  DataStructure dataStructure;
  DataGroup* levelZeroGroup = DataGroup::Create(dataStructure, Constants::k_LevelZero);
  // auto levelZeroId = levelZeroGroup->getId();
  DataGroup* levelOneGroup = DataGroup::Create(dataStructure, Constants::k_LevelOne, levelZeroGroup->getId());
  auto levelOneId = levelOneGroup->getId();
  DataGroup* levelTwoGroup = DataGroup::Create(dataStructure, Constants::k_LevelTwo, levelZeroGroup->getId());
  auto levelTwoId = levelTwoGroup->getId();

  //  // Create an Image Geometry grid for the Scan Data
  //  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, levelOneGroup->getId());
  //  SizeVec3 imageGeomDims = {40, 60, 80};
  //  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)
  //  imageGeom->setSpacing({0.25f, 0.55f, 1.86});
  //  imageGeom->setOrigin({0.0f, 20.0f, 66.0f});

  // DataStore<usize>::ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  // Create Scalar type data
  DataStore<uint64>::ShapeType componentShape = {1ULL};

  CreateTestDataArray<int8>(dataStructure, Constants::k_Int8DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint8>(dataStructure, Constants::k_Uint8DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int16>(dataStructure, Constants::k_Int16DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint16>(dataStructure, Constants::k_Uint16DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int32>(dataStructure, Constants::k_Int32DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint32>(dataStructure, Constants::k_Uint32DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int64>(dataStructure, Constants::k_Int64DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint64>(dataStructure, Constants::k_Uint64DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<float32>(dataStructure, Constants::k_Float32DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<float64>(dataStructure, Constants::k_Float64DataSet, tupleShape, componentShape, levelOneId);

  // Create Vector/RGB type of data
  componentShape = {3ULL};
  CreateTestDataArray<int8>(dataStructure, Constants::k_Int8DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint8>(dataStructure, Constants::k_Uint8DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int16>(dataStructure, Constants::k_Int16DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint16>(dataStructure, Constants::k_Uint16DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int32>(dataStructure, Constants::k_Int32DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint32>(dataStructure, Constants::k_Uint32DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int64>(dataStructure, Constants::k_Int64DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint64>(dataStructure, Constants::k_Uint64DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<float32>(dataStructure, Constants::k_Float32DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<float64>(dataStructure, Constants::k_Float64DataSet, tupleShape, componentShape, levelTwoId);

  return dataStructure;
}

/**
 * @brief Adds an ImageGeometry of the prescribed size to a group in the DataStructure.
 */
inline void AddImageGeometry(DataStructure& dataStructure, const SizeVec3& imageGeomDims, const FloatVec3& spacing, const FloatVec3& origin, const DataGroup& dataGroup)
{
  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, dataGroup.getId());
  if(imageGeom == nullptr)
  {
    throw std::runtime_error("UnitTestCommon: Unable to create ImageGeom");
  }
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)
  imageGeom->setSpacing(spacing);
  imageGeom->setOrigin(origin);
}

inline void CompareExemplarToGeneratedData(const DataStructure& dataStructure, const DataStructure& exemplarDataStructure, const DataPath attributeMatrix, const std::string& exemplarDataContainerName)
{
  auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(attributeMatrix);
  std::vector<DataPath> selectedCellArrays;

  // Create the vector of selected cell DataPaths
  for(auto& child : cellDataGroup)
  {
    selectedCellArrays.push_back(attributeMatrix.createChildPath(child.second->getName()));
  }

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    const auto* generatedArray = dataStructure.getDataAs<IArray>(cellArrayPath);
    // Now generate the path to the exemplar data set in the exemplar data structure.
    std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
    generatedPathVector[0] = exemplarDataContainerName;
    DataPath exemplarDataArrayPath(generatedPathVector);
    const auto* exemplarArray = exemplarDataStructure.getDataAs<IArray>(exemplarDataArrayPath);

    // Check to see if there is something to compare against in the exemplar file.
    if(nullptr == exemplarArray)
    {
      continue;
    }

    DataType type = DataType::int8;
    const IArray::ArrayType arrayType = generatedArray->getArrayType();
    if(arrayType == IArray::ArrayType::DataArray)
    {
      type = dataStructure.getDataRefAs<IDataArray>(cellArrayPath).getDataType();
    }
    if(arrayType == IArray::ArrayType::NeighborListArray)
    {
      type = dataStructure.getDataRefAs<INeighborList>(cellArrayPath).getDataType();
    }

    switch(type)
    {
    case DataType::boolean: {
      complex::UnitTest::CompareArrays<bool>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int8: {
      complex::UnitTest::CompareArrays<int8>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int16: {
      complex::UnitTest::CompareArrays<int16>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int32: {
      complex::UnitTest::CompareArrays<int32>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int64: {
      complex::UnitTest::CompareArrays<int64>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint8: {
      complex::UnitTest::CompareArrays<uint8>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint16: {
      complex::UnitTest::CompareArrays<uint16>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint32: {
      complex::UnitTest::CompareArrays<uint32>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint64: {
      complex::UnitTest::CompareArrays<uint64>(generatedArray, exemplarArray);
      break;
    }
    case DataType::float32: {
      complex::UnitTest::CompareArrays<float32>(generatedArray, exemplarArray);
      break;
    }
    case DataType::float64: {
      complex::UnitTest::CompareArrays<float64>(generatedArray, exemplarArray);
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }
  }
}

inline void CompareAsciiFiles(std::ifstream& computedFile, std::ifstream& exemplarFile, const std::vector<size_t>& lineIndicesToSkip)
{
  std::vector<std::string> computedLines;
  std::vector<std::string> exemplarLines;
  for(std::string line; std::getline(computedFile, line);)
  {
    computedLines.push_back(line);
  }
  for(std::string line; std::getline(exemplarFile, line);)
  {
    exemplarLines.push_back(line);
  }

  REQUIRE(computedLines.size() == exemplarLines.size());
  for(size_t i = 0; i < computedLines.size(); ++i)
  {
    if(std::find(begin(lineIndicesToSkip), end(lineIndicesToSkip), i) != std::end(lineIndicesToSkip))
    {
      continue;
    }

    REQUIRE(computedLines[i] == exemplarLines[i]);
  }
}

/**
 * Here's the DataStructure we will be working with:
 *
 *     A   B          Level Zero
 *    / \ /|\
 *   H   C | F        Level One
 *  /   / \|/ \
 * N   D   E   G      Level Two
 *    / \ / \ /|\
 *   I   J   K L M    Level Three
 */
inline DataStructure CreateComplexMultiLevelDataGraph()
{
  DataStructure dataStructure;

  // Level Zero //
  auto* groupA = DataGroup::Create(dataStructure, Constants::k_GroupAName);
  auto* groupB = DataGroup::Create(dataStructure, Constants::k_GroupBName);

  auto groupAPath = DataPath({groupA->getName()});
  auto groupBPath = DataPath({groupB->getName()});

  // Level One //
  auto* groupH = DataGroup::Create(dataStructure, Constants::k_GroupHName, groupA->getId());
  auto* groupC = DataGroup::Create(dataStructure, Constants::k_GroupCName, groupA->getId());
  groupB->insert(dataStructure.getSharedData(groupC->getId()));
  auto* groupF = DataGroup::Create(dataStructure, Constants::k_GroupFName, groupB->getId());

  auto groupAHPath = groupAPath.createChildPath(groupH->getName());

  auto groupACPath = groupAPath.createChildPath(groupC->getName());
  auto groupBCPath = groupBPath.createChildPath(groupC->getName());

  auto groupBFPath = groupBPath.createChildPath(groupF->getName());

  // Level Two //
  auto* groupD = DataGroup::Create(dataStructure, Constants::k_GroupDName, groupC->getId());
  auto* groupE = DataGroup::Create(dataStructure, Constants::k_GroupEName, groupC->getId());
  groupB->insert(dataStructure.getSharedData(groupE->getId()));
  groupF->insert(dataStructure.getSharedData(groupE->getId()));
  auto* groupG = DataGroup::Create(dataStructure, Constants::k_GroupGName, groupF->getId());
  auto* arrayN = CreateTestDataArray<int8>(dataStructure, Constants::k_ArrayNName, {1ULL}, {1ULL}, groupH->getId());

  groupAHPath.createChildPath(arrayN->getName());

  auto groupACDPath = groupACPath.createChildPath(groupD->getName());
  auto groupBCDPath = groupBCPath.createChildPath(groupD->getName());

  auto groupACEPath = groupACPath.createChildPath(groupE->getName());
  auto groupBCEPath = groupBCPath.createChildPath(groupE->getName());
  auto groupBEPath = groupBPath.createChildPath(groupE->getName());
  auto groupBFEPath = groupBFPath.createChildPath(groupE->getName());

  auto groupBFGPath = groupBFPath.createChildPath(groupG->getName());

  // Level Three //
  auto* arrayI = CreateTestDataArray<uint8>(dataStructure, Constants::k_ArrayIName, {1ULL}, {1ULL}, groupD->getId());
  auto* arrayJ = CreateTestDataArray<float32>(dataStructure, Constants::k_ArrayJName, {1ULL}, {1ULL}, groupD->getId());
  groupE->insert(dataStructure.getSharedData(arrayJ->getId()));
  auto* arrayK = CreateTestDataArray<float64>(dataStructure, Constants::k_ArrayKName, {1ULL}, {1ULL}, groupE->getId());
  groupG->insert(dataStructure.getSharedData(arrayK->getId()));
  auto* arrayL = CreateTestDataArray<uint32>(dataStructure, Constants::k_ArrayLName, {1ULL}, {1ULL}, groupG->getId());
  auto* arrayM = CreateTestDataArray<int64>(dataStructure, Constants::k_ArrayMName, {1ULL}, {1ULL}, groupG->getId());

  groupACDPath.createChildPath(arrayI->getName());
  groupBCDPath.createChildPath(arrayI->getName());

  groupBCDPath.createChildPath(arrayJ->getName());
  groupACEPath.createChildPath(arrayJ->getName());
  groupBCEPath.createChildPath(arrayJ->getName());
  groupBEPath.createChildPath(arrayJ->getName());
  groupBFEPath.createChildPath(arrayJ->getName());

  groupACEPath.createChildPath(arrayK->getName());
  groupBCEPath.createChildPath(arrayK->getName());
  groupBEPath.createChildPath(arrayK->getName());
  groupBFEPath.createChildPath(arrayK->getName());

  groupBFGPath.createChildPath(arrayL->getName());

  groupBFGPath.createChildPath(arrayM->getName());

  return dataStructure;
}

} // namespace UnitTest

// Make sure we can load the needed filters from the plugins
const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
// Make sure we can instantiate the MultiThreshold Objects Filter
const Uuid k_MultiThresholdObjectsId = *Uuid::FromString("4246245e-1011-4add-8436-0af6bed19228");
const FilterHandle k_MultiThresholdObjectsFilterHandle(k_MultiThresholdObjectsId, k_ComplexCorePluginId);
// Make sure we can instantiate the IdentifySample
const Uuid k_IdentifySampleFilterId = *Uuid::FromString("94d47495-5a89-4c7f-a0ee-5ff20e6bd273");
const FilterHandle k_IdentifySampleFilterHandle(k_IdentifySampleFilterId, k_ComplexCorePluginId);

} // namespace complex

namespace SmallIn100
{
//------------------------------------------------------------------------------
inline void ExecuteMultiThresholdObjects(DataStructure& dataStructure, const FilterList& filterList)
{
  constexpr StringLiteral k_ArrayThresholds_Key = "array_thresholds";
  constexpr StringLiteral k_CreatedDataPath_Key = "created_data_path";
  INFO(fmt::format("Error creating Filter '{}'  ", k_MultiThresholdObjectsFilterHandle.getFilterName()));

  auto filter = filterList.createFilter(k_MultiThresholdObjectsFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;

  ArrayThresholdSet arrayThresholdset;
  ArrayThresholdSet::CollectionType thresholds;

  std::shared_ptr<ArrayThreshold> ciThreshold = std::make_shared<ArrayThreshold>();
  ciThreshold->setArrayPath(Constants::k_ConfidenceIndexArrayPath);
  ciThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  ciThreshold->setComparisonValue(0.1);
  thresholds.push_back(ciThreshold);

  std::shared_ptr<ArrayThreshold> iqThreshold = std::make_shared<ArrayThreshold>();
  iqThreshold->setArrayPath(Constants::k_ImageQualityArrayPath);
  iqThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  iqThreshold->setComparisonValue(120.0);
  thresholds.push_back(iqThreshold);

  arrayThresholdset.setArrayThresholds(thresholds);

  args.insertOrAssign(k_ArrayThresholds_Key, std::make_any<ArrayThresholdsParameter::ValueType>(arrayThresholdset));
  args.insertOrAssign(k_CreatedDataPath_Key, std::make_any<std::string>(Constants::k_Mask));

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
}

//------------------------------------------------------------------------------
inline void ExecuteIdentifySample(DataStructure& dataStructure, const FilterList& filterList)
{
  INFO(fmt::format("Error creating Filter '{}'  ", k_IdentifySampleFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_IdentifySampleFilterHandle);
  REQUIRE(nullptr != filter);

  // Parameter Keys
  constexpr StringLiteral k_FillHoles_Key = "fill_holes";
  constexpr StringLiteral k_ImageGeom_Key = "image_geometry";
  constexpr StringLiteral k_GoodVoxels_Key = "good_voxels";

  Arguments args;
  args.insertOrAssign(k_FillHoles_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(k_ImageGeom_Key, std::make_any<GeometrySelectionParameter::ValueType>(Constants::k_DataContainerPath));
  args.insertOrAssign(k_GoodVoxels_Key, std::make_any<ArraySelectionParameter::ValueType>(Constants::k_MaskArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
}

} // namespace SmallIn100

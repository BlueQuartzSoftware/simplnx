#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/Montage/AbstractMontage.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ArrayThresholdsParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataTypeParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MD5.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
template <class>
inline constexpr bool IsResult_v = false;

template <class T>
inline constexpr bool IsResult_v<Result<T>> = true;
} // namespace nx::core

#define NX_IS_LVALUE_RESULT(value) (IsResult_v<std::remove_cvref_t<decltype(value)>> && std::is_lvalue_reference_v<decltype((value))>)

#define SIMPLNX_RESULT_CATCH_PRINT(result)                                                                                                                                                             \
  for(const auto& warning : (result).warnings())                                                                                                                                                       \
  {                                                                                                                                                                                                    \
    WARN(fmt::format("{} : {}", warning.code, warning.message));                                                                                                                                       \
  }                                                                                                                                                                                                    \
  if((result).invalid())                                                                                                                                                                               \
  {                                                                                                                                                                                                    \
    for(const auto& error : (result).errors())                                                                                                                                                         \
    {                                                                                                                                                                                                  \
      UNSCOPED_INFO(fmt::format("{} : {}", error.code, error.message));                                                                                                                                \
    }                                                                                                                                                                                                  \
  }

#define SIMPLNX_RESULT_REQUIRE_VALID(result)                                                                                                                                                           \
  do                                                                                                                                                                                                   \
  {                                                                                                                                                                                                    \
    static_assert(NX_IS_LVALUE_RESULT(result), "SIMPLNX_RESULT_REQUIRE_VALID requires an lvalue Result<T>");                                                                                           \
    SIMPLNX_RESULT_CATCH_PRINT(result);                                                                                                                                                                \
    REQUIRE((result).valid());                                                                                                                                                                         \
  } while(false);

#define SIMPLNX_RESULT_REQUIRE_INVALID(result)                                                                                                                                                         \
  do                                                                                                                                                                                                   \
  {                                                                                                                                                                                                    \
    static_assert(NX_IS_LVALUE_RESULT(result), "SIMPLNX_RESULT_REQUIRE_INVALID requires an lvalue Result<T>");                                                                                         \
    SIMPLNX_RESULT_CATCH_PRINT(result);                                                                                                                                                                \
    REQUIRE((result).invalid());                                                                                                                                                                       \
  } while(false);

namespace nx::core
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
inline constexpr StringLiteral k_Cell_Ensemble_Data("Cell Ensemble Data");
inline constexpr StringLiteral k_Phase_Data("Phase Data");

inline constexpr StringLiteral k_TriangleDataContainerName("TriangleDataContainer");
inline constexpr StringLiteral k_VertexDataContainerName("VertexDataContainer");
inline constexpr StringLiteral k_PointCloudContainerName("Point Cloud");
inline constexpr StringLiteral k_FaceData("FaceData");
inline constexpr StringLiteral k_Face_Data("Face Data");
inline constexpr StringLiteral k_FaceFeatureData("FaceFeatureData");
inline constexpr StringLiteral k_VertexData("VertexData");
inline constexpr StringLiteral k_Vertex_Data("Vertex Data");
inline constexpr StringLiteral k_Edge_Data("Edge Data");
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
inline constexpr StringLiteral k_AlignmentAMName("Alignment Shifts Data");
inline constexpr StringLiteral k_SlicesArrayName("Slice Indices");
inline constexpr StringLiteral k_RelativeShiftsArrayName("Relative Shifts");
inline constexpr StringLiteral k_CumulativeShiftsArrayName("Cumulative Shifts");

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

template <class T>
std::string ComputeMD5Hash(const std::vector<T>& outputDataArray)
{
  const T* dataPtr = outputDataArray.data();
  usize arraySize = outputDataArray.size();
  MD5 md5;
  md5.update(reinterpret_cast<const uint8*>(dataPtr), arraySize * sizeof(T));
  md5.finalize();
  return md5.hexdigest();
}

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
   * @param testFilesDir The directory where the archive is located
   * @param inputArchiveName The full name of the archive. The location is assumed to be in the TestFiles directory
   * @param expectedTopLevelOutput The name of the decompressed folder or file. WARNING: This assumes
   * that only a single file or single directory are part of the archive. In the case of a directory, the
   * directory itself can have as many subdirectories as needed.
   * @param decompressFiles Decompress the archive
   * @param removeTemp delete files that were decompressed
   */
  TestFileSentinel(std::string testFilesDir, std::string inputArchiveName, std::string expectedTopLevelOutput, bool decompressFiles = true, bool removeTemp = true);

  ~TestFileSentinel();

  TestFileSentinel(const TestFileSentinel&) = delete;            // Copy Constructor Not Implemented
  TestFileSentinel(TestFileSentinel&&) = delete;                 // Move Constructor Not Implemented
  TestFileSentinel& operator=(const TestFileSentinel&) = delete; // Copy Assignment Not Implemented
  TestFileSentinel& operator=(TestFileSentinel&&) = delete;      // Move Assignment Not Implemented

  /**
   * @brief Does the actual decompression of the archive.
   * @return
   */
  std::error_code decompress();

private:
  std::string m_TestFilesDir;
  std::string m_InputArchiveName;
  std::string m_ExpectedTopLevelOutput;
  bool m_Decompress;
  bool m_RemoveTemp;
};

/**
 * @brief This class provides RAII-style management of Preferences for unit tests.
 * It preserves the current preference values on construction, sets new test-specific values,
 * and restores the original values on destruction (even if the test fails).
 */
class PreferencesSentinel
{
public:
  /**
   * @brief Construct a Preferences Sentinel that saves current preferences, sets new values,
   * and restores original values on destruction.
   *
   * @param largeDataFormat The large data format to use (e.g., "Zarr", "FileStore")
   * @param largeDataSize The large data size threshold in bytes
   * @param forceOocData Whether to force out-of-core data storage
   */
  PreferencesSentinel(std::string largeDataFormat, int64 largeDataSize, bool forceOocData);

  ~PreferencesSentinel();

  PreferencesSentinel(const PreferencesSentinel&) = delete;            // Copy Constructor Not Implemented
  PreferencesSentinel(PreferencesSentinel&&) = delete;                 // Move Constructor Not Implemented
  PreferencesSentinel& operator=(const PreferencesSentinel&) = delete; // Copy Assignment Not Implemented
  PreferencesSentinel& operator=(PreferencesSentinel&&) = delete;      // Move Assignment Not Implemented

private:
  std::string m_OriginalFormat;
  int64 m_OriginalSize;
  bool m_OriginalForceOoc;
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
 * @brief closeEnough
 * @param a
 * @param b
 * @param epsilon
 * @return
 */
template <typename K>
bool CloseEnoughAbs(const K& a, const K& b, const K& epsilon = EPSILON)
{
  return (epsilon > std::abs(std::abs(a) - std::abs(b)));
}

/**
 * @brief Loads a .dream3d file into a DataStructure. Checks are made to ensure the filepath does exist
 * @param filepath
 * @return DataStructure
 */
DataStructure LoadDataStructure(const fs::path& filepath);

/**
 * @brief Loads all simplnx plugins using singleton pattern.
 * Plugins are loaded only once per test execution, subsequent calls return immediately.
 * Thread-safe initialization guaranteed by C++11 static local variable initialization.
 */
inline void LoadPlugins()
{
  static bool pluginsLoaded = []() {
    const Result<> result = Application::GetOrCreateInstance()->loadPlugins(SIMPLNX_BUILD_DIR, true);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    return true;
  }();
  (void)pluginsLoaded; // Suppress unused variable warning
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
  SIMPLNX_RESULT_REQUIRE_VALID(result2);
}

/**
 * @brief Compares two Image Geometries
 * @param exemplarGeom
 * @param computedGeom
 * @param threshold
 */
inline void CompareImageGeometry(const ImageGeom* exemplarGeom, const ImageGeom* computedGeom, float32 threshold = 0.0f)
{
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

  const auto exemplarOrigin = exemplarGeom->getOrigin();
  const auto computedOrigin = computedGeom->getOrigin();
  REQUIRE(std::fabs(exemplarOrigin[0] - computedOrigin[0]) <= threshold);
  REQUIRE(std::fabs(exemplarOrigin[1] - computedOrigin[1]) <= threshold);
  REQUIRE(std::fabs(exemplarOrigin[2] - computedOrigin[2]) <= threshold);
}

/**
 * @brief Compares two Image Geometries
 * @param dataStructure
 * @param exemplaryDataPath
 * @param computedPath
 */
inline void CompareImageGeometry(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, float32 threshold = 0.0f)
{
  INFO(fmt::format("Comparing Image Geometries. {} and {}", exemplaryDataPath.toString(), computedPath.toString()));
  const auto* exemplarGeom = dataStructure.getDataAs<ImageGeom>(exemplaryDataPath);
  const auto* computedGeom = dataStructure.getDataAs<ImageGeom>(computedPath);
  CompareImageGeometry(exemplarGeom, computedGeom, threshold);
}

/**
 * @brief Compares two IGeometries (HELPER FUNCTION)
 * @param geom1
 * @param geom2
 * @returns bool true if equivalent
 */
inline bool CompareIGeometry(const IGeometry* geom1, const IGeometry* geom2)
{
  REQUIRE(geom1->getGeomType() == geom2->getGeomType());
  REQUIRE(geom1->getSpatialDimensionality() == geom2->getSpatialDimensionality());
  REQUIRE(geom1->getUnitDimensionality() == geom2->getUnitDimensionality());
  REQUIRE(geom1->getNumberOfCells() == geom2->getNumberOfCells());
  REQUIRE(geom1->findAllChildrenOfType<IArray>().size() == geom2->findAllChildrenOfType<IArray>().size());
  REQUIRE(geom1->getParametricCenter() == geom2->getParametricCenter());

  return true;
}

/**
 * @brief Compares two Montages
 * @param exemplar
 * @param generated
 */
inline void CompareMontage(const AbstractMontage& exemplar, const AbstractMontage& generated)
{
  REQUIRE(exemplar.getTileCount() == generated.getTileCount());
  const AbstractMontage::CollectionType exemplarGeometries = exemplar.getGeometries();
  const AbstractMontage::CollectionType generatedGeometries = generated.getGeometries();
  for(const auto* exGeom : exemplarGeometries)
  {
    std::vector<usize> usedIndices(exemplarGeometries.size());
    bool exists = false;
    for(usize i = 0; i < generatedGeometries.size(); i++)
    {
      if(CompareIGeometry(exGeom, generatedGeometries[i]))
      {
        if(std::find(usedIndices.begin(), usedIndices.end(), i) == usedIndices.end())
        {
          usedIndices.push_back(i);
          exists = true;
          break;
        }
      }
    }
    REQUIRE(exists);
  }
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
  const auto& oldDataStore = left.template getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto& newDataStore = right.template getIDataStoreRefAs<AbstractDataStore<T>>();
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
      UNSCOPED_INFO(fmt::format("index=: {}  oldValue != newValue. {} != {}", i, oldVal, newVal));

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
 * @brief Wrapper for CompareDataArrays to use with the ExecuteDataFunction
 */
struct CompareArraysFunctor
{
  template <typename T>
  void operator()(const IDataArray& left, const IDataArray& right) const
  {
    CompareDataArrays<T>(left, right);
  }
};

/**
 * @brief Compares IDataArrays by a specific component
 * @tparam T
 * @param left
 * @param right
 * @param startTuple
 * @param component
 */
template <typename T>
void CompareDataArraysByComponent(const IDataArray& left, const IDataArray& right, const usize startTuple = 0, const usize component = 0)
{
  const auto& oldDataStore = left.template getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto& newDataStore = right.template getIDataStoreRefAs<AbstractDataStore<T>>();
  usize tupleCount = oldDataStore.getNumberOfTuples();
  usize componentCount = oldDataStore.getNumberOfComponents();
  INFO(fmt::format("Input Data Array:'{}'  Output DataArray: '{}' bad comparison", left.getName(), right.getName()));
  REQUIRE(startTuple < tupleCount);
  REQUIRE(component < componentCount);
  T oldVal;
  T newVal;
  bool failed = false;
  for(usize t = startTuple; t < tupleCount; t++)
  {
    oldVal = oldDataStore[t * componentCount + component];
    newVal = newDataStore[t * componentCount + component];
    if(oldVal != newVal)
    {
      UNSCOPED_INFO(fmt::format("tuple=: {}  component=: {}  oldValue != newValue. {} != {}", t, component, oldVal, newVal));

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
  INFO(fmt::format("Exemplary Data Array:'{}'\n  Computed DataArray: '{}'\n   bad comparison", exemplaryDataPath.toString(), computedPath.toString()));

  const auto& exemplaryDataArray = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);
  const auto& computedDataArray = dataStructure.getDataRefAs<DataArray<T>>(computedPath);
  REQUIRE(exemplaryDataArray.getNumberOfTuples() == computedDataArray.getNumberOfTuples());

  usize start = 0;
  usize end = exemplaryDataArray.getSize();
  for(usize i = start; i < end; i++)
  {
    auto oldVal = exemplaryDataArray[i];
    auto newVal = computedDataArray[i];
    if(oldVal != newVal)
    {
      float diff = std::fabs(static_cast<float>(oldVal - newVal));
      REQUIRE(diff < EPSILON);
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
  T maxDiff = 0;
  usize maxDiffIndex = 0;
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
      T diff = std::fabs(static_cast<T>(oldVal - newVal));
      if(diff > maxDiff)
      {
        maxDiff = diff;
        maxDiffIndex = i;
      }
    }
  }
  INFO(fmt::format("Maximum difference of {} occurs at index {} (epsilon = {})", maxDiff, maxDiffIndex, epsilon));
  REQUIRE(maxDiff < epsilon);
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
    auto exemplary = exemplaryList.getList(i);
    auto computed = computedNeighborList.getList(i);
    if(exemplary.size() != 0 && computed.size() != 0)
    {
      REQUIRE(exemplary.size() == computed.size());
      std::sort(exemplary.begin(), exemplary.end());
      std::sort(computed.begin(), computed.end());
      for(usize j = 0; j < exemplary.size(); ++j)
      {
        auto exemplaryVal = exemplary.at(j);
        auto computedVal = computed.at(j);
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
          INFO(fmt::format("Bad Neighborlist Comparison\n  Exemplary NeighborList:'{}'  size:{}\n  Computed NeighborList: '{}' size:{} ", exemplaryDataPath.toString(), exemplary.size(),
                           computedPath.toString(), computed.size()));
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
 * @param exemplaryData
 * @param computedData
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
      auto exemplary = exemplaryList.getList(i);
      auto computed = computedList.getList(i);
      if(exemplary.size() != 0 && computed.size() != 0)
      {
        REQUIRE(exemplary.size() == computed.size());
        std::sort(exemplary.begin(), exemplary.end());
        std::sort(computed.begin(), computed.end());
        for(usize j = 0; j < exemplary.size(); ++j)
        {
          auto exemplaryVal = exemplary.at(j);
          auto computedVal = computed.at(j);
          if(exemplaryVal != computedVal)
          {
            float diff = std::fabs(static_cast<float>(exemplaryVal - computedVal));
            INFO(fmt::format("Bad Neighborlist Comparison\n  Exemplary NeighborList:'{}'  size:{}\n  Computed NeighborList: '{}' size:{} ", exemplaryList.getDataPaths()[0].toString(),
                             exemplary.size(), computedList.getDataPaths()[0].toString(), computed.size()));
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
    auto exemplary = exemplaryList.getList(i);
    auto computed = computedNeighborList.getList(i);
    if(exemplary.size() != 0 && computed.size() != 0)
    {
      INFO(fmt::format("Bad Neighborlist Comparison\n  Exemplary NeighborList:'{}'  size:{}\n  Computed NeighborList: '{}' size:{} ", exemplaryDataPath.toString(), exemplary.size(),
                       computedPath.toString(), computed.size()));
      REQUIRE(exemplary.size() == computed.size());
      std::sort(exemplary.begin(), exemplary.end());
      std::sort(computed.begin(), computed.end());
      for(usize j = 0; j < exemplary.size(); ++j)
      {
        auto exemplaryVal = exemplary.at(j);
        auto computedVal = computed.at(j);
        if(exemplaryVal != computedVal)
        {
          float diff = std::fabs(static_cast<float>(exemplaryVal - computedVal));
          INFO(fmt::format("  NeighborList {}, Index {} Exemplary Value: {} Computed Value: {}", i, j, exemplaryVal, computedVal));

          REQUIRE(diff < EPSILON);
          break;
        }
      }
    }
  }
}

/**
 * @brief Wrapper for CompareNeighborLists to use with the ExecuteDataFunction
 */
struct CompareNeighborListsFunctor
{
  template <typename T>
  void operator()(const INeighborList* left, const INeighborList* right) const
  {
    CompareNeighborLists<T>(left, right);
  }
};

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
    const auto& oldVal = exemplaryDataArray[i];
    const auto& newVal = generatedDataArray[i];
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
                             fmt::underlying(arrayType), fmt::underlying(exemplarArrayType))
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
      std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray->getName(), exemplarDataArray->getName(),
                               fmt::underlying(type), fmt::underlying(exemplarType))
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
      std::cout << fmt::format("NeighborList {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray->getName(), exemplarDataArray->getName(),
                               fmt::underlying(type), fmt::underlying(exemplarType))
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

inline void CompareDataStructures(const DataStructure& dataStructureA, const DataStructure& dataStructureB, const DataPath& parentGroup = DataPath{})
{
  try
  {
    std::vector<std::string> childrenNamesA;
    std::vector<std::string> childrenNamesB;
    if(parentGroup.empty())
    {
      // std::cout << "DEBUG TEST: dsA size = " << dataStructureA.getSize() << "\tdsB size = " << dataStructureB.getSize();
      INFO(fmt::format("DEBUG TEST: dsA size = {}\tdsB size = {}", dataStructureA.getSize(), dataStructureB.getSize()));
      REQUIRE(dataStructureA.getSize() == dataStructureB.getSize());
      childrenNamesA = dataStructureA.getDataMap().getNames();
      childrenNamesB = dataStructureB.getDataMap().getNames();
    }
    else
    {
      const auto* parentA = dataStructureA.getDataAs<BaseGroup>(parentGroup);
      const auto* parentB = dataStructureB.getDataAs<BaseGroup>(parentGroup);
      REQUIRE(parentA != nullptr);
      REQUIRE(parentB != nullptr);
      const BaseGroup::GroupType parentAGroupType = parentA->getGroupType();
      const BaseGroup::GroupType parentBGroupType = parentB->getGroupType();
      REQUIRE(parentAGroupType == parentBGroupType);
      // std::cout << "DEBUG TEST: ds parentA size = " << parentA->getSize() << "\tds parentB size = " << parentB->getSize();
      INFO(fmt::format("DEBUG TEST: ds parentA size = {}\tds parentB size = {}", parentA->getSize(), parentB->getSize()));
      REQUIRE(parentA->getSize() == parentB->getSize());
      childrenNamesA = parentA->getDataMap().getNames();
      childrenNamesB = parentB->getDataMap().getNames();

      switch(parentAGroupType)
      {
      case nx::core::BaseGroup::GroupType::AttributeMatrix: {
        const auto* attributeMatrixA = dynamic_cast<const AttributeMatrix*>(parentA);
        const auto* attributeMatrixB = dynamic_cast<const AttributeMatrix*>(parentB);
        REQUIRE(attributeMatrixA != nullptr);
        REQUIRE(attributeMatrixB != nullptr);
        REQUIRE(attributeMatrixA->getShape() == attributeMatrixB->getShape());
        break;
      }
      case nx::core::BaseGroup::GroupType::ImageGeom: {
        const auto* geomA = dynamic_cast<const ImageGeom*>(parentA);
        const auto* geomB = dynamic_cast<const ImageGeom*>(parentB);
        CompareImageGeometry(geomA, geomB, UnitTest::EPSILON);
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::RectGridGeom: {
        const auto* geomA = dynamic_cast<const RectGridGeom*>(parentA);
        const auto* geomB = dynamic_cast<const RectGridGeom*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getDimensions() == geomB->getDimensions());
        const auto originA = geomA->getOrigin();
        const auto originB = geomB->getOrigin();
        REQUIRE(originA.valid());
        REQUIRE(originB.valid());
        const auto originAVec = originA.value();
        const auto originBVec = originB.value();
        REQUIRE(std::fabs(originAVec[0] - originBVec[0]) <= UnitTest::EPSILON);
        REQUIRE(std::fabs(originAVec[1] - originBVec[1]) <= UnitTest::EPSILON);
        REQUIRE(std::fabs(originAVec[2] - originBVec[2]) <= UnitTest::EPSILON);
        CompareDataArrays<float32>(geomA->getXBoundsRef(), geomB->getXBoundsRef());
        CompareDataArrays<float32>(geomA->getYBoundsRef(), geomB->getYBoundsRef());
        CompareDataArrays<float32>(geomA->getZBoundsRef(), geomB->getZBoundsRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::HexahedralGeom:
      case nx::core::BaseGroup::GroupType::TetrahedralGeom: {
        const auto* geomA = dynamic_cast<const INodeGeometry3D*>(parentA);
        const auto* geomB = dynamic_cast<const INodeGeometry3D*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getVertices() != nullptr);
        REQUIRE(geomB->getVertices() != nullptr);
        CompareDataArrays<float32>(geomA->getVerticesRef(), geomB->getVerticesRef());
        REQUIRE(geomA->getEdges() != nullptr);
        REQUIRE(geomB->getEdges() != nullptr);
        CompareDataArrays<uint64>(geomA->getEdgesRef(), geomB->getEdgesRef());
        REQUIRE(geomA->getFaces() != nullptr);
        REQUIRE(geomB->getFaces() != nullptr);
        CompareDataArrays<uint64>(geomA->getFacesRef(), geomB->getFacesRef());
        REQUIRE(geomA->getPolyhedra() != nullptr);
        REQUIRE(geomB->getPolyhedra() != nullptr);
        CompareDataArrays<uint64>(geomA->getPolyhedraRef(), geomB->getPolyhedraRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::QuadGeom:
      case nx::core::BaseGroup::GroupType::TriangleGeom: {
        const auto* geomA = dynamic_cast<const INodeGeometry2D*>(parentA);
        const auto* geomB = dynamic_cast<const INodeGeometry2D*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getVertices() != nullptr);
        REQUIRE(geomB->getVertices() != nullptr);
        CompareDataArrays<float32>(geomA->getVerticesRef(), geomB->getVerticesRef());
        REQUIRE(geomA->getEdges() != nullptr);
        REQUIRE(geomB->getEdges() != nullptr);
        CompareDataArrays<uint64>(geomA->getEdgesRef(), geomB->getEdgesRef());
        REQUIRE(geomA->getFaces() != nullptr);
        REQUIRE(geomB->getFaces() != nullptr);
        CompareDataArrays<uint64>(geomA->getFacesRef(), geomB->getFacesRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::EdgeGeom: {
        const auto* geomA = dynamic_cast<const EdgeGeom*>(parentA);
        const auto* geomB = dynamic_cast<const EdgeGeom*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getVertices() != nullptr);
        REQUIRE(geomB->getVertices() != nullptr);
        CompareDataArrays<float32>(geomA->getVerticesRef(), geomB->getVerticesRef());
        REQUIRE(geomA->getEdges() != nullptr);
        REQUIRE(geomB->getEdges() != nullptr);
        CompareDataArrays<uint64>(geomA->getEdgesRef(), geomB->getEdgesRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::VertexGeom: {
        const auto* geomA = dynamic_cast<const VertexGeom*>(parentA);
        const auto* geomB = dynamic_cast<const VertexGeom*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getVertices() != nullptr);
        REQUIRE(geomB->getVertices() != nullptr);
        CompareDataArrays<float32>(geomA->getVerticesRef(), geomB->getVerticesRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::DataGroup: {
        break;
      }
      default: {
        INFO(fmt::format("Object at path ({}) has unhandled type ({})", parentGroup.toString(), parentA->getTypeName()));
        REQUIRE(false);
        break;
      }
      }

      for(usize i = 0; i < childrenNamesA.size(); ++i)
      {
        // std::cout << "DEBUG TEST: child A name = " << childrenNamesA[i] << "\tchild B name = " << childrenNamesB[i];
        INFO(fmt::format("DEBUG TEST: child A name = '{}'\tchild B name = '{}'", childrenNamesA[i], childrenNamesB[i]));
        REQUIRE(childrenNamesA[i] == childrenNamesB[i]);

        DataPath childPath = parentGroup.createChildPath(childrenNamesA[i]);
        const DataObject* objectA = dataStructureA.getData(childPath);
        const DataObject* objectB = dataStructureB.getData(childPath);
        REQUIRE(objectA != nullptr);
        REQUIRE(objectB != nullptr);

        const DataObject::Type objectADataObjectType = objectA->getDataObjectType();
        REQUIRE(objectADataObjectType == objectB->getDataObjectType());

        switch(objectADataObjectType)
        {
        case nx::core::DataObject::Type::DynamicListArray: {
          // TODO: ??
          break;
        }
        case nx::core::DataObject::Type::ScalarData: {
          // TODO: ??
          std::cout << objectA->getTypeName() << ": " << objectA->getName() << std::endl;
          break;
        }
        case nx::core::DataObject::Type::AbstractMontage: {
          // TODO: ??
          break;
        }
        case nx::core::DataObject::Type::IDataArray:
        case nx::core::DataObject::Type::DataArray: {
          const auto* dataArrayA = dynamic_cast<const IDataArray*>(objectA);
          const auto* dataArrayB = dynamic_cast<const IDataArray*>(objectB);
          REQUIRE(dataArrayA != nullptr);
          REQUIRE(dataArrayB != nullptr);
          REQUIRE(dataArrayA->getDataType() == dataArrayB->getDataType());
          // std::cout << "DEBUG TEST: data array A DataType = " << DataTypeToString(dataArrayA->getDataType()) << "\tdata array B DataType = " << DataTypeToString(dataArrayB->getDataType());
          INFO(fmt::format("DEBUG TEST: data array A DataType = {}\tdata array B DataType = {}", DataTypeToString(dataArrayA->getDataType()), DataTypeToString(dataArrayB->getDataType())));

          ExecuteDataFunction(CompareArraysFunctor{}, dataArrayA->getDataType(), *dataArrayA, *dataArrayB);

          break;
        }
        case nx::core::DataObject::Type::StringArray: {
          const auto* stringArrayA = dynamic_cast<const StringArray*>(objectA);
          const auto* stringArrayB = dynamic_cast<const StringArray*>(objectB);
          REQUIRE(stringArrayA != nullptr);
          REQUIRE(stringArrayB != nullptr);
          CompareStringArrays(*stringArrayA, *stringArrayB);
          break;
        }
        case nx::core::DataObject::Type::INeighborList:
        case nx::core::DataObject::Type::NeighborList: {
          const auto* neighborlistA = dynamic_cast<const INeighborList*>(objectA);
          const auto* neighborlistB = dynamic_cast<const INeighborList*>(objectB);
          REQUIRE(neighborlistA != nullptr);
          REQUIRE(neighborlistB != nullptr);
          // std::cout << "DEBUG TEST: neighborlist array A DataType = " << DataTypeToString(neighborlistA->getDataType()) << "\tneighborlist B DataType = " <<
          // DataTypeToString(neighborlistB->getDataType());
          INFO(fmt::format("DEBUG TEST: NeighborList A DataType = {}\tNeighborList B DataType = {}", DataTypeToString(neighborlistA->getDataType()), DataTypeToString(neighborlistB->getDataType())));
          REQUIRE(neighborlistA->getDataType() == neighborlistB->getDataType());

          ExecuteDataFunction(CompareNeighborListsFunctor{}, neighborlistA->getDataType(), neighborlistA, neighborlistB);

          break;
        }
        case nx::core::DataObject::Type::VertexGeom:
        case nx::core::DataObject::Type::EdgeGeom:
        case nx::core::DataObject::Type::RectGridGeom:
        case nx::core::DataObject::Type::ImageGeom:
        case nx::core::DataObject::Type::INodeGeometry2D:
        case nx::core::DataObject::Type::QuadGeom:
        case nx::core::DataObject::Type::TriangleGeom:
        case nx::core::DataObject::Type::INodeGeometry3D:
        case nx::core::DataObject::Type::HexahedralGeom:
        case nx::core::DataObject::Type::TetrahedralGeom:
        case nx::core::DataObject::Type::AttributeMatrix:
        case nx::core::DataObject::Type::DataGroup:
        case nx::core::DataObject::Type::BaseGroup: {
          CompareDataStructures(dataStructureA, dataStructureB, childPath);
          break;
        }
        default: {
          auto underlyingDataType = to_underlying(objectADataObjectType);
          std::cout << "Missing DataType: " << underlyingDataType << std::endl;
          INFO(fmt::format("Object at path ({}) has unhandled type ({})", childPath.toString(), underlyingDataType));
          REQUIRE(false);
          break;
        }
        }
      }
    }
  } catch(std::exception& e)
  {
    INFO(fmt::format("Caught exception: {}", e.what()));
    REQUIRE(false);
  }
} // namespace UnitTest

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
DataArray<T>* CreateTestDataArray(DataStructure& dataStructure, const std::string& name, const ShapeType& tupleShape, const ShapeType& componentShape, DataObject::IdType parentId = {})
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
  auto* neighborList = NeighborListType::Create(dataStructure, name, {numTuples}, parentId);
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
  ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

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
inline DataStructure CreateAllPrimitiveTypes(const ShapeType& tupleShape)
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
  ShapeType componentShape = {1ULL};

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

inline void CompareExemplarToGenerateAttributeMatrix(const DataStructure& exemplarDataStructure, const DataPath& exemplarAttributeMatrix, const DataStructure& computedDataStructure,
                                                     const DataPath& computedAttributeMatrix, bool allMustMatch = false)
{
  auto& exemplarAttrMatr = exemplarDataStructure.getDataRefAs<AttributeMatrix>(exemplarAttributeMatrix);
  // std::vector<DataPath> selectedCellArrays;

  // Create the vector of all cell DataPaths from the exemplar data structure
  for(auto& exemplarArrayPath : exemplarAttrMatr)
  {

    DataPath exemplarDataArrayPath = exemplarAttributeMatrix.createChildPath(exemplarArrayPath.second->getName());
    DataPath computedDataArrayPath = computedAttributeMatrix.createChildPath(exemplarArrayPath.second->getName());
    INFO(fmt::format("Exemplar Array:'{}'  Computed Array: '{}'", exemplarDataArrayPath.toString(), computedDataArrayPath.toString()));

    const auto* exemplarArrayPtr = exemplarDataStructure.getDataAs<IArray>(exemplarDataArrayPath);
    const auto* computedArrayPtr = computedDataStructure.getDataAs<IArray>(computedDataArrayPath);

    // Check to see if there is something to compare against in the exemplar file.
    if(nullptr == exemplarArrayPtr && !allMustMatch)
    {
      continue;
    }
    if(nullptr == computedArrayPtr && !allMustMatch)
    {
      continue;
    }

    if(allMustMatch)
    {
      REQUIRE(exemplarArrayPtr != nullptr);
      REQUIRE(computedArrayPtr != nullptr);
    }

    DataType type = DataType::int8;
    const IArray::ArrayType arrayType = computedArrayPtr->getArrayType();
    if(arrayType == IArray::ArrayType::DataArray)
    {
      type = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath).getDataType();
    }
    if(arrayType == IArray::ArrayType::NeighborListArray)
    {
      type = exemplarDataStructure.getDataRefAs<INeighborList>(exemplarDataArrayPath).getDataType();
    }

    switch(type)
    {
    case DataType::boolean: {
      nx::core::UnitTest::CompareArrays<bool>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::int8: {
      nx::core::UnitTest::CompareArrays<int8>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::int16: {
      nx::core::UnitTest::CompareArrays<int16>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::int32: {
      nx::core::UnitTest::CompareArrays<int32>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::int64: {
      nx::core::UnitTest::CompareArrays<int64>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::uint8: {
      nx::core::UnitTest::CompareArrays<uint8>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::uint16: {
      nx::core::UnitTest::CompareArrays<uint16>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::uint32: {
      nx::core::UnitTest::CompareArrays<uint32>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::uint64: {
      nx::core::UnitTest::CompareArrays<uint64>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::float32: {
      nx::core::UnitTest::CompareArrays<float32>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::float64: {
      nx::core::UnitTest::CompareArrays<float64>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }
  }
}

inline void CompareExemplarToGeneratedData(const DataStructure& dataStructure, const DataStructure& exemplarDataStructure, const DataPath& attributeMatrix,
                                           const std::string& exemplarDataContainerName)
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
      nx::core::UnitTest::CompareArrays<bool>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int8: {
      nx::core::UnitTest::CompareArrays<int8>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int16: {
      nx::core::UnitTest::CompareArrays<int16>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int32: {
      nx::core::UnitTest::CompareArrays<int32>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int64: {
      nx::core::UnitTest::CompareArrays<int64>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint8: {
      nx::core::UnitTest::CompareArrays<uint8>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint16: {
      nx::core::UnitTest::CompareArrays<uint16>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint32: {
      nx::core::UnitTest::CompareArrays<uint32>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint64: {
      nx::core::UnitTest::CompareArrays<uint64>(generatedArray, exemplarArray);
      break;
    }
    case DataType::float32: {
      nx::core::UnitTest::CompareArrays<float32>(generatedArray, exemplarArray);
      break;
    }
    case DataType::float64: {
      nx::core::UnitTest::CompareArrays<float64>(generatedArray, exemplarArray);
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

  // groupAHPath.createChildPath(arrayN->getName());

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

inline void CheckArraysInheritTupleDims(const DataStructure& dataStructure, const std::vector<DataPath>& ignoredPaths = {})
{
  std::optional<std::vector<DataPath>> amPathsOpt = GetAllChildDataPathsRecursive(dataStructure, {}, DataObject::Type::AttributeMatrix);
  REQUIRE(amPathsOpt.has_value());
  for(const auto& amPath : amPathsOpt.value())
  {
    const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(amPath);
    std::optional<std::vector<DataPath>> daPathsOpt = GetAllChildArrayDataPaths(dataStructure, amPath, ignoredPaths);
    REQUIRE(daPathsOpt.has_value());
    for(const auto& daPath : daPathsOpt.value())
    {
      INFO(fmt::format("AttributeMatrix: {}  Array: '{}'", amPath.toString(), daPath.toString()));
      const auto& arr = dataStructure.getDataAs<IArray>(daPath);
      REQUIRE(attrMatrix.getShape() == arr->getTupleShape());
    }
  }
}

/**
 * @brief Asserts that every vertex on a triple line also reports a junction count of 3 or
 * more in NodeTypes. The two quantities are computed by completely independent paths -
 * NodeTypes from the mesher's own junction logic, triple lines from FaceLabels - so
 * agreement is a real signal. NodeTypes adds 10 for surface nodes, hence the % 10.
 *
 * A failure here indicates the NodeTypes producer is wrong, not GenerateTripleLines.
 *
 * Cost: O(triple-line vertices x mesh vertices) with exact coordinate matching, so this
 * helper is intended for small fixtures only - not for full datasets.
 */
inline void CheckTripleLineNodeTypeAgreement(const DataStructure& dataStructure, const DataPath& tripleLineGeomPath, const DataPath& triangleGeomPath, const DataPath& nodeTypesPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(tripleLineGeomPath));
  const auto& tripleLineGeom = dataStructure.getDataRefAs<EdgeGeom>(tripleLineGeomPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(triangleGeomPath));
  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(nodeTypesPath));
  const auto& nodeTypes = dataStructure.getDataRefAs<Int8Array>(nodeTypesPath);

  const auto& tripleLineVertsRef = tripleLineGeom.getVertices()->getDataStoreRef();
  const auto& meshVertsRef = triangleGeom.getVertices()->getDataStoreRef();

  // Triple line vertices are a compacted copy, so match them back by coordinate.
  for(usize i = 0; i < tripleLineGeom.getNumberOfVertices(); i++)
  {
    bool foundMatch = false;
    for(usize j = 0; j < triangleGeom.getNumberOfVertices(); j++)
    {
      if(tripleLineVertsRef[i * 3 + 0] == meshVertsRef[j * 3 + 0] && tripleLineVertsRef[i * 3 + 1] == meshVertsRef[j * 3 + 1] && tripleLineVertsRef[i * 3 + 2] == meshVertsRef[j * 3 + 2])
      {
        const int8 junctionCount = static_cast<int8>(nodeTypes[j] % 10);
        INFO("Triple line vertex " << i << " maps to mesh vertex " << j << " with NodeType " << static_cast<int32>(nodeTypes[j]));
        REQUIRE(junctionCount >= 3);
        foundMatch = true;
        break;
      }
    }
    INFO("Triple line vertex " << i << " has no matching mesh vertex");
    REQUIRE(foundMatch);
  }
}

/**
 * @brief Builds a 2x2x1 four-grain Image Geometry: every one of the four cells is its own
 * Feature, so the four cells meet along one interior grid edge with 4 unique Feature Ids.
 * Used by the triple-line tests across QuickSurfaceMesh, SurfaceNets, and M3CSurfaceMeshing.
 *
 * @param dataStructure The DataStructure to populate.
 * @param imageGeomName Name given to the created ImageGeom.
 * @param cellDataName Name given to the created Cell Data AttributeMatrix.
 * @param featureIdsName Name given to the created FeatureIds Int32Array.
 */
inline void BuildFourGrainBlock(DataStructure& dataStructure, const std::string& imageGeomName = "ImageGeom", const std::string& cellDataName = "Cell Data",
                                const std::string& featureIdsName = "FeatureIds")
{
  auto* imageGeom = ImageGeom::Create(dataStructure, imageGeomName);
  imageGeom->setDimensions({2, 2, 1});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(dataStructure, cellDataName, ShapeType{1, 2, 2}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto featureIdsStore = std::make_unique<DataStore<int32>>(std::vector<usize>{1, 2, 2}, std::vector<usize>{1}, 0);
  auto* featureIds = Int32Array::Create(dataStructure, featureIdsName, std::move(featureIdsStore), cellAM->getId());
  auto& featureIdsRef = featureIds->getDataStoreRef();
  featureIdsRef[0] = 1;
  featureIdsRef[1] = 2;
  featureIdsRef[2] = 3;
  featureIdsRef[3] = 4;
}

namespace Cropping
{
inline std::string BoolToString(bool v)
{
  return v ? "True" : "False";
}

inline std::string CropTypeToString(CropGeometryParameter::CropValues::TypeEnum t)
{
  using T = CropGeometryParameter::CropValues::TypeEnum;
  switch(t)
  {
  case T::NoCropping:
    return "NoCropping";
  case T::VoxelSubvolume:
    return "VoxelSubvolume";
  case T::PhysicalSubvolume:
    return "PhysicalSubvolume";
  }
  return "Unknown";
}

struct AxisBoundsChoices
{
  std::vector<SizeVec2> voxelX;
  std::vector<SizeVec2> voxelY;
  std::vector<SizeVec2> voxelZ;
  std::vector<FloatVec2Type> physX;
  std::vector<FloatVec2Type> physY;
  std::vector<FloatVec2Type> physZ;
};

//------------------------------------------------------------------------------
inline std::vector<CropGeometryParameter::ValueType> GenerateAllCropValues(const AxisBoundsChoices& C, bool is2D = false)
{
  std::vector<CropGeometryParameter::ValueType> out;

  // NoCropping
  {
    CropGeometryParameter::ValueType cv;
    cv.type = CropGeometryParameter::CropValues::TypeEnum::NoCropping;
    cv.cropX = false;
    cv.cropY = false;
    cv.cropZ = false;
    cv.is2D = is2D;
    out.push_back(cv);
  }

  // Flag combinations
  // 2D: only X/Y combinations, Z always false
  // 3D: original full set
  const std::vector<std::tuple<bool, bool, bool>> flagOrder = is2D ? std::vector<std::tuple<bool, bool, bool>>{{false, true, false}, {true, false, false}, {true, true, false}} :
                                                                     std::vector<std::tuple<bool, bool, bool>>{{false, false, true}, {false, true, false}, {false, true, true}, {true, false, false},
                                                                                                               {true, false, true},  {true, true, false},  {true, true, true}};

  // --------------------
  // Voxel subvolumes
  // --------------------
  for(const auto& [cx, cy, cz] : flagOrder)
  {
    std::vector<std::optional<SizeVec2>> xOpts = cx ? std::vector<std::optional<SizeVec2>>(C.voxelX.begin(), C.voxelX.end()) : std::vector<std::optional<SizeVec2>>{std::nullopt};

    std::vector<std::optional<SizeVec2>> yOpts = cy ? std::vector<std::optional<SizeVec2>>(C.voxelY.begin(), C.voxelY.end()) : std::vector<std::optional<SizeVec2>>{std::nullopt};

    std::vector<std::optional<SizeVec2>> zOpts = (!is2D && cz) ? std::vector<std::optional<SizeVec2>>(C.voxelZ.begin(), C.voxelZ.end()) : std::vector<std::optional<SizeVec2>>{std::nullopt};

    for(const auto& xb : xOpts)
    {
      for(const auto& yb : yOpts)
      {
        for(const auto& zb : zOpts)
        {
          CropGeometryParameter::ValueType cv;
          cv.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
          cv.cropX = cx;
          cv.cropY = cy;
          cv.cropZ = false;
          cv.is2D = is2D;

          if(xb)
          {
            cv.xBoundVoxels = *xb;
          }
          if(yb)
          {
            cv.yBoundVoxels = *yb;
          }

          // Z never set in 2D
          if(!is2D && zb)
          {
            cv.zBoundVoxels = *zb;
            cv.cropZ = cz;
          }

          out.push_back(cv);
        }
      }
    }
  }

  // --------------------
  // Physical subvolumes
  // --------------------
  for(const auto& [cx, cy, cz] : flagOrder)
  {
    std::vector<std::optional<FloatVec2Type>> xOpts = cx ? std::vector<std::optional<FloatVec2Type>>(C.physX.begin(), C.physX.end()) : std::vector<std::optional<FloatVec2Type>>{std::nullopt};

    std::vector<std::optional<FloatVec2Type>> yOpts = cy ? std::vector<std::optional<FloatVec2Type>>(C.physY.begin(), C.physY.end()) : std::vector<std::optional<FloatVec2Type>>{std::nullopt};

    std::vector<std::optional<FloatVec2Type>> zOpts =
        (!is2D && cz) ? std::vector<std::optional<FloatVec2Type>>(C.physZ.begin(), C.physZ.end()) : std::vector<std::optional<FloatVec2Type>>{std::nullopt};

    for(const auto& xb : xOpts)
    {
      for(const auto& yb : yOpts)
      {
        for(const auto& zb : zOpts)
        {
          CropGeometryParameter::ValueType cv;
          cv.type = CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
          cv.cropX = cx;
          cv.cropY = cy;
          cv.cropZ = false;
          cv.is2D = is2D;

          if(xb)
          {
            cv.xBoundPhysical = *xb;
          }
          if(yb)
          {
            cv.yBoundPhysical = *yb;
          }

          // Z never set in 2D
          if(!is2D && zb)
          {
            cv.zBoundPhysical = *zb;
            cv.cropZ = cz;
          }

          out.push_back(cv);
        }
      }
    }
  }

  return out;
}
} // namespace Cropping

} // namespace UnitTest

// Make sure we can load the needed filters from the plugins
constexpr Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
// Make sure we can instantiate the MultiThreshold Objects Filter
constexpr Uuid k_MultiThresholdObjectsId = *Uuid::FromString("4246245e-1011-4add-8436-0af6bed19228");
const FilterHandle k_MultiThresholdObjectsFilterHandle(k_MultiThresholdObjectsId, k_SimplnxCorePluginId);
// Make sure we can instantiate the IdentifySampleFilter
constexpr Uuid k_IdentifySampleFilterId = *Uuid::FromString("94d47495-5a89-4c7f-a0ee-5ff20e6bd273");
const FilterHandle k_IdentifySampleFilterHandle(k_IdentifySampleFilterId, k_SimplnxCorePluginId);

} // namespace nx::core

namespace SmallIn100
{
// These paths are excluded because they come from a version prior to
// NeighborList and StringArray having multidimensional tuple capability
const std::vector<DataPath> k_TupleCheckIgnoredPaths{{{"MirroredXDataContainer", "CellData", "NeighborList"}},
                                                     {{"MirroredXDataContainer", "CellData", "StringArray"}},
                                                     {{"MirroredXInconsistentArrays", "CellData", "NeighborList"}},
                                                     {{"MirroredXInconsistentArrays", "CellData", "StringArray"}},
                                                     {{"MirroredYDataContainer", "CellData", "NeighborList"}},
                                                     {{"MirroredYDataContainer", "CellData", "StringArray"}},
                                                     {{"MirroredYInconsistentArrays", "CellData", "NeighborList"}},
                                                     {{"MirroredYInconsistentArrays", "CellData", "StringArray"}},
                                                     {{"MirroredZDataContainer", "CellData", "NeighborList"}},
                                                     {{"MirroredZDataContainer", "CellData", "StringArray"}},
                                                     {{"MirroredZInconsistentArrays", "CellData", "NeighborList"}},
                                                     {{"MirroredZInconsistentArrays", "CellData", "StringArray"}},
                                                     {{"XDataContainer", "CellData", "NeighborList"}},
                                                     {{"XDataContainer", "CellData", "StringArray"}},
                                                     {{"XInconsistentArrays", "CellData", "NeighborList"}},
                                                     {{"XInconsistentArrays", "CellData", "StringArray"}},
                                                     {{"YDataContainer", "CellData", "NeighborList"}},
                                                     {{"YDataContainer", "CellData", "StringArray"}},
                                                     {{"YInconsistentArrays", "CellData", "NeighborList"}},
                                                     {{"YInconsistentArrays", "CellData", "StringArray"}},
                                                     {{"ZDataContainer", "CellData", "NeighborList"}},
                                                     {{"ZDataContainer", "CellData", "StringArray"}},
                                                     {{"ZInconsistentArrays", "CellData", "NeighborList"}},
                                                     {{"ZInconsistentArrays", "CellData", "StringArray"}}};

//------------------------------------------------------------------------------
/**
 * @brief Runs the Multithreshold objects filter. For backwards compatibility the `useBoolOutputType` parameter is available and defaulted to `true`.
 *
 * If a newer exemplar data set needs to have the filter generate `uint8` values, then set the argument to false. The filter
 * will by default create uint8 values.
 *
 * @param dataStructure
 * @param filterList
 * @param useBoolOutputType This is set to true for legacy support where exemplar data sets created boolean arrays.
 */
inline void ExecuteMultiThresholdObjects(DataStructure& dataStructure, const FilterList& filterList, bool useBoolOutputType = true)
{
  constexpr StringLiteral k_ArrayThresholds_Key = "array_thresholds_object";
  constexpr StringLiteral k_CreatedDataPath_Key = "output_data_array_name";
  constexpr StringLiteral k_CreatedMaskType_Key = "created_mask_type";

  INFO(fmt::format("Error creating Filter '{}'  ", k_MultiThresholdObjectsFilterHandle.getFilterName()));

  auto filter = filterList.createFilter(k_MultiThresholdObjectsFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;

  ArrayThresholdSet arrayThresholdset;
  ArrayThresholdSet::CollectionType thresholds;

  std::shared_ptr<ArrayThreshold> ciThreshold = std::make_shared<ArrayThreshold>();
  ciThreshold->setArrayPath(nx::core::Constants::k_ConfidenceIndexArrayPath);
  ciThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  ciThreshold->setComparisonValue(0.1);
  thresholds.push_back(ciThreshold);

  std::shared_ptr<ArrayThreshold> iqThreshold = std::make_shared<ArrayThreshold>();
  iqThreshold->setArrayPath(nx::core::Constants::k_ImageQualityArrayPath);
  iqThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  iqThreshold->setComparisonValue(120.0);
  thresholds.push_back(iqThreshold);

  arrayThresholdset.setArrayThresholds(thresholds);

  args.insertOrAssign(k_ArrayThresholds_Key, std::make_any<ArrayThresholdsParameter::ValueType>(arrayThresholdset));
  args.insertOrAssign(k_CreatedDataPath_Key, std::make_any<std::string>(nx::core::Constants::k_Mask));
  if(useBoolOutputType)
  {
    args.insertOrAssign(k_CreatedMaskType_Key, std::make_any<DataTypeParameter::ValueType>(DataType::boolean));
  }

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure, k_TupleCheckIgnoredPaths);
}

//------------------------------------------------------------------------------
inline void ExecuteIdentifySample(DataStructure& dataStructure, const FilterList& filterList)
{
  INFO(fmt::format("Error creating Filter '{}'  ", k_IdentifySampleFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_IdentifySampleFilterHandle);
  REQUIRE(nullptr != filter);

  // Parameter Keys
  constexpr StringLiteral k_FillHoles_Key = "fill_holes";
  constexpr StringLiteral k_ImageGeom_Key = "input_image_geometry_path";
  constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";

  Arguments args;
  args.insertOrAssign(k_FillHoles_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(k_ImageGeom_Key, std::make_any<GeometrySelectionParameter::ValueType>(nx::core::Constants::k_DataContainerPath));
  args.insertOrAssign(k_MaskArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(nx::core::Constants::k_MaskArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure, k_TupleCheckIgnoredPaths);
}
} // namespace SmallIn100

#pragma once

#include "simplnx/UnitTest/AlgorithmTestScope.hpp"

#include "MD5/MD5.hpp"
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
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/Montage/AbstractMontage.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ArrayThresholdsParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataTypeParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <nlohmann/json.hpp>

#include <catch2/catch.hpp>

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
template <class>
inline constexpr bool IsResult_v = false;

template <class T>
inline constexpr bool IsResult_v<Result<T>> = true;

template <class T>
inline constexpr bool k_IsLvalueResult = IsResult_v<std::remove_cvref_t<T>> && std::is_lvalue_reference_v<T>;
} // namespace nx::core

/**
 * @def SIMPLNX_RESULT_CATCH_PRINT
 * @brief Adds all warnings and errors from a Result to the current Catch2 test.
 * @param result Result expression to evaluate once and report.
 */
#define SIMPLNX_RESULT_CATCH_PRINT(result)                                                                                                                                                             \
  do                                                                                                                                                                                                   \
  {                                                                                                                                                                                                    \
    auto&& simplnxResultToPrint = (result);                                                                                                                                                            \
    for(const auto& warning : simplnxResultToPrint.warnings())                                                                                                                                         \
    {                                                                                                                                                                                                  \
      WARN(fmt::format("{} : {}", warning.code, warning.message));                                                                                                                                     \
    }                                                                                                                                                                                                  \
    if(simplnxResultToPrint.invalid())                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
      for(const auto& error : simplnxResultToPrint.errors())                                                                                                                                           \
      {                                                                                                                                                                                                \
        UNSCOPED_INFO(fmt::format("{} : {}", error.code, error.message));                                                                                                                              \
      }                                                                                                                                                                                                \
    }                                                                                                                                                                                                  \
  } while(false)

/**
 * @def SIMPLNX_RESULT_REQUIRE_VALID
 * @brief Reports a Result and requires it to be valid.
 * @param result Result expression to evaluate once and verify.
 */
#define SIMPLNX_RESULT_REQUIRE_VALID(result)                                                                                                                                                           \
  do                                                                                                                                                                                                   \
  {                                                                                                                                                                                                    \
    static_assert(nx::core::k_IsLvalueResult<decltype((result))>, "SIMPLNX_RESULT_REQUIRE_VALID requires an lvalue Result<T>");                                                                        \
    auto&& simplnxCheckedResult = (result);                                                                                                                                                            \
    SIMPLNX_RESULT_CATCH_PRINT(simplnxCheckedResult);                                                                                                                                                  \
    REQUIRE(simplnxCheckedResult.valid());                                                                                                                                                             \
  } while(false);

/**
 * @def SIMPLNX_RESULT_REQUIRE_INVALID
 * @brief Reports a Result and requires it to be invalid.
 * @param result Result expression to evaluate once and verify.
 */
#define SIMPLNX_RESULT_REQUIRE_INVALID(result)                                                                                                                                                         \
  do                                                                                                                                                                                                   \
  {                                                                                                                                                                                                    \
    static_assert(nx::core::k_IsLvalueResult<decltype((result))>, "SIMPLNX_RESULT_REQUIRE_INVALID requires an lvalue Result<T>");                                                                      \
    auto&& simplnxCheckedResult = (result);                                                                                                                                                            \
    SIMPLNX_RESULT_CATCH_PRINT(simplnxCheckedResult);                                                                                                                                                  \
    REQUIRE(simplnxCheckedResult.invalid());                                                                                                                                                           \
  } while(false);

namespace nx::core
{
/**
 * @namespace nx::core::Constants
 * @brief Provides common names and paths for simplnx unit tests.
 */
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

// These paths select the common test data container and its child objects.
inline const DataPath k_DataContainerPath({k_DataContainer});

inline const DataPath k_CellAttributeMatrix = k_DataContainerPath.createChildPath(k_CellData);
inline const DataPath k_EulersArrayPath = k_CellAttributeMatrix.createChildPath(k_EulerAngles);
inline const DataPath k_QuatsArrayPath = k_CellAttributeMatrix.createChildPath(k_Quats);
inline const DataPath k_PhasesArrayPath = k_CellAttributeMatrix.createChildPath(k_Phases);
inline const DataPath k_FeatureIdsArrayPath = k_CellAttributeMatrix.createChildPath(k_FeatureIds);
inline const DataPath k_ConfidenceIndexArrayPath = k_CellAttributeMatrix.createChildPath(k_Confidence_Index);
inline const DataPath k_ImageQualityArrayPath = k_CellAttributeMatrix.createChildPath(k_Image_Quality);
inline const DataPath k_MaskArrayPath = k_CellAttributeMatrix.createChildPath(k_Mask);
inline const DataPath k_FitArrayPath = k_CellAttributeMatrix.createChildPath(k_Fit);
inline const DataPath k_SEMSignalArrayPath = k_CellAttributeMatrix.createChildPath(k_SEMSignal);

inline const DataPath k_CellEnsembleAttributeMatrixPath = k_DataContainerPath.createChildPath(k_EnsembleAttributeMatrix);
inline const DataPath k_CrystalStructuresArrayPath = k_CellEnsembleAttributeMatrixPath.createChildPath(k_CrystalStructures);

inline const DataPath k_CellFeatureAttributeMatrix = k_DataContainerPath.createChildPath(k_Grain_Data);
inline const DataPath k_NumCellsPath = k_CellFeatureAttributeMatrix.createChildPath(k_NumElements);
inline const DataPath k_FeaturePhasesPath = k_CellFeatureAttributeMatrix.createChildPath(k_Phases);

inline const DataPath k_CellFeatureDataPath = k_DataContainerPath.createChildPath(k_CellFeatureData);

// This path selects the top-level exemplar data container.
inline const DataPath k_ExemplarDataContainerPath({k_ExemplarDataContainer});

} // namespace Constants

/**
 * @namespace nx::core::UnitTest
 * @brief Provides common data builders and assertions for simplnx unit tests.
 */
namespace UnitTest
{
#ifdef _WIN32
/** @brief Queries the POSIX unlink/rename capability of the volume containing an existing directory. */
[[nodiscard]] bool VolumeSupportsPosixRename(const std::filesystem::path& directory);
#endif

/**
 * @brief Gets a required object and reports lookup exceptions as test failures.
 * @tparam T Specifies the required DataObject type.
 * @param dataStructure Owns the required object.
 * @param path Identifies the required object.
 * @return Reference that remains valid while the object exists in dataStructure.
 */
template <typename T>
const T& GetRequiredData(const DataStructure& dataStructure, const DataPath& path)
{
  const T* valuePtr = nullptr;
  REQUIRE_NOTHROW(valuePtr = &dataStructure.getDataRefAs<T>(path));
  REQUIRE((valuePtr != nullptr));
  return *valuePtr;
}

/**
 * @brief Requires an HDF5-OOC array with an existing backing file.
 * @param array Supplies the store and recovery metadata to verify.
 */
void RequireDiskBackedStore(const IDataArray& array);

/**
 * @brief Creates a message handler that appends every message to a vector.
 * The handler is not thread-safe. The filter must serialize its messages.
 * @param messages Receives each message in the order it was sent. Must outlive the handler.
 * @return Handler that records messages for later inspection by a test.
 */
IFilter::MessageHandler RecordMessages(std::vector<IFilter::Message>& messages);

/**
 * @brief Verifies that each numeric creation action requests automatic storage.
 * @param outputActions Contains the preflight actions to inspect.
 * @param expectedCount Specifies the required numeric creation-action count.
 */
void RequireAutomaticCreateArrayActions(const Result<OutputActions>& outputActions, usize expectedCount);

/**
 * @brief Verifies automatic storage for the specified numeric output paths.
 * @param outputActions Contains the preflight actions to inspect.
 * @param expectedPaths Identifies each required numeric output.
 */
void RequireAutomaticCreateArrayActions(const Result<OutputActions>& outputActions, const std::vector<DataPath>& expectedPaths);

/**
 * @brief Default absolute tolerance for unit-test comparisons.
 */
inline constexpr float32 EPSILON = 0.0001;

/**
 * @brief Computes an MD5 digest from a vector's object representation.
 * @tparam T Specifies the vector element type.
 * @param outputDataArray Values to hash.
 * @return Lowercase hexadecimal MD5 digest.
 *
 * The digest depends on the host representation of T, including byte order and
 * any padding bytes.
 */
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
 * @class TestFileSentinel
 * @brief Decompresses a tar.gz archive for a test and optionally removes its output.
 *
 * The destructor removes only the expected top-level output path. The archive
 * can contain more entries, so callers must supply an output name that owns all
 * extracted content.
 */
class TestFileSentinel
{
public:
  /**
   * @brief Configures archive extraction and optional output removal.
   *
   * @param testFilesDir Directory that contains the archive and receives extracted entries.
   * @param inputArchiveName Archive file name relative to testFilesDir.
   * @param expectedTopLevelOutput Path that the destructor can remove relative to testFilesDir.
   * @param decompressFiles True to extract the archive during construction.
   * @param removeTemp True to remove expectedTopLevelOutput during destruction.
   * @note Construction prints an extraction error but does not fail the test.
   */
  TestFileSentinel(std::string testFilesDir, std::string inputArchiveName, std::string expectedTopLevelOutput, bool decompressFiles = true, bool removeTemp = true);

  /**
   * @brief Removes the configured top-level output when removal is enabled.
   */
  ~TestFileSentinel();

  TestFileSentinel(const TestFileSentinel&) = delete;
  TestFileSentinel(TestFileSentinel&&) = delete;
  TestFileSentinel& operator=(const TestFileSentinel&) = delete;
  TestFileSentinel& operator=(TestFileSentinel&&) = delete;

  /**
   * @brief Extracts regular files and directories from the configured gzip tar archive.
   * @return An error code if the archive cannot be opened, read, or validated.
   *
   * The extractor skips links and unsupported entry types. It supports GNU long
   * names and validates each tar header checksum.
   */
  std::error_code decompress();

private:
  std::string m_TestFilesDir;
  std::string m_InputArchiveName;
  std::string m_ExpectedTopLevelOutput;
  bool m_Decompress;
  bool m_RemoveTemp;
  std::filesystem::path m_HolderFile;
};

/**
 * @class PreferencesSentinel
 * @brief Applies temporary storage preferences to the process-wide Application.
 *
 * The constructor saves and changes the selected preferences. The destructor
 * restores the saved values in memory, including during stack unwinding.
 *
 * @warning Concurrent instances can interfere because they change the same process-wide preferences.
 */
class PreferencesSentinel
{
public:
  /**
   * @brief Saves the current preferences and applies test-specific storage values.
   * @param mode Storage mode to use during the sentinel lifetime.
   * @param largeDataSize Large-data threshold in bytes.
   */
  PreferencesSentinel(nx::core::DataStorageMode mode, int64 largeDataSize);

  /**
   * @brief Applies a temporary threshold without narrowing its JSON integer type.
   * @param mode Storage mode to use during the sentinel lifetime.
   * @param largeDataSize Exact JSON integer
   * threshold in bytes.
   */
  PreferencesSentinel(nx::core::DataStorageMode mode, const nlohmann::json& largeDataSize);

  /**
   * @brief Restores the saved storage preferences without writing a preferences file.
   */
  ~PreferencesSentinel();

  PreferencesSentinel(const PreferencesSentinel&) = delete;
  PreferencesSentinel(PreferencesSentinel&&) = delete;
  PreferencesSentinel& operator=(const PreferencesSentinel&) = delete;
  PreferencesSentinel& operator=(PreferencesSentinel&&) = delete;

private:
  nlohmann::json m_OriginalMode;
  nlohmann::json m_OriginalSize;
  bool m_HadMode = false;
  bool m_HadSize = false;
};

/**
 * @brief Tests whether two values differ by less than a tolerance.
 * @tparam K Specifies the value and tolerance type.
 * @param a First value.
 * @param b Second value.
 * @param epsilon Exclusive absolute-difference limit.
 * @return True if `abs(a - b) < epsilon`.
 */
template <typename K>
bool CloseEnough(const K& a, const K& b, const K& epsilon = EPSILON)
{
  return (epsilon > fabs(a - b));
}

/**
 * @brief Tests whether two value magnitudes differ by less than a tolerance.
 * @tparam K Specifies the value and tolerance type.
 * @param a First value.
 * @param b Second value.
 * @param epsilon Exclusive magnitude-difference limit.
 * @return True if `abs(abs(a) - abs(b)) < epsilon`.
 */
template <typename K>
bool CloseEnoughAbs(const K& a, const K& b, const K& epsilon = EPSILON)
{
  return (epsilon > std::abs(std::abs(a) - std::abs(b)));
}

/**
 * @brief Loads a .dream3d file into a DataStructure for a unit test.
 * @param filepath Existing .dream3d file path.
 * @return The loaded DataStructure.
 *
 * The helper loads plugins first. It fails the current Catch2 test if the path
 * does not exist or the DREAM3D reader returns an error.
 */
DataStructure LoadDataStructure(const fs::path& filepath);

/**
 * @brief Loads all simplnx plugins once for the current process.
 *
 * C++ static initialization serializes the first call. Later calls reuse the
 * completed result without loading the plugins again.
 */
void LoadPlugins();

/**
 * @brief Writes a DataStructure to a .dream3d file and requires success.
 * @param dataStructure DataStructure to write.
 * @param filepath Destination file path.
 */
void WriteTestDataStructure(const DataStructure& dataStructure, const fs::path& filepath);

/**
 * @brief Requires two ImageGeom objects to have equal dimensions and compatible coordinates.
 * @param exemplarGeom Non-null exemplar geometry.
 * @param computedGeom Non-null computed geometry.
 * @param threshold Inclusive absolute tolerance for each spacing and origin component.
 */
void CompareImageGeometry(const ImageGeom* exemplarGeom, const ImageGeom* computedGeom, float32 threshold = 0.0f);

/**
 * @brief Compares two ImageGeom objects that are stored in one DataStructure.
 * @param dataStructure Contains both geometries.
 * @param exemplaryDataPath Exemplar ImageGeom path.
 * @param computedPath Computed ImageGeom path.
 * @param threshold Inclusive absolute tolerance for each spacing and origin component.
 */
void CompareImageGeometry(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, float32 threshold = 0.0f);

/**
 * @brief Requires two geometries to have equal type, dimensions, cells, arrays, and center.
 * @param geom1 Non-null first geometry.
 * @param geom2 Non-null second geometry.
 * @return True if all Catch2 requirements continue.
 */
bool CompareIGeometry(const IGeometry* geom1, const IGeometry* geom2);

/**
 * @brief Compares two IDataArray objects in bounded blocks.
 *
 * The helper uses 40,000-element bulk reads to keep memory bounded and avoid
 * per-value OOC access. It treats two NaN values as equal for floating-point
 * arrays.
 *
 * @tparam T Specifies the array element type.
 * @param left Exemplar array.
 * @param right Computed array.
 * @param start Zero-based first element to compare.
 */
template <typename T>
void CompareDataArrays(const IDataArray& left, const IDataArray& right, usize start = 0)
{
  const auto& oldDataStore = left.template getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto& newDataStore = right.template getIDataStoreRefAs<AbstractDataStore<T>>();
  const usize totalSize = oldDataStore.getSize();
  INFO(fmt::format("Input Data Array:'{}'  Output DataArray: '{}' bad comparison", left.getName(), right.getName()));
  REQUIRE(totalSize == newDataStore.getSize());

  constexpr usize k_ChunkSize = 40000;
  auto oldBuf = std::make_unique<T[]>(k_ChunkSize);
  auto newBuf = std::make_unique<T[]>(k_ChunkSize);

  bool failed = false;
  usize failIndex = 0;
  T failOld = {};
  T failNew = {};

  for(usize offset = start; offset < totalSize && !failed; offset += k_ChunkSize)
  {
    const usize count = std::min(k_ChunkSize, totalSize - offset);
    auto oldDataStoreReadResult = oldDataStore.copyIntoBuffer(offset, nonstd::span<T>(oldBuf.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(oldDataStoreReadResult);
    auto newDataStoreReadResult = newDataStore.copyIntoBuffer(offset, nonstd::span<T>(newBuf.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(newDataStoreReadResult);

    for(usize i = 0; i < count; i++)
    {
      const T oldVal = oldBuf[i];
      const T newVal = newBuf[i];
      if(oldVal != newVal)
      {
        if constexpr(std::is_floating_point_v<T>)
        {
          // Two NaN values represent the same missing result for this test helper.
          if(std::isnan(oldVal) && std::isnan(newVal))
          {
            continue;
          }
          float32 diff = std::fabs(static_cast<float32>(oldVal - newVal));
          if(diff <= EPSILON)
          {
            continue;
          }
        }
        failed = true;
        failIndex = offset + i;
        failOld = oldVal;
        failNew = newVal;
        break;
      }
    }
  }

  if(failed)
  {
    UNSCOPED_INFO(fmt::format("index=: {}  oldValue != newValue. {} != {}", failIndex, failOld, failNew));
  }
  REQUIRE(!failed);
}

/**
 * @struct CompareArraysFunctor
 * @brief Adapts CompareDataArrays for ExecuteDataFunction type dispatch.
 */
struct CompareArraysFunctor
{
  /**
   * @brief Dispatches a typed array comparison.
   * @tparam T Specifies the dispatched array element type.
   * @param left Exemplar array.
   * @param right Computed array.
   */
  template <typename T>
  void operator()(const IDataArray& left, const IDataArray& right) const
  {
    CompareDataArrays<T>(left, right);
  }
};

/**
 * @brief Compares one component of two IDataArray objects in bounded blocks.
 * @tparam T Specifies the array element type.
 * @param left Exemplar array.
 * @param right Computed array.
 * @param startTuple Zero-based first tuple to compare.
 * @param component Zero-based component to compare.
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

  constexpr usize k_ChunkTuples = 40000;
  const usize chunkElements = k_ChunkTuples * componentCount;
  auto oldBuf = std::make_unique<T[]>(chunkElements);
  auto newBuf = std::make_unique<T[]>(chunkElements);

  bool failed = false;
  usize failTuple = 0;
  T failOld = {};
  T failNew = {};

  for(usize tStart = startTuple; tStart < tupleCount && !failed; tStart += k_ChunkTuples)
  {
    const usize tCount = std::min(k_ChunkTuples, tupleCount - tStart);
    const usize elemCount = tCount * componentCount;
    auto oldDataStoreReadResult = oldDataStore.copyIntoBuffer(tStart * componentCount, nonstd::span<T>(oldBuf.get(), elemCount));
    SIMPLNX_RESULT_REQUIRE_VALID(oldDataStoreReadResult);
    auto newDataStoreReadResult = newDataStore.copyIntoBuffer(tStart * componentCount, nonstd::span<T>(newBuf.get(), elemCount));
    SIMPLNX_RESULT_REQUIRE_VALID(newDataStoreReadResult);

    for(usize t = 0; t < tCount; t++)
    {
      const T oldVal = oldBuf[t * componentCount + component];
      const T newVal = newBuf[t * componentCount + component];
      if(oldVal != newVal)
      {
        if constexpr(std::is_floating_point_v<T>)
        {
          float32 diff = std::fabs(static_cast<float32>(oldVal - newVal));
          if(diff <= EPSILON)
          {
            continue;
          }
        }
        failed = true;
        failTuple = tStart + t;
        failOld = oldVal;
        failNew = newVal;
        break;
      }
    }
  }

  if(failed)
  {
    UNSCOPED_INFO(fmt::format("tuple=: {}  component=: {}  oldValue != newValue. {} != {}", failTuple, component, failOld, failNew));
  }
  REQUIRE(!failed);
}

/**
 * @brief Compares two numeric DataArray objects with the default absolute tolerance.
 * @tparam T Specifies the array element type.
 * @param dataStructure Contains both arrays.
 * @param exemplaryDataPath Exemplar array path.
 * @param computedPath Computed array path.
 */
template <typename T>
void CompareArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(computedPath));
  INFO(fmt::format("Exemplary Data Array:'{}'\n  Computed DataArray: '{}'\n   bad comparison", exemplaryDataPath.toString(), computedPath.toString()));

  const auto& exemplaryDataArray = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);
  const auto& computedDataArray = dataStructure.getDataRefAs<DataArray<T>>(computedPath);
  REQUIRE(exemplaryDataArray.getNumberOfTuples() == computedDataArray.getNumberOfTuples());

  const auto& oldStore = exemplaryDataArray.getDataStoreRef();
  const auto& newStore = computedDataArray.getDataStoreRef();
  const usize totalSize = oldStore.getSize();

  constexpr usize k_ChunkSize = 40000;
  auto oldBuf = std::make_unique<T[]>(k_ChunkSize);
  auto newBuf = std::make_unique<T[]>(k_ChunkSize);

  bool failed = false;
  usize failIndex = 0;
  T failOld = {};
  T failNew = {};

  for(usize offset = 0; offset < totalSize && !failed; offset += k_ChunkSize)
  {
    const usize count = std::min(k_ChunkSize, totalSize - offset);
    auto oldStoreReadResult = oldStore.copyIntoBuffer(offset, nonstd::span<T>(oldBuf.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(oldStoreReadResult);
    auto newStoreReadResult = newStore.copyIntoBuffer(offset, nonstd::span<T>(newBuf.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(newStoreReadResult);

    for(usize i = 0; i < count; i++)
    {
      if(oldBuf[i] != newBuf[i])
      {
        float32 diff = std::fabs(static_cast<float32>(oldBuf[i] - newBuf[i]));
        if(diff >= EPSILON)
        {
          failed = true;
          failIndex = offset + i;
          failOld = oldBuf[i];
          failNew = newBuf[i];
          break;
        }
      }
    }
  }

  if(failed)
  {
    UNSCOPED_INFO(fmt::format("index=: {}  oldValue != newValue. {} != {}", failIndex, failOld, failNew));
  }
  REQUIRE(!failed);
}

/**
 * @namespace FloatArrayComparisonDetail
 * @brief Contains internal test comparison support.
 */
namespace FloatArrayComparisonDetail
{
/**
 * @struct FloatArrayComparison
 * @brief Separates numerical differences from errors that prevent a complete comparison.
 * @tparam T Floating-point element type used for difference calculations.
 */
template <typename T>
struct FloatArrayComparison
{
  static_assert(std::is_floating_point_v<T>);

  bool matches = false;
  T maximumDifference = 0;
  usize maximumDifferenceIndex = 0;
  std::optional<usize> firstNanMismatchIndex;
  T exemplaryNanMismatchValue = {};
  T generatedNanMismatchValue = {};
};

/**
 * @brief Compares all values without making Catch assertions.
 * @tparam T Floating-point element type used for difference calculations.
 * @param exemplaryArray Exemplar values.
 * @param generatedArray Computed values.
 * @param epsilon Exclusive absolute-difference limit.
 * @param checkNans True to compare NaN positions; false to omit pairs that contain NaN.
 * @return Comparison diagnostics, or an error for unequal sizes or a failed read.
 */
template <typename T>
Result<FloatArrayComparison<T>> EvaluateFloatArraysWithNans(const DataArray<T>& exemplaryArray, const DataArray<T>& generatedArray, const T& epsilon, bool checkNans)
{
  static_assert(std::is_floating_point_v<T>);
  if(exemplaryArray.getNumberOfTuples() != generatedArray.getNumberOfTuples())
  {
    return MakeErrorResult<FloatArrayComparison<T>>(
        -90051, fmt::format("Floating-array comparison requires equal tuple counts: exemplar {}, computed {}.", exemplaryArray.getNumberOfTuples(), generatedArray.getNumberOfTuples()));
  }
  if(exemplaryArray.getNumberOfComponents() != generatedArray.getNumberOfComponents())
  {
    return MakeErrorResult<FloatArrayComparison<T>>(
        -90052, fmt::format("Floating-array comparison requires equal component counts: exemplar {}, computed {}.", exemplaryArray.getNumberOfComponents(), generatedArray.getNumberOfComponents()));
  }

  const auto& oldStore = exemplaryArray.getDataStoreRef();
  const auto& newStore = generatedArray.getDataStoreRef();
  const usize totalSize = oldStore.getSize();
  if(totalSize != newStore.getSize())
  {
    return MakeErrorResult<FloatArrayComparison<T>>(-90053, fmt::format("Floating-array comparison requires equal flat sizes: exemplar {}, computed {}.", totalSize, newStore.getSize()));
  }

  Result<FloatArrayComparison<T>> result;
  auto& comparison = result.value();
  constexpr usize k_ChunkSize = 40000;
  auto oldBuf = std::make_unique<T[]>(k_ChunkSize);
  auto newBuf = std::make_unique<T[]>(k_ChunkSize);

  // The full scan reports the largest difference for the V&V diagnostic.
  // Bounded bulk reads keep memory independent of the total element count.
  // The difference uses T to preserve the input array precision.
  for(usize offset = 0; offset < totalSize; offset += k_ChunkSize)
  {
    const usize count = std::min(k_ChunkSize, totalSize - offset);
    auto oldStoreReadResult = oldStore.copyIntoBuffer(offset, nonstd::span<T>(oldBuf.get(), count));
    result.warnings().insert(result.warnings().end(), oldStoreReadResult.warnings().begin(), oldStoreReadResult.warnings().end());
    if(oldStoreReadResult.invalid())
    {
      result.m_Expected = nonstd::make_unexpected(std::move(oldStoreReadResult.errors()));
      return result;
    }
    auto newStoreReadResult = newStore.copyIntoBuffer(offset, nonstd::span<T>(newBuf.get(), count));
    result.warnings().insert(result.warnings().end(), newStoreReadResult.warnings().begin(), newStoreReadResult.warnings().end());
    if(newStoreReadResult.invalid())
    {
      result.m_Expected = nonstd::make_unexpected(std::move(newStoreReadResult.errors()));
      return result;
    }

    for(usize valueIndex = 0; valueIndex < count; ++valueIndex)
    {
      const T oldVal = oldBuf[valueIndex];
      const T newVal = newBuf[valueIndex];
      const bool oldIsNan = std::isnan(oldVal);
      const bool newIsNan = std::isnan(newVal);
      if(oldIsNan || newIsNan)
      {
        if(checkNans && oldIsNan != newIsNan && !comparison.firstNanMismatchIndex.has_value())
        {
          comparison.firstNanMismatchIndex = offset + valueIndex;
          comparison.exemplaryNanMismatchValue = oldVal;
          comparison.generatedNanMismatchValue = newVal;
        }
        // Paired NaNs represent the same missing result. A one-sided NaN does not stop the full diagnostic scan.
        continue;
      }
      if(oldVal != newVal)
      {
        const T difference = std::fabs(static_cast<T>(oldVal - newVal));
        if(difference > comparison.maximumDifference)
        {
          comparison.maximumDifference = difference;
          comparison.maximumDifferenceIndex = offset + valueIndex;
        }
      }
    }
  }
  comparison.matches = !comparison.firstNanMismatchIndex.has_value() && comparison.maximumDifference < epsilon;
  return result;
}
} // namespace FloatArrayComparisonDetail

/**
 * @brief Compares two floating-point DataArray objects and reports their largest difference.
 * @tparam T Specifies the floating-point element type.
 * @param dataStructure Contains both arrays.
 * @param exemplaryDataPath Exemplar array path.
 * @param computedPath Computed array path.
 * @param epsilon Exclusive absolute-difference limit.
 * @param checkNans True to compare NaN positions; false to omit values where either array has NaN.
 */
template <typename T>
void CompareFloatArraysWithNans(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, const T& epsilon = EPSILON, bool checkNans = true)
{
  static_assert(std::is_floating_point_v<T>);
  INFO(fmt::format("Input Data Array: '{}'  Output DataArray: '{}'", exemplaryDataPath.toString(), computedPath.toString()));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(computedPath));
  const auto& exemplaryDataArray = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);
  const auto& generatedDataArray = dataStructure.getDataRefAs<DataArray<T>>(computedPath);
  auto evaluation = FloatArrayComparisonDetail::EvaluateFloatArraysWithNans(exemplaryDataArray, generatedDataArray, epsilon, checkNans);
  SIMPLNX_RESULT_REQUIRE_VALID(evaluation);
  const auto& comparison = evaluation.value();
  INFO(fmt::format("Maximum difference of {} occurs at index {} (epsilon = {})", comparison.maximumDifference, comparison.maximumDifferenceIndex, epsilon));
  if(comparison.firstNanMismatchIndex.has_value())
  {
    UNSCOPED_INFO(
        fmt::format("NaN positions differ at index {}: exemplar {}, computed {}.", *comparison.firstNanMismatchIndex, comparison.exemplaryNanMismatchValue, comparison.generatedNanMismatchValue));
  }
  REQUIRE(comparison.matches);
}

/**
 * @brief Compares two floating-point NeighborList arrays without depending on list order.
 * @tparam T Specifies the floating-point element type.
 * @param dataStructure Contains both lists.
 * @param exemplaryDataPath Exemplar NeighborList path.
 * @param computedPath Computed NeighborList path.
 * @param epsilon Exclusive absolute-difference limit.
 * @param checkNans True to compare NaN positions; false to omit values where either list has NaN.
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
          // Two NaN values represent the same missing result for this test helper.
          continue;
        }
        if(exemplaryVal != computedVal)
        {
          float32 diff = std::fabs(static_cast<float32>(exemplaryVal - computedVal));
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
 * @brief Compares two NeighborList objects without depending on list order.
 * @tparam T Specifies the list element type.
 * @param exemplaryData Non-null exemplar list.
 * @param computedData Non-null computed list.
 * @note The helper reports Boolean lists as unsupported without failing the test.
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
            float32 diff = std::fabs(static_cast<float32>(exemplaryVal - computedVal));
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
 * @brief Compares two NeighborList objects that are stored in one DataStructure.
 * @tparam T Specifies the list element type.
 * @param dataStructure Contains both lists.
 * @param exemplaryDataPath Exemplar NeighborList path.
 * @param computedPath Computed NeighborList path.
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
          float32 diff = std::fabs(static_cast<float32>(exemplaryVal - computedVal));
          INFO(fmt::format("  NeighborList {}, Index {} Exemplary Value: {} Computed Value: {}", i, j, exemplaryVal, computedVal));

          REQUIRE(diff < EPSILON);
          break;
        }
      }
    }
  }
}

/**
 * @struct CompareNeighborListsFunctor
 * @brief Adapts CompareNeighborLists for ExecuteDataFunction type dispatch.
 */
struct CompareNeighborListsFunctor
{
  /**
   * @brief Dispatches a typed NeighborList comparison.
   * @tparam T Specifies the dispatched list element type.
   * @param left Exemplar list.
   * @param right Computed list.
   */
  template <typename T>
  void operator()(const INeighborList* left, const INeighborList* right) const
  {
    CompareNeighborLists<T>(left, right);
  }
};

/**
 * @brief Requires equal values in two StringArray objects from one DataStructure.
 * @param dataStructure Contains both arrays.
 * @param exemplaryDataPath Exemplar StringArray path.
 * @param computedPath Computed StringArray path.
 */
void CompareStringArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath);

/**
 * @brief Requires equal values in two StringArray objects.
 * @param exemplar Exemplar array.
 * @param computed Computed array.
 */
void CompareStringArrays(const StringArray& exemplar, const StringArray& computed);

/**
 * @brief Compares two DynamicListArray objects with the default absolute tolerance.
 * @tparam T Specifies the list index type.
 * @tparam K Specifies the list value type.
 * @param dataStructure Contains both arrays.
 * @param exemplaryDataPath Exemplar DynamicListArray path.
 * @param computedPath Computed DynamicListArray path.
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
      float32 diff = std::fabs(static_cast<float32>(oldNumCells - newNumCells));
      REQUIRE(diff < EPSILON);
    }
    for(T j = 0; j < oldNumCells; ++j)
    {
      auto oldVal = oldEltList.cells[j];
      auto newVal = newEltList.cells[j];
      if(oldVal != newVal)
      {
        float32 diff = std::fabs(static_cast<float32>(oldVal - newVal));
        REQUIRE(diff < EPSILON);
      }
    }
  }
}

/**
 * @brief Compares two IArray objects after validating their runtime array and value types.
 * @tparam T Specifies the dispatched numeric element type.
 * @param generatedArray Non-null computed array.
 * @param exemplarArray Non-null exemplar array.
 * @note A type mismatch writes a diagnostic and returns without failing the test.
 */
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

/**
 * @brief Recursively compares supported objects in two DataStructure trees.
 * @param dataStructureA First DataStructure.
 * @param dataStructureB Second DataStructure.
 * @param parentGroup Group path to compare, or an empty path for the roots.
 *
 * The helper compares geometry metadata and typed array values. Unsupported
 * DynamicListArray, ScalarData, and AbstractMontage objects are not compared.
 */
void CompareDataStructures(const DataStructure& dataStructureA, const DataStructure& dataStructureB, const DataPath& parentGroup = DataPath{});

/**
 * @brief Creates a zero-filled DataArray with an in-memory DataStore.
 * @tparam T Specifies the primitive element type.
 * @param dataStructure Receives the array.
 * @param name Array name.
 * @param tupleShape Tuple dimensions from slowest to fastest.
 * @param componentShape Component dimensions from slowest to fastest.
 * @param parentId Identifier of the parent DataObject.
 * @return The created array, which the DataStructure owns.
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

/**
 * @brief Creates an empty NeighborList with a selected tuple count.
 * @tparam T Specifies the list element type.
 * @param dataStructure Receives the NeighborList.
 * @param name NeighborList name.
 * @param numTuples Number of lists to create.
 * @param parentId Identifier of the parent DataObject.
 * @return The created NeighborList, which the DataStructure owns.
 */
template <typename T>
NeighborList<T>* CreateTestNeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples, DataObject::IdType parentId)
{
  using NeighborListType = NeighborList<T>;
  auto* neighborList = NeighborListType::Create(dataStructure, name, {numTuples}, parentId);
  return neighborList;
}

/**
 * @brief Creates a small DataStructure that represents an EBSD data set.
 * @return The populated test DataStructure.
 */
DataStructure CreateDataStructure();

/**
 * @brief Creates scalar and three-component arrays for each supported primitive type.
 * @param tupleShape Tuple dimensions for every created array.
 * @return A DataStructure with separate scalar and three-component groups.
 */
DataStructure CreateAllPrimitiveTypes(const ShapeType& tupleShape);

/**
 * @brief Adds an ImageGeom with selected dimensions and coordinates to a DataGroup.
 * @param dataStructure Receives the ImageGeom.
 * @param imageGeomDims Geometry dimensions in {X, Y, Z} order.
 * @param spacing Voxel spacing in geometry units.
 * @param origin Geometry origin in geometry units.
 * @param dataGroup Parent group for the ImageGeom.
 * @throws std::runtime_error If the ImageGeom cannot be created.
 */
void AddImageGeometry(DataStructure& dataStructure, const SizeVec3& imageGeomDims, const FloatVec3& spacing, const FloatVec3& origin, const DataGroup& dataGroup);

/**
 * @brief Compares arrays in corresponding exemplar and computed AttributeMatrix objects.
 * @param exemplarDataStructure Contains the exemplar AttributeMatrix.
 * @param exemplarAttributeMatrix Exemplar AttributeMatrix path.
 * @param computedDataStructure Contains the computed AttributeMatrix.
 * @param computedAttributeMatrix Computed AttributeMatrix path.
 * @param allMustMatch True to require each corresponding array to exist.
 *
 * When allMustMatch is false, the helper omits a comparison if either array is
 * absent. Runtime array and value types select the applicable comparison.
 */
void CompareExemplarToGenerateAttributeMatrix(const DataStructure& exemplarDataStructure, const DataPath& exemplarAttributeMatrix, const DataStructure& computedDataStructure,
                                              const DataPath& computedAttributeMatrix, bool allMustMatch = false);

/**
 * @brief Compares generated AttributeMatrix arrays with arrays in an exemplar container.
 * @param dataStructure Contains the generated AttributeMatrix.
 * @param exemplarDataStructure Contains the exemplar arrays.
 * @param attributeMatrix Generated AttributeMatrix path.
 * @param exemplarDataContainerName Top-level exemplar group that replaces the generated path root.
 *
 * The helper omits generated arrays that have no exemplar array. Runtime array
 * and value types select the applicable comparison.
 */
void CompareExemplarToGeneratedData(const DataStructure& dataStructure, const DataStructure& exemplarDataStructure, const DataPath& attributeMatrix, const std::string& exemplarDataContainerName);

/**
 * @brief Requires equal lines in two text streams except at selected indices.
 * @param computedFile Computed text stream, positioned before its first line.
 * @param exemplarFile Exemplar text stream, positioned before its first line.
 * @param lineIndicesToSkip Zero-based line indices to omit from value comparison.
 *
 * The streams must contain the same number of lines, including omitted lines.
 */
void CompareAsciiFiles(std::ifstream& computedFile, std::ifstream& exemplarFile, const std::vector<usize>& lineIndicesToSkip);

/**
 * @brief Creates a multi-parent DataStructure graph for hierarchy tests.
 * @return The populated graph.
 *
 * The returned DataStructure has this topology:
 *
 * @code
 *     A   B          Level Zero
 *    / \ /|\
 *   H   C | F        Level One
 *  /   / \|/ \
 * N   D   E   G      Level Two
 *    / \ / \ /|\
 *   I   J   K L M    Level Three
 * @endcode
 */
DataStructure CreateComplexMultiLevelDataGraph();

/**
 * @brief Requires each child array to use its parent AttributeMatrix tuple shape.
 * @param dataStructure DataStructure to inspect.
 * @param ignoredPaths Array paths to omit from the check.
 */
void CheckArraysInheritTupleDims(const DataStructure& dataStructure, const std::vector<DataPath>& ignoredPaths = {});

/**
 * @namespace nx::core::UnitTest::Cropping
 * @brief Provides input generators and labels for crop-geometry tests.
 */
namespace Cropping
{
/**
 * @brief Converts a Boolean value to the title-case text used in test labels.
 * @param v Boolean value.
 * @return `True` or `False`.
 */
std::string BoolToString(bool v);

/**
 * @brief Converts a crop type to the identifier text used in test labels.
 * @param t Crop type.
 * @return The enumerator name, or `Unknown` for an unsupported value.
 */
std::string CropTypeToString(CropGeometryParameter::CropValues::TypeEnum t);

/**
 * @struct AxisBoundsChoices
 * @brief Stores candidate voxel and physical bounds for each geometry axis.
 */
struct AxisBoundsChoices
{
  std::vector<SizeVec2> voxelX;
  std::vector<SizeVec2> voxelY;
  std::vector<SizeVec2> voxelZ;
  std::vector<FloatVec2Type> physX;
  std::vector<FloatVec2Type> physY;
  std::vector<FloatVec2Type> physZ;
};

/**
 * @brief Generates crop values for each supported axis and bounds combination.
 * @param C Candidate voxel and physical bounds.
 * @param is2D True to omit Z-axis cropping combinations.
 * @return Crop values in deterministic flag and bounds order.
 */
std::vector<CropGeometryParameter::ValueType> GenerateAllCropValues(const AxisBoundsChoices& C, bool is2D = false);
} // namespace Cropping

} // namespace UnitTest

/**
 * @var k_SimplnxCorePluginId
 * @brief Identifies the SimplnxCore plugin during unit-test filter lookup.
 */
inline constexpr Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
/**
 * @var k_MultiThresholdObjectsId
 * @brief Identifies MultiThresholdObjectsFilter during unit-test lookup.
 */
inline constexpr Uuid k_MultiThresholdObjectsId = *Uuid::FromString("4246245e-1011-4add-8436-0af6bed19228");
/**
 * @var k_MultiThresholdObjectsFilterHandle
 * @brief Selects MultiThresholdObjectsFilter from the SimplnxCore plugin.
 */
inline const FilterHandle k_MultiThresholdObjectsFilterHandle(k_MultiThresholdObjectsId, k_SimplnxCorePluginId);
/**
 * @var k_IdentifySampleFilterId
 * @brief Identifies IdentifySampleFilter during unit-test lookup.
 */
inline constexpr Uuid k_IdentifySampleFilterId = *Uuid::FromString("94d47495-5a89-4c7f-a0ee-5ff20e6bd273");
/**
 * @var k_IdentifySampleFilterHandle
 * @brief Selects IdentifySampleFilter from the SimplnxCore plugin.
 */
inline const FilterHandle k_IdentifySampleFilterHandle(k_IdentifySampleFilterId, k_SimplnxCorePluginId);

} // namespace nx::core

/**
 * @namespace SmallIn100
 * @brief Provides compatibility helpers for the Small IN100 exemplar data.
 */
namespace SmallIn100
{
/**
 * @brief Paths omitted from tuple-shape checks for the legacy Small IN100 exemplar.
 *
 * The exemplar predates multidimensional tuple support for NeighborList and
 * StringArray objects. Their stored tuple shapes therefore cannot satisfy the
 * current AttributeMatrix contract.
 */
inline const std::vector<DataPath> k_TupleCheckIgnoredPaths{{{"MirroredXDataContainer", "CellData", "NeighborList"}},
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

/**
 * @brief Executes MultiThresholdObjectsFilter with the Small IN100 thresholds.
 *
 * Legacy exemplar files store the mask as Boolean. New filter behavior uses
 * uint8 unless the helper adds the Boolean output argument.
 *
 * @param dataStructure Contains the Small IN100 arrays and receives the mask.
 * @param filterList Creates the filter instance.
 * @param useBoolOutputType True to match the Boolean mask in legacy exemplars.
 */
void ExecuteMultiThresholdObjects(DataStructure& dataStructure, const FilterList& filterList, bool useBoolOutputType = true);

/**
 * @brief Executes IdentifySampleFilter on the Small IN100 mask.
 * @param dataStructure Contains the image geometry and mask to update.
 * @param filterList Creates the filter instance.
 */
void ExecuteIdentifySample(DataStructure& dataStructure, const FilterList& filterList);
} // namespace SmallIn100

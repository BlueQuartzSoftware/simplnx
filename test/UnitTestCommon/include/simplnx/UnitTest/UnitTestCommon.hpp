#pragma once

#include "simplnx/UnitTest/AlgorithmTestScope.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Core/Application.hpp"
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
#include <nonstd/span.hpp>

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

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
const DataPath k_DataContainerPath({k_DataContainer});

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

const DataPath k_CellEnsembleAttributeMatrixPath = k_DataContainerPath.createChildPath(k_EnsembleAttributeMatrix);
const DataPath k_CrystalStructuresArrayPath = k_CellEnsembleAttributeMatrixPath.createChildPath(k_CrystalStructures);
const DataPath k_CalculatedShiftsPath = k_DataContainerPath.createChildPath(k_CalculatedShifts);

const DataPath k_CellFeatureAttributeMatrix = k_DataContainerPath.createChildPath(k_Grain_Data);
const DataPath k_ActiveArrayPath = k_CellFeatureAttributeMatrix.createChildPath(k_ActiveName);
const DataPath k_NumCellsPath = k_CellFeatureAttributeMatrix.createChildPath(k_NumElements);
const DataPath k_FeaturePhasesPath = k_CellFeatureAttributeMatrix.createChildPath(k_Phases);

const DataPath k_CellFeatureDataPath = k_DataContainerPath.createChildPath(k_CellFeatureData);

// This path selects the top-level exemplar data container.
const DataPath k_ExemplarDataContainerPath({k_ExemplarDataContainer});

} // namespace Constants

/**
 * @namespace nx::core::UnitTest
 * @brief Provides common data builders and assertions for simplnx unit tests.
 */
namespace UnitTest
{
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
   * @brief Restores the saved storage preferences without writing a preferences file.
   */
  ~PreferencesSentinel();

  PreferencesSentinel(const PreferencesSentinel&) = delete;
  PreferencesSentinel(PreferencesSentinel&&) = delete;
  PreferencesSentinel& operator=(const PreferencesSentinel&) = delete;
  PreferencesSentinel& operator=(PreferencesSentinel&&) = delete;

private:
  nx::core::DataStorageMode m_OriginalMode;
  int64 m_OriginalSize;
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
inline void LoadPlugins()
{
  static bool pluginsLoaded = []() {
    const Result<> result = Application::GetOrCreateInstance()->loadPlugins(SIMPLNX_BUILD_DIR, true);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    return true;
  }();
  (void)pluginsLoaded;
}

/**
 * @brief Writes a DataStructure to a .dream3d file and requires success.
 * @param dataStructure DataStructure to write.
 * @param filepath Destination file path.
 */
inline void WriteTestDataStructure(const DataStructure& dataStructure, const fs::path& filepath)
{
  Pipeline pipeline;
  const Result<> result2 = DREAM3D::WriteFile(filepath, dataStructure, pipeline, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result2);
}

/**
 * @brief Requires two ImageGeom objects to have equal dimensions and compatible coordinates.
 * @param exemplarGeom Non-null exemplar geometry.
 * @param computedGeom Non-null computed geometry.
 * @param threshold Inclusive absolute tolerance for each spacing and origin component.
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
 * @brief Compares two ImageGeom objects that are stored in one DataStructure.
 * @param dataStructure Contains both geometries.
 * @param exemplaryDataPath Exemplar ImageGeom path.
 * @param computedPath Computed ImageGeom path.
 * @param threshold Inclusive absolute tolerance for each spacing and origin component.
 */
inline void CompareImageGeometry(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, float32 threshold = 0.0f)
{
  INFO(fmt::format("Comparing Image Geometries. {} and {}", exemplaryDataPath.toString(), computedPath.toString()));
  const auto* exemplarGeom = dataStructure.getDataAs<ImageGeom>(exemplaryDataPath);
  const auto* computedGeom = dataStructure.getDataAs<ImageGeom>(computedPath);
  CompareImageGeometry(exemplarGeom, computedGeom, threshold);
}

/**
 * @brief Requires two geometries to have equal type, dimensions, cells, arrays, and center.
 * @param geom1 Non-null first geometry.
 * @param geom2 Non-null second geometry.
 * @return True if all Catch2 requirements continue.
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
 * @brief Requires equal montage tile counts and a generated match for each exemplar geometry.
 * @param exemplar Exemplar montage.
 * @param generated Generated montage.
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
    oldDataStore.copyIntoBuffer(offset, nonstd::span<T>(oldBuf.get(), count));
    newDataStore.copyIntoBuffer(offset, nonstd::span<T>(newBuf.get(), count));

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
    oldDataStore.copyIntoBuffer(tStart * componentCount, nonstd::span<T>(oldBuf.get(), elemCount));
    newDataStore.copyIntoBuffer(tStart * componentCount, nonstd::span<T>(newBuf.get(), elemCount));

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
    oldStore.copyIntoBuffer(offset, nonstd::span<T>(oldBuf.get(), count));
    newStore.copyIntoBuffer(offset, nonstd::span<T>(newBuf.get(), count));

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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(computedPath));

  const auto& exemplaryDataArray = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);
  const auto& generatedDataArray = dataStructure.getDataRefAs<DataArray<T>>(computedPath);
  REQUIRE(generatedDataArray.getNumberOfTuples() == exemplaryDataArray.getNumberOfTuples());

  INFO(fmt::format("Input Data Array:'{}'  Output DataArray: '{}' bad comparison", exemplaryDataPath.toString(), computedPath.toString()));

  const auto& oldStore = exemplaryDataArray.getDataStoreRef();
  const auto& newStore = generatedDataArray.getDataStoreRef();
  const usize totalSize = oldStore.getSize();

  constexpr usize k_ChunkSize = 40000;
  auto oldBuf = std::make_unique<T[]>(k_ChunkSize);
  auto newBuf = std::make_unique<T[]>(k_ChunkSize);

  // The full scan reports the largest difference for the V&V diagnostic.
  // Bounded bulk reads keep memory independent of the total element count.
  // The difference uses T to preserve the input array precision.
  T maxDiff = 0;
  usize maxDiffIndex = 0;

  for(usize offset = 0; offset < totalSize; offset += k_ChunkSize)
  {
    const usize count = std::min(k_ChunkSize, totalSize - offset);
    oldStore.copyIntoBuffer(offset, nonstd::span<T>(oldBuf.get(), count));
    newStore.copyIntoBuffer(offset, nonstd::span<T>(newBuf.get(), count));

    for(usize i = 0; i < count; i++)
    {
      const T oldVal = oldBuf[i];
      const T newVal = newBuf[i];
      if(!checkNans && (std::isnan(newVal) || std::isnan(oldVal)))
      {
        continue;
      }
      if(std::isnan(oldVal) && std::isnan(newVal))
      {
        // Two NaN values represent the same missing result for this test helper.
        continue;
      }
      if(oldVal != newVal)
      {
        const T diff = std::fabs(static_cast<T>(oldVal - newVal));
        if(diff > maxDiff)
        {
          maxDiff = diff;
          maxDiffIndex = offset + i;
        }
      }
    }
  }
  INFO(fmt::format("Maximum difference of {} occurs at index {} (epsilon = {})", maxDiff, maxDiffIndex, epsilon));
  REQUIRE(maxDiff < epsilon);
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
 * @brief Requires equal values in two StringArray objects.
 * @param exemplar Exemplar array.
 * @param computed Computed array.
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
          // This recursive helper does not compare DynamicListArray objects.
          break;
        }
        case nx::core::DataObject::Type::ScalarData: {
          // This recursive helper reports ScalarData objects without comparing values.
          std::cout << objectA->getTypeName() << ": " << objectA->getName() << std::endl;
          break;
        }
        case nx::core::DataObject::Type::AbstractMontage: {
          // This recursive helper does not compare AbstractMontage objects.
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
inline DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroup->getId());

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.55f, 1.86});
  imageGeom->setOrigin({0.0f, 20.0f, 66.0f});
  SizeVec3 imageGeomDims = {40, 60, 80};
  imageGeom->setDimensions(imageGeomDims); // ImageGeom dimensions use {X, Y, Z} order.

  // The DataStructure owns each array. Local raw pointers remain non-owning.
  usize numComponents = 1;
  ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  Float32Array* ci_data = CreateTestDataArray<float32>(dataStructure, Constants::k_ConfidenceIndex, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = CreateTestDataArray<int32>(dataStructure, Constants::k_FeatureIds, tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = CreateTestDataArray<int32>(dataStructure, "Phases", tupleShape, {numComponents}, scanData->getId());
  UInt64Array* voxelIndices = CreateTestDataArray<uint64>(dataStructure, "Voxel Indices", tupleShape, {numComponents}, scanData->getId());

  BoolArray* conditionalArray = CreateTestDataArray<bool>(dataStructure, Constants::k_ConditionalArray, tupleShape, {1}, scanData->getId());
  conditionalArray->fill(true);

  numComponents = 3;
  UInt8Array* ipf_color_data = CreateTestDataArray<uint8>(dataStructure, "IPF Colors", tupleShape, {numComponents}, scanData->getId());
  Float32Array* euler_data = CreateTestDataArray<float32>(dataStructure, "Euler", tupleShape, {numComponents}, scanData->getId());

  // The ensemble group stores phase-level data, such as the Laue class.
  DataGroup* ensembleGroup = DataGroup::Create(dataStructure, "Phase Data", topLevelGroup->getId());
  numComponents = 1;
  usize numTuples = 2;
  Int32Array* laue_data = CreateTestDataArray<int32>(dataStructure, "Laue Class", {numTuples}, {numComponents}, ensembleGroup->getId());

  VertexGeom* vertexGeom = VertexGeom::Create(dataStructure, Constants::k_VertexGeometry, scanData->getId());
  vertexGeom->setVertices(*euler_data);

  // NeighborList<float32>* neighborList = CreateTestNeighborList<float32>(dataStructure, "Neighbor List", numTuples, scanData->getId());

  return dataStructure;
}

/**
 * @brief Creates scalar and three-component arrays for each supported primitive type.
 * @param tupleShape Tuple dimensions for every created array.
 * @return A DataStructure with separate scalar and three-component groups.
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
  // The first child group contains scalar arrays.
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

  // The second child group contains three-component arrays.
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
 * @brief Adds an ImageGeom with selected dimensions and coordinates to a DataGroup.
 * @param dataStructure Receives the ImageGeom.
 * @param imageGeomDims Geometry dimensions in {X, Y, Z} order.
 * @param spacing Voxel spacing in geometry units.
 * @param origin Geometry origin in geometry units.
 * @param dataGroup Parent group for the ImageGeom.
 * @throws std::runtime_error If the ImageGeom cannot be created.
 */
inline void AddImageGeometry(DataStructure& dataStructure, const SizeVec3& imageGeomDims, const FloatVec3& spacing, const FloatVec3& origin, const DataGroup& dataGroup)
{
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, dataGroup.getId());
  if(imageGeom == nullptr)
  {
    throw std::runtime_error("UnitTestCommon: Unable to create ImageGeom");
  }
  imageGeom->setDimensions(imageGeomDims);
  imageGeom->setSpacing(spacing);
  imageGeom->setOrigin(origin);
}

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
inline void CompareExemplarToGenerateAttributeMatrix(const DataStructure& exemplarDataStructure, const DataPath& exemplarAttributeMatrix, const DataStructure& computedDataStructure,
                                                     const DataPath& computedAttributeMatrix, bool allMustMatch = false)
{
  auto& exemplarAttrMatr = exemplarDataStructure.getDataRefAs<AttributeMatrix>(exemplarAttributeMatrix);
  // std::vector<DataPath> selectedCellArrays;

  // Each exemplar child selects the corresponding computed array by name.
  for(auto& exemplarArrayPath : exemplarAttrMatr)
  {

    DataPath exemplarDataArrayPath = exemplarAttributeMatrix.createChildPath(exemplarArrayPath.second->getName());
    DataPath computedDataArrayPath = computedAttributeMatrix.createChildPath(exemplarArrayPath.second->getName());
    INFO(fmt::format("Exemplar Array:'{}'  Computed Array: '{}'", exemplarDataArrayPath.toString(), computedDataArrayPath.toString()));

    const auto* exemplarArrayPtr = exemplarDataStructure.getDataAs<IArray>(exemplarDataArrayPath);
    const auto* computedArrayPtr = computedDataStructure.getDataAs<IArray>(computedDataArrayPath);

    // Optional mode omits arrays that are absent from either DataStructure.
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
inline void CompareExemplarToGeneratedData(const DataStructure& dataStructure, const DataStructure& exemplarDataStructure, const DataPath& attributeMatrix,
                                           const std::string& exemplarDataContainerName)
{
  auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(attributeMatrix);
  std::vector<DataPath> selectedCellArrays;

  // Snapshot the child paths before the comparison starts.
  for(auto& child : cellDataGroup)
  {
    selectedCellArrays.push_back(attributeMatrix.createChildPath(child.second->getName()));
  }

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    const auto* generatedArray = dataStructure.getDataAs<IArray>(cellArrayPath);
    // The exemplar uses a different top-level container but the same child path.
    std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
    generatedPathVector[0] = exemplarDataContainerName;
    DataPath exemplarDataArrayPath(generatedPathVector);
    const auto* exemplarArray = exemplarDataStructure.getDataAs<IArray>(exemplarDataArrayPath);

    // An absent exemplar array means that this generated array has no reference.
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

/**
 * @brief Requires equal lines in two text streams except at selected indices.
 * @param computedFile Computed text stream, positioned before its first line.
 * @param exemplarFile Exemplar text stream, positioned before its first line.
 * @param lineIndicesToSkip Zero-based line indices to omit from value comparison.
 *
 * The streams must contain the same number of lines, including omitted lines.
 */
inline void CompareAsciiFiles(std::ifstream& computedFile, std::ifstream& exemplarFile, const std::vector<usize>& lineIndicesToSkip)
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
  for(usize i = 0; i < computedLines.size(); ++i)
  {
    if(std::find(begin(lineIndicesToSkip), end(lineIndicesToSkip), i) != std::end(lineIndicesToSkip))
    {
      continue;
    }

    REQUIRE(computedLines[i] == exemplarLines[i]);
  }
}

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
inline DataStructure CreateComplexMultiLevelDataGraph()
{
  DataStructure dataStructure;

  // Create the two root groups.
  auto* groupA = DataGroup::Create(dataStructure, Constants::k_GroupAName);
  auto* groupB = DataGroup::Create(dataStructure, Constants::k_GroupBName);

  auto groupAPath = DataPath({groupA->getName()});
  auto groupBPath = DataPath({groupB->getName()});

  // Create level-one groups, including shared group C.
  auto* groupH = DataGroup::Create(dataStructure, Constants::k_GroupHName, groupA->getId());
  auto* groupC = DataGroup::Create(dataStructure, Constants::k_GroupCName, groupA->getId());
  groupB->insert(dataStructure.getSharedData(groupC->getId()));
  auto* groupF = DataGroup::Create(dataStructure, Constants::k_GroupFName, groupB->getId());

  auto groupAHPath = groupAPath.createChildPath(groupH->getName());

  auto groupACPath = groupAPath.createChildPath(groupC->getName());
  auto groupBCPath = groupBPath.createChildPath(groupC->getName());

  auto groupBFPath = groupBPath.createChildPath(groupF->getName());

  // Create level-two groups, including shared group E.
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

  // Create arrays that can have paths through more than one parent.
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

/**
 * @brief Requires each child array to use its parent AttributeMatrix tuple shape.
 * @param dataStructure DataStructure to inspect.
 * @param ignoredPaths Array paths to omit from the check.
 */
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
inline std::string BoolToString(bool v)
{
  return v ? "True" : "False";
}

/**
 * @brief Converts a crop type to the identifier text used in test labels.
 * @param t Crop type.
 * @return The enumerator name, or `Unknown` for an unsupported value.
 */
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
inline std::vector<CropGeometryParameter::ValueType> GenerateAllCropValues(const AxisBoundsChoices& C, bool is2D = false)
{
  std::vector<CropGeometryParameter::ValueType> out;

  // The first value represents the no-cropping case.
  {
    CropGeometryParameter::ValueType cv;
    cv.type = CropGeometryParameter::CropValues::TypeEnum::NoCropping;
    cv.cropX = false;
    cv.cropY = false;
    cv.cropZ = false;
    cv.is2D = is2D;
    out.push_back(cv);
  }

  // Two-dimensional cases use only X and Y combinations. Three-dimensional
  // cases use each nonempty combination of X, Y, and Z.
  const std::vector<std::tuple<bool, bool, bool>> flagOrder = is2D ? std::vector<std::tuple<bool, bool, bool>>{{false, true, false}, {true, false, false}, {true, true, false}} :
                                                                     std::vector<std::tuple<bool, bool, bool>>{{false, false, true}, {false, true, false}, {false, true, true}, {true, false, false},
                                                                                                               {true, false, true},  {true, true, false},  {true, true, true}};

  // Generate the voxel-bounds combinations first.
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

          // Two-dimensional crop values never set a Z bound.
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

  // Generate the physical-bounds combinations after the voxel combinations.
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

          // Two-dimensional crop values never set a Z bound.
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

/**
 * @var k_SimplnxCorePluginId
 * @brief Identifies the SimplnxCore plugin during unit-test filter lookup.
 */
constexpr Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
/**
 * @var k_MultiThresholdObjectsId
 * @brief Identifies MultiThresholdObjectsFilter during unit-test lookup.
 */
constexpr Uuid k_MultiThresholdObjectsId = *Uuid::FromString("4246245e-1011-4add-8436-0af6bed19228");
/**
 * @var k_MultiThresholdObjectsFilterHandle
 * @brief Selects MultiThresholdObjectsFilter from the SimplnxCore plugin.
 */
const FilterHandle k_MultiThresholdObjectsFilterHandle(k_MultiThresholdObjectsId, k_SimplnxCorePluginId);
/**
 * @var k_IdentifySampleFilterId
 * @brief Identifies IdentifySampleFilter during unit-test lookup.
 */
constexpr Uuid k_IdentifySampleFilterId = *Uuid::FromString("94d47495-5a89-4c7f-a0ee-5ff20e6bd273");
/**
 * @var k_IdentifySampleFilterHandle
 * @brief Selects IdentifySampleFilter from the SimplnxCore plugin.
 */
const FilterHandle k_IdentifySampleFilterHandle(k_IdentifySampleFilterId, k_SimplnxCorePluginId);

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

  // Preflight verifies that the exemplar contains the required threshold arrays.
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execution creates the compatibility mask and must preserve tuple shapes.
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure, k_TupleCheckIgnoredPaths);
}

/**
 * @brief Executes IdentifySampleFilter on the Small IN100 mask.
 * @param dataStructure Contains the image geometry and mask to update.
 * @param filterList Creates the filter instance.
 */
inline void ExecuteIdentifySample(DataStructure& dataStructure, const FilterList& filterList)
{
  INFO(fmt::format("Error creating Filter '{}'  ", k_IdentifySampleFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_IdentifySampleFilterHandle);
  REQUIRE(nullptr != filter);

  // These keys select the inputs used by the legacy Small IN100 workflow.
  constexpr StringLiteral k_FillHoles_Key = "fill_holes";
  constexpr StringLiteral k_ImageGeom_Key = "input_image_geometry_path";
  constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";

  Arguments args;
  args.insertOrAssign(k_FillHoles_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(k_ImageGeom_Key, std::make_any<GeometrySelectionParameter::ValueType>(nx::core::Constants::k_DataContainerPath));
  args.insertOrAssign(k_MaskArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(nx::core::Constants::k_MaskArrayPath));

  // Preflight verifies the legacy image geometry and mask paths.
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execution updates the mask and must preserve tuple shapes.
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure, k_TupleCheckIgnoredPaths);
}
} // namespace SmallIn100

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureSizes.hpp"
#include "SimplnxCore/Filters/Algorithms/ComputeFeatureSizesScanline.hpp"
#include "SimplnxCore/Filters/ComputeFeatureSizesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

using namespace nx::core;

namespace fs = std::filesystem;

/**
 * @namespace LegacyTest
 * @brief Provides array names from the Small IN100 exemplar.
 */
namespace LegacyTest
{
const std::string k_Volumes("Volumes");
const std::string k_EquivalentDiameters("EquivalentDiameters");
} // namespace LegacyTest

/**
 * @namespace Test
 * @brief Provides generated feature-size fixtures and analytical validators.
 */
namespace Test
{
constexpr usize k_OutputBlockTuples = 262144;
constexpr usize k_MultiBlockFeatureCount = k_OutputBlockTuples + 3;
constexpr int32 k_BulkWriteError = -78232;

// These names and paths define the test geometry hierarchy.
const std::string k_ImageGeomName = "Image";
const DataPath k_ImageGeomPath = DataPath({k_ImageGeomName});

const std::string k_CellAMName = "CellData";
const DataPath k_CellAMPath = k_ImageGeomPath.createChildPath(k_CellAMName);
const std::string k_FeatureIdsName = "FeatureIds";
const DataPath k_FeatureIdsPath = k_CellAMPath.createChildPath(k_FeatureIdsName);

const std::string k_FeatureAMName = "FeatureData";
const DataPath k_FeatureAMPath = k_ImageGeomPath.createChildPath(k_FeatureAMName);

// These names and paths select generated output arrays.
const std::string k_NumElementsName = "NumElements";
const DataPath k_NumElementsPath = k_FeatureAMPath.createChildPath(k_NumElementsName);
const std::string k_VolumesName = "Volumes";
const DataPath k_VolumesPath = k_FeatureAMPath.createChildPath(k_VolumesName);
const std::string k_EquivalentDiametersName = "EquivalentDiameters";
const DataPath k_EquivalentDiametersPath = k_FeatureAMPath.createChildPath(k_EquivalentDiametersName);

/**
 * @var k_RelativeTolerance
 * @brief Relative tolerance for hand-derived float32 oracle values.
 *
 * The tolerance permits several float32 ULPs from platform and TBB reduction order.
 * A larger
 * difference indicates a deviation from the analytical result.
 */
constexpr float64 k_RelativeTolerance = 1.0e-6;

/**
 * @brief Gets a required object and reports access exceptions through Catch2.
 * @tparam T Specifies the required object type.
 * @param dataStructure Contains the object.
 * @param path
 * Identifies the object.
 * @return Reference to the required object.
 */
template <typename T>
T& GetRequiredDataRef(DataStructure& dataStructure, const DataPath& path)
{
  T* dataPtr = nullptr;
  REQUIRE_NOTHROW(dataPtr = &dataStructure.getDataRefAs<T>(path));
  REQUIRE(dataPtr != nullptr);
  return *dataPtr;
}

/**
 * @brief Gets a required const object and reports access exceptions through Catch2.
 * @tparam T Specifies the required object type.
 * @param dataStructure Contains the object.
 * @param path
 * Identifies the object.
 * @return Const reference to the required object.
 */
template <typename T>
const T& GetRequiredDataRef(const DataStructure& dataStructure, const DataPath& path)
{
  const T* dataPtr = nullptr;
  REQUIRE_NOTHROW(dataPtr = &dataStructure.getDataRefAs<T>(path));
  REQUIRE(dataPtr != nullptr);
  return *dataPtr;
}

/**
 * @enum FeatureOutputGeometry
 * @brief Selects an analytical geometry for scanline output tests.
 */
enum class FeatureOutputGeometry : uint8
{
  Image2D,
  Image3D,
  RectGrid
};

/**
 * @struct BulkWriteRecord
 * @brief Records one flat bulk-write range.
 */
struct BulkWriteRecord
{
  usize Start = 0;
  usize Count = 0;

  bool operator==(const BulkWriteRecord&) const = default;
};

/**
 * @class ObservedDataStore
 * @brief Records bulk writes and can reject scalar writes or one bulk write.
 * @tparam T Specifies the stored value type.
 */
template <typename T>
class ObservedDataStore : public DataStore<T>
{
public:
  using value_type = typename DataStore<T>::value_type;

  /**
   * @brief Creates a zero-initialized observed store.
   * @param tupleCount Number of scalar tuples.
   * @param rejectScalarWrites True to throw if the algorithm writes one value.
   */
  ObservedDataStore(usize tupleCount, bool rejectScalarWrites)
  : DataStore<T>({tupleCount}, {1}, std::optional<T>{T{}})
  , m_RejectScalarWrites(rejectScalarWrites)
  {
  }

  /**
   * @brief Records or rejects one scalar write.
   * @param index Flat value index.
   * @param value Value to store.
   */
  void setValue(usize index, value_type value) override
  {
    m_ScalarWriteCount++;
    if(m_RejectScalarWrites)
    {
      throw std::runtime_error("ComputeFeatureSizesScanline used a scalar output write.");
    }
    DataStore<T>::setValue(index, value);
  }

  /**
   * @brief Records one bulk write and applies the configured test behavior.
   * @param startIndex First flat value index.
   * @param buffer Values to store.
   * @return The configured error,
   * or the in-memory store result.
   */
  [[nodiscard]] Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_BulkWrites.push_back({startIndex, buffer.size()});
    if(m_WriteObserver)
    {
      m_WriteObserver(m_BulkWrites.size());
    }
    if(m_BulkWriteError.has_value() && m_BulkWrites.size() == m_FailingBulkWrite)
    {
      return MakeErrorResult(*m_BulkWriteError, "Injected feature output bulk-write failure.");
    }
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  /**
   * @brief Selects one bulk write that returns an error.
   * @param callIndex One-based bulk-write call index.
   * @param errorCode Error code to return.
   */
  void failBulkWrite(usize callIndex, int32 errorCode)
  {
    m_FailingBulkWrite = callIndex;
    m_BulkWriteError = errorCode;
  }

  /**
   * @brief Sets a callback that runs after each bulk write is recorded.
   * @param observer Callback that receives the one-based call index.
   */
  void setWriteObserver(std::function<void(usize)> observer)
  {
    m_WriteObserver = std::move(observer);
  }

  usize getScalarWriteCount() const noexcept
  {
    return m_ScalarWriteCount;
  }

  const std::vector<BulkWriteRecord>& getBulkWrites() const noexcept
  {
    return m_BulkWrites;
  }

private:
  bool m_RejectScalarWrites = false;
  usize m_ScalarWriteCount = 0;
  usize m_FailingBulkWrite = 0;
  std::optional<int32> m_BulkWriteError;
  std::function<void(usize)> m_WriteObserver;
  std::vector<BulkWriteRecord> m_BulkWrites;
};

/**
 * @struct ObservedFeatureOutputs
 * @brief Owns the three observed feature-output stores.
 */
struct ObservedFeatureOutputs
{
  std::shared_ptr<ObservedDataStore<int32>> NumElements;
  std::shared_ptr<ObservedDataStore<float32>> Volumes;
  std::shared_ptr<ObservedDataStore<float32>> EquivalentDiameters;
};

/**
 * @class ActualOocExecutionScope
 * @brief Preserves automatic dispatch for actual OOC-store tests.
 *
 * The shared AlgorithmTestScope supports resident stores only in this branch.
 * This local
 * scope restores dispatch state and verifies the actual OOC path.
 */
class ActualOocExecutionScope
{
public:
  ActualOocExecutionScope()
  : m_OriginalForceOoc(ForceOocAlgorithm())
  , m_OriginalForceInCore(ForceInCoreAlgorithm())
  , m_OriginalCounts(GetAlgorithmPathExecutionCounts())
  {
  }

  ~ActualOocExecutionScope()
  {
    ForceOocAlgorithm() = m_OriginalForceOoc;
    ForceInCoreAlgorithm() = m_OriginalForceInCore;
    SetAlgorithmPathExecutionCounts(m_OriginalCounts);
  }

  ActualOocExecutionScope(const ActualOocExecutionScope&) = delete;
  ActualOocExecutionScope(ActualOocExecutionScope&&) noexcept = delete;
  ActualOocExecutionScope& operator=(const ActualOocExecutionScope&) = delete;
  ActualOocExecutionScope& operator=(ActualOocExecutionScope&&) noexcept = delete;

  template <typename CallableT>
  decltype(auto) execute(CallableT&& callable)
  {
    ResetAlgorithmPathExecutionCounts();
    ForceOocAlgorithm() = false;
    ForceInCoreAlgorithm() = false;
    return std::invoke(std::forward<CallableT>(callable));
  }

  void requireOocStoreExecution() const
  {
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    INFO("Dispatch counts: in-core=" << counts.InCore << ", OOC=" << counts.OutOfCore << ", OOC/resident=" << counts.OutOfCoreOnInMemoryStore << ", OOC/OOC=" << counts.OutOfCoreOnOutOfCoreStore
                                     << ", in-core/resident=" << counts.InCoreOnInMemoryStore << ", in-core/OOC=" << counts.InCoreOnOutOfCoreStore);
    REQUIRE(counts.OutOfCore > 0);
    REQUIRE(counts.InCore == 0);
    REQUIRE(counts.OutOfCoreOnOutOfCoreStore > 0);
    REQUIRE(counts.OutOfCoreOnInMemoryStore == 0);
    REQUIRE(counts.InCoreOnInMemoryStore == 0);
    REQUIRE(counts.InCoreOnOutOfCoreStore == 0);
  }

private:
  bool m_OriginalForceOoc = false;
  bool m_OriginalForceInCore = false;
  AlgorithmPathExecutionCounts m_OriginalCounts;
};

/**
 * @brief Creates a feature fixture for bounded output tests.
 * @param geometryKind Selects Image 2D, Image 3D, or RectGrid data.
 * @param numFeatures Number of feature tuples.
 * @param[in]
 * useConfiguredStore Store selection.
 * @return The populated DataStructure.
 */
DataStructure CreateFeatureOutputDataStructure(FeatureOutputGeometry geometryKind, usize numFeatures, bool useConfiguredStore)
{
  REQUIRE(numFeatures >= 7);
  DataStructure dataStructure;
  ShapeType cellShape;
  std::vector<int32> featureIdsValues;
  const int32 blockFeatureId = static_cast<int32>(numFeatures > k_OutputBlockTuples ? k_OutputBlockTuples : numFeatures - 2);
  const int32 finalFeatureId = static_cast<int32>(numFeatures - 1);

  if(geometryKind == FeatureOutputGeometry::Image2D)
  {
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
    REQUIRE(imageGeomPtr != nullptr);
    imageGeomPtr->setSpacing({2.0f, 3.0f, 5.0f});
    imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f});
    imageGeomPtr->setDimensions({3, 2, 1});
    cellShape = {1, 2, 3};
    featureIdsValues = {0, 1, 1, blockFeatureId, finalFeatureId, finalFeatureId};
  }
  else if(geometryKind == FeatureOutputGeometry::Image3D)
  {
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
    REQUIRE(imageGeomPtr != nullptr);
    imageGeomPtr->setSpacing({2.0f, 3.0f, 4.0f});
    imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f});
    imageGeomPtr->setDimensions({3, 2, 2});
    cellShape = {2, 2, 3};
    featureIdsValues = {0, 1, 1, 2, blockFeatureId, finalFeatureId, 1, 2, 2, blockFeatureId, finalFeatureId, finalFeatureId};
  }
  else
  {
    auto* rectGridGeomPtr = RectGridGeom::Create(dataStructure, k_ImageGeomName);
    REQUIRE(rectGridGeomPtr != nullptr);
    rectGridGeomPtr->setDimensions({3, 2, 2});
    cellShape = {2, 2, 3};

    auto* xBoundsPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "xBounds", {4}, {1}, rectGridGeomPtr->getId());
    auto* yBoundsPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "yBounds", {3}, {1}, rectGridGeomPtr->getId());
    auto* zBoundsPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "zBounds", {3}, {1}, rectGridGeomPtr->getId());
    REQUIRE(xBoundsPtr != nullptr);
    REQUIRE(yBoundsPtr != nullptr);
    REQUIRE(zBoundsPtr != nullptr);
    constexpr std::array<float32, 4> k_XBounds = {0.0f, 1.0f, 3.0f, 6.0f};
    constexpr std::array<float32, 3> k_YBounds = {0.0f, 4.0f, 9.0f};
    constexpr std::array<float32, 3> k_ZBounds = {0.0f, 2.0f, 5.0f};
    auto xBoundsPtrWriteResult = xBoundsPtr->getDataStoreRef().copyFromBuffer(0, k_XBounds);
    SIMPLNX_RESULT_REQUIRE_VALID(xBoundsPtrWriteResult);
    auto yBoundsPtrWriteResult = yBoundsPtr->getDataStoreRef().copyFromBuffer(0, k_YBounds);
    SIMPLNX_RESULT_REQUIRE_VALID(yBoundsPtrWriteResult);
    auto zBoundsPtrWriteResult = zBoundsPtr->getDataStoreRef().copyFromBuffer(0, k_ZBounds);
    SIMPLNX_RESULT_REQUIRE_VALID(zBoundsPtrWriteResult);
    rectGridGeomPtr->setXBoundsId(xBoundsPtr->getId());
    rectGridGeomPtr->setYBoundsId(yBoundsPtr->getId());
    rectGridGeomPtr->setZBoundsId(zBoundsPtr->getId());
    featureIdsValues = {0, 1, 1, 2, blockFeatureId, finalFeatureId, 1, 2, 2, blockFeatureId, finalFeatureId, finalFeatureId};
  }

  auto& gridGeom = GetRequiredDataRef<IGridGeometry>(dataStructure, k_ImageGeomPath);
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, k_CellAMName, cellShape, gridGeom.getId());
  REQUIRE(cellDataPtr != nullptr);
  gridGeom.setCellData(*cellDataPtr);
  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, {numFeatures}, gridGeom.getId());
  REQUIRE(featureDataPtr != nullptr);

  std::shared_ptr<Int32AbstractDataStore> featureIdsStore;
  if(useConfiguredStore)
  {
    featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FeatureIdsPath, cellShape, {1});
  }
  else
  {
    featureIdsStore = std::make_shared<Int32DataStore>(cellShape, ShapeType{1}, std::optional<int32>{0});
  }
  REQUIRE(featureIdsStore != nullptr);
  auto* featureIdsPtr = Int32Array::Create(dataStructure, k_FeatureIdsName, featureIdsStore, cellDataPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  auto featureIdsStoreWriteResult = featureIdsStore->copyFromBuffer(0, featureIdsValues);
  SIMPLNX_RESULT_REQUIRE_VALID(featureIdsStoreWriteResult);
  return dataStructure;
}

/**
 * @brief Adds observed output arrays to a feature fixture.
 * @param dataStructure Receives the output arrays.
 * @param rejectScalarWrites True to throw on a scalar output write.
 * @return
 * Shared output stores for behavior checks.
 */
ObservedFeatureOutputs AddObservedFeatureOutputs(DataStructure& dataStructure, bool rejectScalarWrites)
{
  auto& featureData = GetRequiredDataRef<AttributeMatrix>(dataStructure, k_FeatureAMPath);
  const usize numFeatures = featureData.getNumberOfTuples();
  ObservedFeatureOutputs outputs{
      std::make_shared<ObservedDataStore<int32>>(numFeatures, rejectScalarWrites),
      std::make_shared<ObservedDataStore<float32>>(numFeatures, rejectScalarWrites),
      std::make_shared<ObservedDataStore<float32>>(numFeatures, rejectScalarWrites),
  };
  REQUIRE(Int32Array::Create(dataStructure, k_NumElementsName, outputs.NumElements, featureData.getId()) != nullptr);
  REQUIRE(Float32Array::Create(dataStructure, k_VolumesName, outputs.Volumes, featureData.getId()) != nullptr);
  REQUIRE(Float32Array::Create(dataStructure, k_EquivalentDiametersName, outputs.EquivalentDiameters, featureData.getId()) != nullptr);
  return outputs;
}

/**
 * @brief Runs the scanline implementation directly.
 * @param dataStructure Contains the analytical fixture and outputs.
 * @param shouldCancel Signals cancellation.
 * @return The scanline
 * execution result.
 */
Result<> RunFeatureOutputScanline(DataStructure& dataStructure, const std::atomic_bool& shouldCancel)
{
  ComputeFeatureSizesInputValues inputValues;
  inputValues.EquivalentDiametersName = k_EquivalentDiametersName;
  inputValues.FeatureAttributeMatrixPath = k_FeatureAMPath;
  inputValues.FeatureIdsPath = k_FeatureIdsPath;
  inputValues.InputImageGeometryPath = k_ImageGeomPath;
  inputValues.NumElementsName = k_NumElementsName;
  inputValues.SaveElementSizes = false;
  inputValues.VolumesName = k_VolumesName;
  return ComputeFeatureSizesScanline(dataStructure, IFilter::MessageHandler{}, shouldCancel, &inputValues)();
}

/**
 * @brief Verifies analytical feature outputs for a generated fixture.
 * @param dataStructure Contains the generated outputs.
 * @param geometryKind Selects the expected measurements.
 *
 * @param[in] numFeatures Feature count.
 */
void ValidateFeatureOutputFixture(const DataStructure& dataStructure, FeatureOutputGeometry geometryKind, usize numFeatures)
{
  const auto& numElements = GetRequiredDataRef<Int32Array>(dataStructure, k_NumElementsPath);
  const auto& volumes = GetRequiredDataRef<Float32Array>(dataStructure, k_VolumesPath);
  const auto& equivalentDiameters = GetRequiredDataRef<Float32Array>(dataStructure, k_EquivalentDiametersPath);
  const usize blockFeatureId = numFeatures > k_OutputBlockTuples ? k_OutputBlockTuples : numFeatures - 2;
  const usize finalFeatureId = numFeatures - 1;

  CHECK(numElements[0] == 0);
  CHECK(volumes[0] == 0.0f);
  CHECK(equivalentDiameters[0] == 0.0f);

  if(geometryKind == FeatureOutputGeometry::Image2D)
  {
    CHECK(numElements[1] == 2);
    CHECK(numElements[2] == 0);
    CHECK(numElements[blockFeatureId] == 1);
    CHECK(numElements[finalFeatureId] == 2);
    CHECK(volumes[1] == Approx(60.0f));
    CHECK(volumes[2] == Approx(0.0f));
    CHECK(volumes[blockFeatureId] == Approx(30.0f));
    CHECK(volumes[finalFeatureId] == Approx(60.0f));
    CHECK(equivalentDiameters[1] == Approx(8.7403879f).epsilon(k_RelativeTolerance));
    CHECK(equivalentDiameters[blockFeatureId] == Approx(6.1803870f).epsilon(k_RelativeTolerance));
    CHECK(equivalentDiameters[finalFeatureId] == Approx(8.7403879f).epsilon(k_RelativeTolerance));
  }
  else if(geometryKind == FeatureOutputGeometry::Image3D)
  {
    CHECK(numElements[1] == 3);
    CHECK(numElements[3] == 0);
    CHECK(numElements[blockFeatureId] == 2);
    CHECK(numElements[finalFeatureId] == 3);
    CHECK(volumes[1] == Approx(72.0f));
    CHECK(volumes[3] == Approx(0.0f));
    CHECK(volumes[blockFeatureId] == Approx(48.0f));
    CHECK(volumes[finalFeatureId] == Approx(72.0f));
    CHECK(equivalentDiameters[1] == Approx(5.1615243f).epsilon(k_RelativeTolerance));
    CHECK(equivalentDiameters[blockFeatureId] == Approx(4.5090065f).epsilon(k_RelativeTolerance));
    CHECK(equivalentDiameters[finalFeatureId] == Approx(5.1615243f).epsilon(k_RelativeTolerance));
  }
  else
  {
    CHECK(numElements[1] == 3);
    CHECK(numElements[3] == 0);
    CHECK(numElements[blockFeatureId] == 2);
    CHECK(numElements[finalFeatureId] == 3);
    CHECK(volumes[1] == Approx(52.0f));
    CHECK(volumes[3] == Approx(0.0f));
    CHECK(volumes[blockFeatureId] == Approx(35.0f));
    CHECK(volumes[finalFeatureId] == Approx(105.0f));
    CHECK(equivalentDiameters[1] == Approx(4.6309304f).epsilon(k_RelativeTolerance));
    CHECK(equivalentDiameters[blockFeatureId] == Approx(4.0584154f).epsilon(k_RelativeTolerance));
    CHECK(equivalentDiameters[finalFeatureId] == Approx(5.8532476f).epsilon(k_RelativeTolerance));
  }
}

/**
 * @brief Creates a two-dimensional ImageGeom feature fixture.
 * @return The populated DataStructure.
 */
DataStructure Create2DImageDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{20.2f, 0.1f, 1.0f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{5, 5, 1}});

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{5, 5, 1}, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{4}, imageGeom->getId());

  // clang-format off
  // The following values are the analytical outputs.
  // One cell has area 2.02.
  // Feature element counts are 0 11 1 13.
  // Feature areas are 0.0 22.22 2.02 26.26.
  // Equivalent diameters are 0.0 5.319 1.6037 5.782.
  const std::array<uint8, 25> featureIdsArray = {
    1, 2, 3, 3, 3,
    1, 1, 1, 1, 1,
    1, 1, 1, 3, 3,
    3, 3, 1, 1, 3,
    3, 3, 3, 3, 3
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  return dataStructure;
}

/**
 * @brief Compares two-dimensional feature sizes with analytical values.
 * @param dataStructure Contains the generated size arrays.
 */
void Validate2DImageDataStructure(const DataStructure& dataStructure)
{
  // The following values are the analytical outputs.
  // One cell has area 2.02.
  // Feature element counts are 0 11 1 13.
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 11);
  REQUIRE(numElements.getValue(2) == 1);
  REQUIRE(numElements.getValue(3) == 13);
  // Feature areas are 0.0 22.22 2.02 26.26.
  const auto& areas = dataStructure.getDataRefAs<Float32Array>(k_VolumesPath);
  REQUIRE(areas.getValue(1) == Approx(22.220001f).epsilon(k_RelativeTolerance));
  REQUIRE(areas.getValue(2) == Approx(2.0200002f).epsilon(k_RelativeTolerance));
  REQUIRE(areas.getValue(3) == Approx(26.260002f).epsilon(k_RelativeTolerance));
  // Equivalent diameters are 0.0 5.318964 1.603728 5.78232.
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath);
  REQUIRE(equivalentDiameters.getValue(1) == Approx(5.3189644f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(2) == Approx(1.60372818f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(3) == Approx(5.7823243f).epsilon(k_RelativeTolerance));
}

/**
 * @brief Creates a three-dimensional ImageGeom feature fixture.
 * @return The populated DataStructure.
 */
DataStructure Create3DImageDataStructure()
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.2f, 0.9f, 2.1f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{5, 5, 5}});

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{5, 5, 5}, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{4}, imageGeom->getId());

  // clang-format off
  // The following values are the analytical outputs.
  // One cell has volume 2.268.
  // Feature element counts are 0 73 29 23.
  // Feature volumes are 0.0 165.564 65.772 52.164.
  // Equivalent diameters are 0.0 6.813 5.008 4.636.
  const std::array<uint8, 125> featureIdsArray = {
    1, 2, 2, 2, 2,
    1, 1, 1, 1, 1,
    1, 1, 1, 2, 2,
    2, 2, 1, 1, 2,
    2, 2, 2, 2, 2,

    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,

    3, 3, 3, 3, 1,
    1, 3, 1, 3, 3,
    2, 2, 1, 1, 1,
    2, 2, 1, 1, 3,
    2, 2, 1, 3, 3,

    3, 2, 2, 1, 1,
    3, 2, 1, 1, 1,
    3, 1, 1, 1, 1,
    3, 2, 1, 2, 1,
    3, 2, 2, 2, 2,

    3, 1, 3, 1, 1,
    1, 1, 1, 1, 1,
    3, 1, 1, 1, 3,
    1, 1, 1, 3, 1,
    3, 1, 3, 1, 3
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  return dataStructure;
}

/**
 * @brief Compares three-dimensional ImageGeom feature sizes with analytical values.
 * @param dataStructure Contains the generated size arrays.
 */
void Validate3DImageDataStructure(const DataStructure& dataStructure)
{
  // The following values are the analytical outputs.
  // One cell has volume 2.268.
  // Feature element counts are 0 73 29 23.
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 73);
  REQUIRE(numElements.getValue(2) == 29);
  REQUIRE(numElements.getValue(3) == 23);
  // Feature volumes are 0.0 165.564 65.772 52.164.
  const auto& volumes = dataStructure.getDataRefAs<Float32Array>(k_VolumesPath);
  REQUIRE(volumes.getValue(1) == Approx(165.564f).epsilon(k_RelativeTolerance));
  REQUIRE(volumes.getValue(2) == Approx(65.771995f).epsilon(k_RelativeTolerance));
  REQUIRE(volumes.getValue(3) == Approx(52.163997f).epsilon(k_RelativeTolerance));
  // Equivalent diameters are 0.0 6.81275 5.00819 4.63579.
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath);
  REQUIRE(equivalentDiameters.getValue(1) == Approx(6.8127493f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(2) == Approx(5.0081901f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(3) == Approx(4.6357936f).epsilon(k_RelativeTolerance));
}

/**
 * @brief Creates a nonuniform RectGridGeom feature fixture.
 * @return The populated DataStructure.
 */
DataStructure CreateRectGridDataStructure()
{
  DataStructure dataStructure = {};
  RectGridGeom* rectGridGeom = RectGridGeom::Create(dataStructure, k_ImageGeomName);
  rectGridGeom->setDimensions(SizeVec3{std::array<usize, 3>{4, 4, 4}});

  // X bounds are 0.0f, 0.6f, 0.9f, 2.1f, 13.0f.
  Float32Array* xBoundsArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "xBounds", ShapeType{5}, ShapeType{1}, rectGridGeom->getId());
  xBoundsArray->setValue(0, 0.0f);
  xBoundsArray->setValue(1, 0.6f);
  xBoundsArray->setValue(2, 0.9f);
  xBoundsArray->setValue(3, 2.1f);
  xBoundsArray->setValue(4, 13.0f);
  rectGridGeom->setXBoundsId(xBoundsArray->getId());

  // Y bounds are 0.0f, 0.1f, 1.0f, 10.0f, 100.0f.
  Float32Array* yBoundsArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "yBounds", ShapeType{5}, ShapeType{1}, rectGridGeom->getId());
  yBoundsArray->setValue(0, 0.0f);
  yBoundsArray->setValue(1, 0.1f);
  yBoundsArray->setValue(2, 1.0f);
  yBoundsArray->setValue(3, 10.0f);
  yBoundsArray->setValue(4, 100.0f);
  rectGridGeom->setYBoundsId(yBoundsArray->getId());

  // Z bounds are 0.0f, 1.0f, 1.2f, 2.0f, 2.1f.
  Float32Array* zBoundsArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "zBounds", ShapeType{5}, ShapeType{1}, rectGridGeom->getId());
  zBoundsArray->setValue(0, 0.0f);
  zBoundsArray->setValue(1, 1.0f);
  zBoundsArray->setValue(2, 1.2f);
  zBoundsArray->setValue(3, 2.0f);
  zBoundsArray->setValue(4, 2.1f);
  rectGridGeom->setZBoundsId(zBoundsArray->getId());

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{4, 4, 4}, rectGridGeom->getId());
  rectGridGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{4}, rectGridGeom->getId());

  // clang-format off
  // The following values are the analytical outputs.
  // Feature element counts are 0 39 15 10.
  // Feature volumes are 0.0 2362.434 352.462 15.104.
  // Equivalent diameters are 0.0 16.5242 8.76404 3.06689.
  const std::array<uint8, 64> featureIdsArray = {
    1, 2, 2, 2,
    1, 1, 1, 1,
    1, 1, 1, 2,
    2, 2, 1, 1,

    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,

    3, 3, 3, 3,
    1, 3, 1, 3,
    2, 2, 1, 1,
    2, 2, 1, 1,

    3, 2, 2, 1,
    3, 2, 1, 1,
    3, 1, 1, 1,
    3, 2, 1, 2,
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  return dataStructure;
}

/**
 * @brief Compares RectGridGeom feature sizes with analytical values.
 * @param dataStructure Contains the generated size arrays.
 */
void ValidateRectGridDataStructure(const DataStructure& dataStructure)
{
  // The following values are the analytical outputs.
  // Feature element counts are 0 39 15 10.
  const auto& numElements = dataStructure.getDataRefAs<Int32Array>(k_NumElementsPath);
  REQUIRE(numElements.getValue(1) == 39);
  REQUIRE(numElements.getValue(2) == 15);
  REQUIRE(numElements.getValue(3) == 10);
  // Feature volumes are 0.0 2362.434 352.462 15.104.
  const auto& volumes = dataStructure.getDataRefAs<Float32Array>(k_VolumesPath);
  REQUIRE(volumes.getValue(1) == Approx(2362.43384f).epsilon(k_RelativeTolerance));
  REQUIRE(volumes.getValue(2) == Approx(352.461884f).epsilon(k_RelativeTolerance));
  REQUIRE(volumes.getValue(3) == Approx(15.1039925f).epsilon(k_RelativeTolerance));
  // Equivalent diameters are 0.0 16.5242 8.76404 3.06689.
  const auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(k_EquivalentDiametersPath);
  REQUIRE(equivalentDiameters.getValue(1) == Approx(16.5241966f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(2) == Approx(8.7640428f).epsilon(k_RelativeTolerance));
  REQUIRE(equivalentDiameters.getValue(3) == Approx(3.0668866f).epsilon(k_RelativeTolerance));
}
} // namespace Test

TEST_CASE("SimplnxCore::ComputeFeatureSizesScanline: bounded output writes", "[SimplnxCore][ComputeFeatureSizes][ScanlineOutput]")
{
  const auto [geometryKind, geometryName] = GENERATE(std::make_tuple(Test::FeatureOutputGeometry::Image2D, "Image 2D"), std::make_tuple(Test::FeatureOutputGeometry::Image3D, "Image 3D"),
                                                     std::make_tuple(Test::FeatureOutputGeometry::RectGrid, "RectGrid"));

  DYNAMIC_SECTION(geometryName)
  {
    DataStructure dataStructure = Test::CreateFeatureOutputDataStructure(geometryKind, Test::k_MultiBlockFeatureCount, false);
    const Test::ObservedFeatureOutputs outputs = Test::AddObservedFeatureOutputs(dataStructure, true);
    const std::atomic_bool shouldCancel = false;

    Result<> result;
    REQUIRE_NOTHROW(result = Test::RunFeatureOutputScanline(dataStructure, shouldCancel));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    Test::ValidateFeatureOutputFixture(dataStructure, geometryKind, Test::k_MultiBlockFeatureCount);

    constexpr std::array<Test::BulkWriteRecord, 2> k_ExpectedWrites = {{{1, Test::k_OutputBlockTuples}, {Test::k_OutputBlockTuples + 1, 2}}};
    CHECK(outputs.NumElements->getBulkWrites() == std::vector<Test::BulkWriteRecord>(k_ExpectedWrites.begin(), k_ExpectedWrites.end()));
    CHECK(outputs.Volumes->getBulkWrites() == std::vector<Test::BulkWriteRecord>(k_ExpectedWrites.begin(), k_ExpectedWrites.end()));
    CHECK(outputs.EquivalentDiameters->getBulkWrites() == std::vector<Test::BulkWriteRecord>(k_ExpectedWrites.begin(), k_ExpectedWrites.end()));
    CHECK(outputs.NumElements->getScalarWriteCount() == 0);
    CHECK(outputs.Volumes->getScalarWriteCount() == 0);
    CHECK(outputs.EquivalentDiameters->getScalarWriteCount() == 0);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureSizesScanline: output write failures stop later writes", "[SimplnxCore][ComputeFeatureSizes][ScanlineOutput]")
{
  enum class FailingOutput : uint8
  {
    NumElements,
    Volumes,
    EquivalentDiameters
  };

  const auto [failingOutput, outputName] = GENERATE(std::make_tuple(FailingOutput::NumElements, "NumElements"), std::make_tuple(FailingOutput::Volumes, "Volumes"),
                                                    std::make_tuple(FailingOutput::EquivalentDiameters, "EquivalentDiameters"));

  DYNAMIC_SECTION(outputName)
  {
    DataStructure dataStructure = Test::CreateFeatureOutputDataStructure(Test::FeatureOutputGeometry::Image3D, Test::k_MultiBlockFeatureCount, false);
    const Test::ObservedFeatureOutputs outputs = Test::AddObservedFeatureOutputs(dataStructure, false);
    switch(failingOutput)
    {
    case FailingOutput::NumElements:
      outputs.NumElements->failBulkWrite(1, Test::k_BulkWriteError);
      break;
    case FailingOutput::Volumes:
      outputs.Volumes->failBulkWrite(1, Test::k_BulkWriteError);
      break;
    case FailingOutput::EquivalentDiameters:
      outputs.EquivalentDiameters->failBulkWrite(1, Test::k_BulkWriteError);
      break;
    }

    const std::atomic_bool shouldCancel = false;
    const Result<> result = Test::RunFeatureOutputScanline(dataStructure, shouldCancel);
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    CHECK(result.errors().front().code == Test::k_BulkWriteError);

    const usize expectedNumElementsWrites = 1;
    const usize expectedVolumeWrites = failingOutput == FailingOutput::NumElements ? 0 : 1;
    const usize expectedDiameterWrites = failingOutput == FailingOutput::EquivalentDiameters ? 1 : 0;
    CHECK(outputs.NumElements->getBulkWrites().size() == expectedNumElementsWrites);
    CHECK(outputs.Volumes->getBulkWrites().size() == expectedVolumeWrites);
    CHECK(outputs.EquivalentDiameters->getBulkWrites().size() == expectedDiameterWrites);
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureSizesScanline: cancellation preserves completed output blocks", "[SimplnxCore][ComputeFeatureSizes][ScanlineOutput]")
{
  DataStructure dataStructure = Test::CreateFeatureOutputDataStructure(Test::FeatureOutputGeometry::Image3D, Test::k_MultiBlockFeatureCount, false);
  const Test::ObservedFeatureOutputs outputs = Test::AddObservedFeatureOutputs(dataStructure, false);
  std::atomic_bool shouldCancel = false;
  outputs.EquivalentDiameters->setWriteObserver([&shouldCancel](usize callIndex) {
    if(callIndex == 1)
    {
      shouldCancel.store(true);
    }
  });

  const Result<> result = Test::RunFeatureOutputScanline(dataStructure, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(shouldCancel.load());

  const auto& numElements = Test::GetRequiredDataRef<Int32Array>(dataStructure, Test::k_NumElementsPath);
  const auto& volumes = Test::GetRequiredDataRef<Float32Array>(dataStructure, Test::k_VolumesPath);
  const auto& equivalentDiameters = Test::GetRequiredDataRef<Float32Array>(dataStructure, Test::k_EquivalentDiametersPath);
  CHECK(numElements[Test::k_OutputBlockTuples] == 2);
  CHECK(volumes[Test::k_OutputBlockTuples] == Approx(48.0f));
  CHECK(equivalentDiameters[Test::k_OutputBlockTuples] == Approx(4.5090065f).epsilon(Test::k_RelativeTolerance));
  CHECK(numElements[Test::k_MultiBlockFeatureCount - 1] == 0);
  CHECK(volumes[Test::k_MultiBlockFeatureCount - 1] == 0.0f);
  CHECK(equivalentDiameters[Test::k_MultiBlockFeatureCount - 1] == 0.0f);

  const std::vector<Test::BulkWriteRecord> expectedWrites = {{1, Test::k_OutputBlockTuples}};
  CHECK(outputs.NumElements->getBulkWrites() == expectedWrites);
  CHECK(outputs.Volumes->getBulkWrites() == expectedWrites);
  CHECK(outputs.EquivalentDiameters->getBulkWrites() == expectedWrites);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: real OOC output writes preserve analytical values", "[SimplnxCore][ComputeFeatureSizes][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  auto& ioCollection = DataStoreUtilities::GetIOCollection();
  if(!ioCollection.hasDataStoreCreationFunction("HDF5-OOC"))
  {
#if SIMPLNX_TEST_ALGORITHM_PATH == 1
    FAIL("The OOC-only build did not register HDF5-OOC storage.");
#endif
    WARN("HDF5-OOC storage is unavailable. The in-core build skips the real-store body.");
    return;
  }

  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  constexpr usize k_FeatureCount = 7;
  DataStructure dataStructure = Test::CreateFeatureOutputDataStructure(Test::FeatureOutputGeometry::Image3D, k_FeatureCount, true);
  const auto& featureIds = Test::GetRequiredDataRef<Int32Array>(dataStructure, Test::k_FeatureIdsPath);
  REQUIRE(featureIds.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(featureIds.getDataFormat() == "HDF5-OOC");

  ComputeFeatureSizesFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
  Test::ActualOocExecutionScope executionScope;
  const auto executeResult = executionScope.execute([&] { return filter.execute(dataStructure, args); });
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  executionScope.requireOocStoreExecution();

  const auto& numElements = Test::GetRequiredDataRef<Int32Array>(dataStructure, Test::k_NumElementsPath);
  const auto& volumes = Test::GetRequiredDataRef<Float32Array>(dataStructure, Test::k_VolumesPath);
  const auto& equivalentDiameters = Test::GetRequiredDataRef<Float32Array>(dataStructure, Test::k_EquivalentDiametersPath);
  REQUIRE(numElements.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(volumes.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(equivalentDiameters.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(numElements.getDataFormat() == "HDF5-OOC");
  REQUIRE(volumes.getDataFormat() == "HDF5-OOC");
  REQUIRE(equivalentDiameters.getDataFormat() == "HDF5-OOC");
  Test::ValidateFeatureOutputFixture(dataStructure, Test::FeatureOutputGeometry::Image3D, k_FeatureCount);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: real OOC multi-block output writes", "[SimplnxCore][ComputeFeatureSizes][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  auto& ioCollection = DataStoreUtilities::GetIOCollection();
  REQUIRE(ioCollection.hasDataStoreCreationFunction("HDF5-OOC"));

  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  DataStructure dataStructure = Test::CreateFeatureOutputDataStructure(Test::FeatureOutputGeometry::Image3D, Test::k_MultiBlockFeatureCount, true);
  const auto& featureIds = Test::GetRequiredDataRef<Int32Array>(dataStructure, Test::k_FeatureIdsPath);
  REQUIRE(featureIds.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(featureIds.getDataFormat() == "HDF5-OOC");

  ComputeFeatureSizesFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
  args.insertOrAssign(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
  Test::ActualOocExecutionScope executionScope;
  const auto executeResult = executionScope.execute([&] { return filter.execute(dataStructure, args); });
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  executionScope.requireOocStoreExecution();

  const auto& numElements = Test::GetRequiredDataRef<Int32Array>(dataStructure, Test::k_NumElementsPath);
  const auto& volumes = Test::GetRequiredDataRef<Float32Array>(dataStructure, Test::k_VolumesPath);
  const auto& equivalentDiameters = Test::GetRequiredDataRef<Float32Array>(dataStructure, Test::k_EquivalentDiametersPath);
  REQUIRE(numElements.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(volumes.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(equivalentDiameters.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  REQUIRE(numElements.getDataFormat() == "HDF5-OOC");
  REQUIRE(volumes.getDataFormat() == "HDF5-OOC");
  REQUIRE(equivalentDiameters.getDataFormat() == "HDF5-OOC");
  Test::ValidateFeatureOutputFixture(dataStructure, Test::FeatureOutputGeometry::Image3D, Test::k_MultiBlockFeatureCount);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image 2D", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create2DImageDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, 3);
    UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, std::vector<DataPath>{Test::k_FeatureAMPath.createChildPath(Test::k_VolumesName),
                                                                                                      Test::k_FeatureAMPath.createChildPath(Test::k_EquivalentDiametersName),
                                                                                                      Test::k_FeatureAMPath.createChildPath(Test::k_NumElementsName)});
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate2DImageDataStructure(dataStructure);

  // The geometry must not retain per-cell element sizes when storage is disabled.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);
  REQUIRE_FALSE(imageGeom.getElementSizesId().has_value());
  REQUIRE(imageGeom.getElementSizes() == nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// DREAM3D 6.5.171 calculates two-dimensional area from only the two nonflat spacings.
// Thus, four cells with nonflat spacings 2 and 3 have area 24.
// The current ImageGeom convention multiplies all three spacings and produces area 120.
// A flat spacing of 1 hides this difference, so this fixture uses flat spacing 5.
// It also rotates the flat axis to prevent an axis-specific implementation from passing.
// The `[!shouldfail]` tag records this known formula difference.
// An unexpected pass fails the test and requires review of the selected area contract.
TEST_CASE("SimplnxCore::ComputeFeatureSizes: 2D area excludes the flat-dimension spacing", "[SimplnxCore][ComputeFeatureSizes][2DFlatSpacing][!shouldfail]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  auto [label, dims, spacing] = GENERATE(std::make_tuple("flat Z", SizeVec3{2, 2, 1}, FloatVec3{2.0f, 3.0f, 5.0f}), std::make_tuple("flat X", SizeVec3{1, 2, 2}, FloatVec3{5.0f, 2.0f, 3.0f}),
                                         std::make_tuple("flat Y", SizeVec3{2, 1, 2}, FloatVec3{2.0f, 5.0f, 3.0f}));

  DYNAMIC_SECTION(label)
  {
    // All four cells belong to feature 1. The two nonflat spacings are always 2 and 3.
    // Each cell therefore has area 6, and the feature has area 24.
    // The flat-axis spacing is 5 and does not belong in the legacy area product.
    DataStructure dataStructure;
    auto* imageGeom = ImageGeom::Create(dataStructure, Test::k_ImageGeomName);
    imageGeom->setSpacing(spacing);
    imageGeom->setOrigin(FloatVec3{0.0f, 0.0f, 0.0f});
    imageGeom->setDimensions(dims);

    const ShapeType tupleShape{dims[2], dims[1], dims[0]};
    auto* cellData = AttributeMatrix::Create(dataStructure, Test::k_CellAMName, tupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellData);
    auto* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, Test::k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());
    featureIds->fill(1);
    AttributeMatrix::Create(dataStructure, Test::k_FeatureAMName, ShapeType{2}, imageGeom->getId());

    ComputeFeatureSizesFilter filter;
    Arguments args;
    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto& areas = dataStructure.getDataRefAs<Float32Array>(Test::k_VolumesPath);
    // The legacy oracle expects 24. The current slab calculation produces 120.
    REQUIRE(areas.getValue(1) == Approx(24.0f));

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image 2D with Element Sizes", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create2DImageDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(true));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate2DImageDataStructure(dataStructure);

  // The geometry must retain per-cell element sizes when storage is enabled.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);
  REQUIRE(imageGeom.getElementSizesId().has_value());
  REQUIRE(imageGeom.getElementSizes() != nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create3DImageDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate3DImageDataStructure(dataStructure);

  // The geometry must not retain per-cell element sizes when storage is disabled.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);
  REQUIRE_FALSE(imageGeom.getElementSizesId().has_value());
  REQUIRE(imageGeom.getElementSizes() == nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image_stack.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D with Element Size", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create3DImageDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(true));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::Validate3DImageDataStructure(dataStructure);

  // The geometry must retain per-cell element sizes when storage is enabled.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);
  REQUIRE(imageGeom.getElementSizesId().has_value());
  REQUIRE(imageGeom.getElementSizes() != nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_image_stack_w_element_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::CreateRectGridDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::ValidateRectGridDataStructure(dataStructure);

  // The geometry must not retain per-cell element sizes when storage is disabled.
  const auto& rectGridGeom = dataStructure.getDataRefAs<RectGridGeom>(Test::k_ImageGeomPath);
  REQUIRE_FALSE(rectGridGeom.getElementSizesId().has_value());
  REQUIRE(rectGridGeom.getElementSizes() == nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_rect_grid.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid with Element Size", "[SimplnxCore][ComputeFeatureSizes]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::CreateRectGridDataStructure();

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(true));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  Test::ValidateRectGridDataStructure(dataStructure);

  // The geometry must retain per-cell element sizes when storage is enabled.
  const auto& rectGridGeom = dataStructure.getDataRefAs<RectGridGeom>(Test::k_ImageGeomPath);
  REQUIRE(rectGridGeom.getElementSizesId().has_value());
  REQUIRE(rectGridGeom.getElementSizes() != nullptr);

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/valid_rect_grid_w_element_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Invalid: Execution Failure", "[SimplnxCore][ComputeFeatureSizes]")
{
  // Both the in-core (Direct) and out-of-core (Scanline) paths must surface the same execution error.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = Test::Create3DImageDataStructure();
  auto& featureIds = dataStructure.getDataRefAs<Int32Array>(Test::k_FeatureIdsPath);

  featureIds.setValue(61, 10);

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/invalid_execution.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Invalid: Preflight Failure", "[SimplnxCore][ComputeFeatureSizes]")
{
  DataStructure dataStructure = Test::Create3DImageDataStructure();
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(Test::k_ImageGeomPath);

  // A one-dimensional X extent is invalid for this filter.
  imageGeom.setDimensions(SizeVec3{std::array<usize, 3>{5, 1, 1}});
  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // A one-dimensional Y extent is invalid for this filter.
  imageGeom.setDimensions(SizeVec3{std::array<usize, 3>{1, 5, 1}});
  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // A one-dimensional Z extent is invalid for this filter.
  imageGeom.setDimensions(SizeVec3{std::array<usize, 3>{1, 1, 5}});
  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // A single-cell geometry is also invalid for this filter.
  imageGeom.setDimensions(SizeVec3{std::array<usize, 3>{1, 1, 1}});
  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(Test::k_ImageGeomPath));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Test::k_FeatureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(Test::k_FeatureAMPath));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(Test::k_VolumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(Test::k_EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(Test::k_NumElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  // The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/invalid_preflight.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::ComputeFeatureSizes: Legacy: Small IN100 Test", "[SimplnxCore][ComputeFeatureSizes]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Load the Small IN100 input before feature-size calculation.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(Constants::k_CellData);
  DataPath cellPhasesPath = cellDataPath.createChildPath(Constants::k_Phases);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath featureGroup = smallIn100Group.createChildPath(Constants::k_CellFeatureData);
  std::string volumesName = "computed_volumes";
  std::string numElementsName = "computed_NumElements";
  std::string EquivalentDiametersName = "computed_EquivalentDiameters";

  std::vector<std::string> featureNames = {LegacyTest::k_Volumes, LegacyTest::k_EquivalentDiameters, Constants::k_NumElements};

  {
    ComputeFeatureSizesFilter filter;
    Arguments args;

    args.insert(ComputeFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insert(ComputeFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insert(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(featureGroup));
    args.insert(ComputeFeatureSizesFilter::k_VolumesName_Key, std::make_any<std::string>(volumesName));
    args.insert(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key, std::make_any<std::string>(EquivalentDiametersName));
    args.insert(ComputeFeatureSizesFilter::k_NumElementsName_Key, std::make_any<std::string>(numElementsName));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare each generated feature array with its exemplar array.
  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(LegacyTest::k_Volumes);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(volumesName));
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(LegacyTest::k_EquivalentDiameters);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(EquivalentDiametersName));
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(Constants::k_NumElements);
    UnitTest::CompareArrays<int32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(numElementsName));
  }

// The optional output supports manual inspection of the feature-size arrays.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes/legacy_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureSizesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeatureSizesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureSizesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureSizesFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureSizesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeFeatureSizesFilter::k_SaveElementSizes_Key) == true);
      CHECK(args.value<DataPath>(ComputeFeatureSizesFilter::k_GeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeFeatureSizesFilter::k_EquivalentDiametersName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureSizesFilter::k_NumElementsName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureSizesFilter::k_VolumesName_Key) == "TestName");
    }
  }
}

#include "SimplnxCore/Filters/Algorithms/ComputeSurfaceAreaToVolume.hpp"
#include "SimplnxCore/Filters/Algorithms/ComputeSurfaceAreaToVolumeScanline.hpp"
#include "SimplnxCore/Filters/ComputeSurfaceAreaToVolumeFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "ComputeSurfaceAreaToVolumeTestUtils.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <memory>
#include <numbers>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_SurfaceAreaVolumeRationArrayName("SurfaceAreaVolumeRatio");
const std::string k_SphericityArrayName("Sphericity");
const std::string k_SurfaceAreaVolumeRationArrayNameNX("SurfaceAreaVolumeRatioNX");
const std::string k_SphericityArrayNameNX("SphericityNX");

constexpr usize k_Dim = 200;
constexpr usize k_TotalVoxels = k_Dim * k_Dim * k_Dim;
constexpr int32 k_NumOctantFeatures = 8;
const std::string k_GeomName = "ImageGeom";

// Resident test stores observe Scanline transfers without adding a production fault hook.
template <typename T>
class SurfaceMetricTransferStore : public DataStore<T>
{
public:
  SurfaceMetricTransferStore(usize numTuples, T initialValue)
  : DataStore<T>(numTuples, initialValue)
  {
  }

  /**
   * @brief Selects the read that returns an injected error before copying data.
   * @param readNumber One-based call number; zero disables the injected error.
   */
  void failReadOn(usize readNumber)
  {
    m_FailRead = readNumber;
  }

  /**
   * @brief Selects the write that returns an injected error before copying data.
   * @param writeNumber One-based call number; zero disables the injected error.
   */
  void failWriteOn(usize writeNumber)
  {
    m_FailWrite = writeNumber;
  }

  const std::vector<std::array<usize, 2>>& getReads() const
  {
    return m_Reads;
  }

  const std::vector<std::array<usize, 2>>& getWrites() const
  {
    return m_Writes;
  }

  Result<> copyIntoBuffer(usize offset, nonstd::span<T> buffer) const override
  {
    m_Reads.push_back({offset, buffer.size()});
    if(m_Reads.size() == m_FailRead)
    {
      return MakeErrorResult(-94801, "Injected surface metric input read failure");
    }
    return DataStore<T>::copyIntoBuffer(offset, buffer);
  }

  Result<> copyFromBuffer(usize offset, nonstd::span<const T> buffer) override
  {
    m_Writes.push_back({offset, buffer.size()});
    if(m_Writes.size() == m_FailWrite)
    {
      return MakeErrorResult(-94802, "Injected surface metric output write failure");
    }
    return DataStore<T>::copyFromBuffer(offset, buffer);
  }

private:
  mutable std::vector<std::array<usize, 2>> m_Reads;
  std::vector<std::array<usize, 2>> m_Writes;
  usize m_FailRead = 0;
  usize m_FailWrite = 0;
};

void BuildOctantWithNumCells(DataStructure& ds)
{
  const ShapeType cellTupleShape = {k_Dim, k_Dim, k_Dim};

  auto* imageGeom = ImageGeom::Create(ds, k_GeomName);
  imageGeom->setDimensions({k_Dim, k_Dim, k_Dim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, Constants::k_CellData, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath featureIdsPath = DataPath({k_GeomName, Constants::k_CellData, Constants::k_FeatureIds});
  auto fidsStore = DataStoreUtilities::CreateDataStore<int32>(ds, featureIdsPath, cellTupleShape, {1});
  auto* featureIds = DataArray<int32>::Create(ds, Constants::k_FeatureIds, fidsStore, cellAM->getId());
  auto& fidsRef = featureIds->getDataStoreRef();

  std::vector<int32> featureCounts(k_NumOctantFeatures + 1, 0);
  const usize sliceSize = k_Dim * k_Dim;
  std::vector<int32> sliceBuffer(sliceSize);
  for(usize iz = 0; iz < k_Dim; iz++)
  {
    for(usize iy = 0; iy < k_Dim; iy++)
    {
      for(usize ix = 0; ix < k_Dim; ix++)
      {
        const usize inSlice = iy * k_Dim + ix;
        const usize globalIdx = iz * sliceSize + inSlice;
        const int32 octant = static_cast<int32>((iz >= k_Dim / 2 ? 4 : 0) + (iy >= k_Dim / 2 ? 2 : 0) + (ix >= k_Dim / 2 ? 1 : 0)) + 1;
        const int32 fid = ((globalIdx * 7 + 13) % 100) < 5 ? 0 : octant;
        sliceBuffer[inSlice] = fid;
        if(fid > 0)
        {
          featureCounts[fid]++;
        }
      }
    }
    auto fidsRefWriteResult = fidsRef.copyFromBuffer(iz * sliceSize, nonstd::span<const int32>(sliceBuffer.data(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(fidsRefWriteResult);
  }

  auto* cellFeatureAM = AttributeMatrix::Create(ds, Constants::k_CellFeatureData, {static_cast<usize>(k_NumOctantFeatures + 1)}, imageGeom->getId());

  // NumCells is small (9 tuples) — per-element writes are fine
  const DataPath numCellsPath = DataPath({k_GeomName, Constants::k_CellFeatureData, Constants::k_NumElements});
  auto numCellsStore = DataStoreUtilities::CreateDataStore<int32>(ds, numCellsPath, {static_cast<usize>(k_NumOctantFeatures + 1)}, {1});
  auto* numCells = DataArray<int32>::Create(ds, Constants::k_NumElements, numCellsStore, cellFeatureAM->getId());
  auto& numCellsRef = numCells->getDataStoreRef();
  std::vector<int32> localNumCells(featureCounts.begin(), featureCounts.end());
  auto numCellsRefWriteResult = numCellsRef.copyFromBuffer(0, nonstd::span<const int32>(localNumCells.data(), localNumCells.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(numCellsRefWriteResult);
}
} // namespace

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: sparse feature table batch boundaries", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto numFeatures = GENERATE(usize{1}, usize{2}, usize{3}, usize{7}, usize{131071}, usize{131072}, usize{131073});
  const bool calculateSphericity = GENERATE(false, true);
  CAPTURE(scenario, numFeatures, calculateSphericity);
  UnitTest::AlgorithmTestScope scope(scenario);
  surface_area_to_volume_test::CheckSparseFeatureSurfaceMetrics(scope, numFeatures, calculateSphericity);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeScanline: bounded metric transfers", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  // This targets Scanline directly: the test contract is its bulk-transfer bound, not dispatch.
  const auto featureAndBatchCounts = GENERATE(std::pair<usize, usize>{0, 1}, std::pair<usize, usize>{1, 1}, std::pair<usize, usize>{2, 1}, std::pair<usize, usize>{3, 1}, std::pair<usize, usize>{7, 3},
                                              std::pair<usize, usize>{131071, 65535}, std::pair<usize, usize>{131072, 65536}, std::pair<usize, usize>{131073, 65536});
  const auto [numFeatures, batchCapacity] = featureAndBatchCounts;
  const bool calculateSphericity = GENERATE(false, true);
  CAPTURE(numFeatures, batchCapacity, calculateSphericity);
  DataStructure dataStructure;
  const auto occupiedFeatures = surface_area_to_volume_test::BuildSparseFeatureSurfaceData(dataStructure, numFeatures);
  const DataPath featurePath({"ImageGeom", "CellFeatureData"});
  const DataPath countsPath = featurePath.createChildPath("NumCells");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featurePath));
  const auto featureId = dataStructure.getDataRefAs<AttributeMatrix>(featurePath).getId();
  auto countsStorePtr = std::make_shared<SurfaceMetricTransferStore<int32>>(numFeatures, 0);
  for(const usize feature : occupiedFeatures)
  {
    countsStorePtr->setValue(feature, 1);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  const auto replacement = dataStructure.getDataRefAs<Int32Array>(countsPath).setDataStore(countsStorePtr);
  SIMPLNX_RESULT_REQUIRE_VALID(replacement);
  auto ratioStorePtr = std::make_shared<SurfaceMetricTransferStore<float32>>(numFeatures, -47.0f);
  REQUIRE(Float32Array::Create(dataStructure, "SurfaceAreaVolumeRatio", ratioStorePtr, featureId) != nullptr);
  auto sphericityStorePtr = std::make_shared<SurfaceMetricTransferStore<float32>>(numFeatures, -47.0f);
  if(calculateSphericity)
  {
    REQUIRE(Float32Array::Create(dataStructure, "Sphericity", sphericityStorePtr, featureId) != nullptr);
  }
  const ComputeSurfaceAreaToVolumeInputValues inputValues = {.FeatureIdsArrayPath = DataPath({"ImageGeom", "CellData", "FeatureIds"}),
                                                             .NumCellsArrayPath = countsPath,
                                                             .SurfaceAreaVolumeRatioArrayName = featurePath.createChildPath("SurfaceAreaVolumeRatio"),
                                                             .CalculateSphericity = calculateSphericity,
                                                             .SphericityArrayName = featurePath.createChildPath("Sphericity"),
                                                             .InputImageGeometry = DataPath({"ImageGeom"})};
  const std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler messageHandler;
  const auto result = ComputeSurfaceAreaToVolumeScanline(dataStructure, messageHandler, shouldCancel, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  std::vector<std::array<usize, 2>> expectedTransfers;
  for(usize offset = 0; offset < numFeatures; offset += batchCapacity)
  {
    expectedTransfers.push_back({offset, std::min(batchCapacity, numFeatures - offset)});
  }
  CHECK(countsStorePtr->getReads() == expectedTransfers);
  CHECK(ratioStorePtr->getWrites() == expectedTransfers);
  if(calculateSphericity)
  {
    CHECK(sphericityStorePtr->getWrites() == expectedTransfers);
  }
  else
  {
    CHECK(sphericityStorePtr->getWrites().empty());
  }
  if(numFeatures > 0)
  {
    CHECK(ratioStorePtr->getValue(0) == 0.0f);
    if(calculateSphericity)
    {
      CHECK(sphericityStorePtr->getValue(0) == 0.0f);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeScanline: later metric I/O failures preserve prior batches", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  // Exercise each failed operation on the second batch, after the first batch was written.
  const int failure = GENERATE(0, 1, 2); // NumCells read, ratio write, sphericity write.
  CAPTURE(failure);
  constexpr usize numFeatures = 7;
  constexpr float32 unwritten = -47.0f;
  DataStructure dataStructure;
  const auto occupiedFeatures = surface_area_to_volume_test::BuildSparseFeatureSurfaceData(dataStructure, numFeatures);
  const DataPath featurePath({"ImageGeom", "CellFeatureData"});
  const DataPath countsPath = featurePath.createChildPath("NumCells");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featurePath));
  const auto featureId = dataStructure.getDataRefAs<AttributeMatrix>(featurePath).getId();
  auto countsStorePtr = std::make_shared<SurfaceMetricTransferStore<int32>>(numFeatures, 0);
  countsStorePtr->failReadOn(failure == 0 ? 2 : 0);
  for(const usize feature : occupiedFeatures)
  {
    countsStorePtr->setValue(feature, 1);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  const auto replacement = dataStructure.getDataRefAs<Int32Array>(countsPath).setDataStore(countsStorePtr);
  SIMPLNX_RESULT_REQUIRE_VALID(replacement);
  auto ratioStorePtr = std::make_shared<SurfaceMetricTransferStore<float32>>(numFeatures, unwritten);
  ratioStorePtr->failWriteOn(failure == 1 ? 2 : 0);
  auto sphericityStorePtr = std::make_shared<SurfaceMetricTransferStore<float32>>(numFeatures, unwritten);
  sphericityStorePtr->failWriteOn(failure == 2 ? 2 : 0);
  REQUIRE(Float32Array::Create(dataStructure, "SurfaceAreaVolumeRatio", ratioStorePtr, featureId) != nullptr);
  REQUIRE(Float32Array::Create(dataStructure, "Sphericity", sphericityStorePtr, featureId) != nullptr);
  const ComputeSurfaceAreaToVolumeInputValues inputValues = {.FeatureIdsArrayPath = DataPath({"ImageGeom", "CellData", "FeatureIds"}),
                                                             .NumCellsArrayPath = countsPath,
                                                             .SurfaceAreaVolumeRatioArrayName = featurePath.createChildPath("SurfaceAreaVolumeRatio"),
                                                             .CalculateSphericity = true,
                                                             .SphericityArrayName = featurePath.createChildPath("Sphericity"),
                                                             .InputImageGeometry = DataPath({"ImageGeom"})};
  const std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler messageHandler;
  const auto result = ComputeSurfaceAreaToVolumeScanline(dataStructure, messageHandler, shouldCancel, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == (failure == 0 ? -94801 : -94802));
  CHECK(countsStorePtr->getReads().size() == 2);
  CHECK(ratioStorePtr->getWrites().size() == (failure == 0 ? 1 : 2));
  CHECK(sphericityStorePtr->getWrites().size() == (failure == 2 ? 2 : 1));

  const double piValue = std::acos(-1.0);
  const double sphereRadius = std::cbrt(3.0 * 30.0 / (4.0 * piValue));
  const double expectedSphericity = 4.0 * piValue * sphereRadius * sphereRadius / 62.0;
  CHECK(ratioStorePtr->getValue(0) == 0.0f);
  CHECK(sphericityStorePtr->getValue(0) == 0.0f);
  for(usize feature = 1; feature < numFeatures; ++feature)
  {
    CAPTURE(feature);
    const bool occupied = std::ranges::binary_search(occupiedFeatures, feature);
    if(feature < (failure == 2 ? 6 : 3))
    {
      if(occupied)
      {
        CHECK(ratioStorePtr->getValue(feature) == Approx(62.0 / 30.0).epsilon(1.0e-6));
      }
      else
      {
        CHECK(std::isnan(ratioStorePtr->getValue(feature)));
      }
    }
    else
    {
      CHECK(ratioStorePtr->getValue(feature) == unwritten);
    }
    if(feature < 3)
    {
      CHECK(sphericityStorePtr->getValue(feature) == Approx(expectedSphericity).epsilon(1.0e-6));
    }
    else
    {
      CHECK(sphericityStorePtr->getValue(feature) == unwritten);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: geometry and FeatureIds tuple cardinality", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto labelDimensions = GENERATE(SizeVec3{2, 2, 2}, SizeVec3{6, 5, 4}, SizeVec3{6, 5, 2});
  const auto calculateSphericity = GENERATE(false, true);
  const ShapeType labelShape = {labelDimensions[2], labelDimensions[1], labelDimensions[0]};
  const usize labelCount = labelDimensions[0] * labelDimensions[1] * labelDimensions[2];
  CAPTURE(labelCount, calculateSphericity);

  DataStructure dataStructure;
  const DataPath geomPath({"SelectedGeometry"});
  const DataPath labelGeomPath({"LabelGeometry"});
  const DataPath idsPath = labelGeomPath.createChildPath("CellData").createChildPath("FeatureIds");
  const DataPath countsPath = geomPath.createChildPath("CellFeatureData").createChildPath("NumCells");
  const DataPath ratioPath = countsPath.replaceName("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = countsPath.replaceName("Sphericity");

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 3, 5});
  auto* cellAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, "CellData", {5, 3, 4}, imageGeomPtr->getId());
  REQUIRE(cellAttributeMatrixPtr != nullptr);
  imageGeomPtr->setCellData(*cellAttributeMatrixPtr);
  auto* featureAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, "CellFeatureData", {2}, imageGeomPtr->getId());
  REQUIRE(featureAttributeMatrixPtr != nullptr);

  auto* labelGeomPtr = ImageGeom::Create(dataStructure, labelGeomPath.getTargetName());
  REQUIRE(labelGeomPtr != nullptr);
  labelGeomPtr->setDimensions(labelDimensions);
  auto* labelAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, "CellData", labelShape, labelGeomPtr->getId());
  REQUIRE(labelAttributeMatrixPtr != nullptr);
  labelGeomPtr->setCellData(*labelAttributeMatrixPtr);

  auto idsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, labelShape, {1});
  auto countsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, countsPath, {2}, {1});
  REQUIRE(idsStorePtr != nullptr);
  REQUIRE(countsStorePtr != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", idsStorePtr, labelAttributeMatrixPtr->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "NumCells", countsStorePtr, featureAttributeMatrixPtr->getId()) != nullptr);

  const ComputeSurfaceAreaToVolumeFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(idsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(countsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(calculateSphericity));

  auto preflight = filter.preflight(dataStructure, args);
  if(labelCount == 60)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    CHECK(preflight.outputActions.value().actions.size() == (calculateSphericity ? 2 : 1));
  }
  else
  {
    // An invalid output-actions result exposes no actions that could allocate outputs for the unsafe input.
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
    REQUIRE(preflight.outputActions.errors().size() == 1);
    const auto& error = preflight.outputActions.errors().front();
    CHECK(error.code == -12804);
    CHECK(error.message.find(idsPath.toString()) != std::string::npos);
    CHECK(error.message.find(geomPath.toString()) != std::string::npos);
    CHECK(error.message.find(fmt::format("{} tuples", labelCount)) != std::string::npos);
    CHECK(error.message.find("60 cells") != std::string::npos);
  }
  CHECK(dataStructure.getDataAs<IDataArray>(ratioPath) == nullptr);
  CHECK(dataStructure.getDataAs<IDataArray>(sphericityPath) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: rejects non-DataArray selections", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const bool replaceFeatureIds = GENERATE(true, false);
  const bool useStringArray = GENERATE(true, false);
  CAPTURE(replaceFeatureIds, useStringArray);

  DataStructure dataStructure;
  const DataPath geomPath({"ImageGeom"});
  const DataPath cellPath = geomPath.createChildPath("CellData");
  const DataPath featurePath = geomPath.createChildPath("CellFeatureData");
  const DataPath idsPath = cellPath.createChildPath("FeatureIds");
  const DataPath countsPath = featurePath.createChildPath("NumCells");
  const DataPath ratioPath = featurePath.createChildPath("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = featurePath.createChildPath("Sphericity");
  const ShapeType cellShape = {2, 2, 2};
  const ShapeType featureShape = {2};
  const std::vector<int32> labels(8, 1);
  const std::vector<int32> counts = {0, 8};

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 2});
  auto* cellAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, cellPath.getTargetName(), cellShape, imageGeomPtr->getId());
  REQUIRE(cellAttributeMatrixPtr != nullptr);
  imageGeomPtr->setCellData(*cellAttributeMatrixPtr);
  auto* featureAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, featurePath.getTargetName(), featureShape, imageGeomPtr->getId());
  REQUIRE(featureAttributeMatrixPtr != nullptr);
  auto idsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, cellShape, {1});
  auto countsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, countsPath, featureShape, {1});
  REQUIRE(idsStorePtr != nullptr);
  REQUIRE(countsStorePtr != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, idsPath.getTargetName(), idsStorePtr, cellAttributeMatrixPtr->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, countsPath.getTargetName(), countsStorePtr, featureAttributeMatrixPtr->getId()) != nullptr);
  const auto idsWrite = idsStorePtr->copyFromBuffer(0, nonstd::span<const int32>(labels.data(), labels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWrite);
  const auto countsWrite = countsStorePtr->copyFromBuffer(0, nonstd::span<const int32>(counts.data(), counts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsWrite);

  const auto* parentPtr = replaceFeatureIds ? cellAttributeMatrixPtr : featureAttributeMatrixPtr;
  const DataPath parentPath = replaceFeatureIds ? cellPath : featurePath;
  const auto& values = replaceFeatureIds ? labels : counts;
  const std::string malformedName = useStringArray ? "StringSelection" : "NeighborListSelection";
  const DataPath malformedPath = parentPath.createChildPath(malformedName);
  REQUIRE(values.size() == parentPtr->getNumberOfTuples());
  if(useStringArray)
  {
    std::vector<std::string> strings;
    strings.reserve(values.size());
    for(const int32 value : values)
    {
      strings.push_back(std::to_string(value));
    }
    auto* stringsPtr = StringArray::CreateWithValues(dataStructure, malformedName, parentPtr->getShape(), std::move(strings), parentPtr->getId());
    REQUIRE(stringsPtr != nullptr);
  }
  else
  {
    auto* listsPtr = NeighborList<int32>::Create(dataStructure, malformedName, parentPtr->getShape(), parentPtr->getId());
    REQUIRE(listsPtr != nullptr);
    for(usize index = 0; index < values.size(); ++index)
    {
      listsPtr->setList(static_cast<int32>(index), std::vector<int32>{values[index]});
    }
    CHECK(listsPtr->getComponentShape() == ShapeType{1});
  }

  const ComputeSurfaceAreaToVolumeFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(replaceFeatureIds ? malformedPath : idsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(replaceFeatureIds ? countsPath : malformedPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));

  IFilter::PreflightResult preflight;
  REQUIRE_NOTHROW(preflight = filter.preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  REQUIRE(preflight.outputActions.errors().size() == 1);
  const auto& error = preflight.outputActions.errors().front();
  CHECK(error.code == (replaceFeatureIds ? -12803 : -12801));
  CHECK(error.message.find(malformedPath.toString()) != std::string::npos);
  if(replaceFeatureIds)
  {
    CHECK(error.message.find(useStringArray ? "StringArray" : "NeighborList<int32>") != std::string::npos);
    CHECK(error.message.find("scalar int32 DataArray") != std::string::npos);
    CHECK(error.message.find("Select a scalar int32 DataArray") != std::string::npos);
  }
  CHECK(dataStructure.getData(ratioPath) == nullptr);
  CHECK(dataStructure.getData(sphericityPath) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: analytical interior pair", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto pairAxis = GENERATE(usize{0}, usize{1}, usize{2});
  const auto spacing = GENERATE(FloatVec3{2.0f, 3.0f, 5.0f}, FloatVec3{1.0f, 1.0f, 1.0f});
  const auto calculateSphericity = GENERATE(false, true);
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  surface_area_to_volume_test::CheckInteriorPairSurfaceMetrics(scope, pairAxis, spacing, calculateSphericity);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: analytical single-slice pair", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto pairAxis = GENERATE(usize{0}, usize{1});
  const auto calculateSphericity = GENERATE(false, true);
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  surface_area_to_volume_test::CheckInteriorPairSurfaceMetrics(scope, pairAxis, FloatVec3{2.0f, 3.0f, 5.0f}, calculateSphericity, true);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: decimal connected comb", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto spacing = GENERATE(FloatVec3{0.13f, 0.27f, 1.1f}, FloatVec3{0.13f, 0.13f, 0.13f});
  const auto calculateSphericity = GENERATE(false, true);
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  surface_area_to_volume_test::CheckConnectedCombSurfaceMetrics(scope, spacing, calculateSphericity);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: scale-invariant sphericity", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  surface_area_to_volume_test::CheckScaleInvariantSphericity(scope);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: nonpositive supplied pair counts", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto suppliedPairCount = GENERATE(int32{0}, int32{-2});
  CAPTURE(scenario, suppliedPairCount);
  UnitTest::AlgorithmTestScope scope(scenario);
  surface_area_to_volume_test::CheckInteriorPairSurfaceMetrics(scope, 0, FloatVec3{2.0f, 3.0f, 5.0f}, true, false, suppliedPairCount);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolume", "[SimplnxCore][ComputeSurfaceAreaToVolume]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Configure the filter arguments.
  {
    ComputeSurfaceAreaToVolumeFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_CellData, k_FeatureIds});
    const DataPath k_CellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
    const DataPath k_NumElementsArrayPath({k_DataContainer, k_CellFeatureData, k_NumElements});
    const DataPath k_SelectedGeometryPath({k_DataContainer});

    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(k_NumElementsArrayPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedGeometryPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>(k_SurfaceAreaVolumeRationArrayNameNX));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>(k_SphericityArrayNameNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_SurfaceAreaVolumeRationArrayName});
    DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_SurfaceAreaVolumeRationArrayNameNX});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(exemplarPath), dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    exemplarPath = DataPath({k_DataContainer, k_CellFeatureData, k_SphericityArrayName});
    calculatedPath = DataPath({k_DataContainer, k_CellFeatureData, k_SphericityArrayNameNX});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(calculatedPath));
    // The archived exemplar predates the sphericity exponent correction. Its absolute tolerance can hide small output differences; analytical cases establish the corrected exponent behavior.
    CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(exemplarPath), dataStructure.getDataRefAs<IDataArray>(calculatedPath));
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_surface_area_volume_ratio.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: Generate Large Test Dataset", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  DataStructure buildDS;
  BuildOctantWithNumCells(buildDS);

  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data";
  fs::create_directories(outputDir);
  const auto outputFile = outputDir / "compute_surface_area_to_volume_data.dream3d";
  UnitTest::WriteTestDataStructure(buildDS, outputFile);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: 200x200x200 octant features", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  BuildOctantWithNumCells(dataStructure);

  const DataPath benchGeomPath({k_GeomName});
  const DataPath benchFeatureIdsPath = benchGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);
  const DataPath benchCellFeatureAMPath = benchGeomPath.createChildPath(Constants::k_CellFeatureData);
  const DataPath benchNumCellsPath = benchCellFeatureAMPath.createChildPath(Constants::k_NumElements);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(benchFeatureIdsPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(benchFeatureIdsPath));

  {
    ComputeSurfaceAreaToVolumeFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(benchGeomPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(benchFeatureIdsPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(benchNumCellsPath));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>("SurfaceAreaVolumeRatio"));
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>("Sphericity"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Verify: SA/V ratios and sphericity should be positive for all features
  const DataPath savPath = benchCellFeatureAMPath.createChildPath("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = benchCellFeatureAMPath.createChildPath("Sphericity");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(savPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(sphericityPath));
  const auto& savRatio = dataStructure.getDataRefAs<DataArray<float32>>(savPath).getDataStoreRef();
  const auto& sphericity = dataStructure.getDataRefAs<DataArray<float32>>(sphericityPath).getDataStoreRef();
  for(int32 f = 1; f <= k_NumOctantFeatures; f++)
  {
    REQUIRE(savRatio[f] > 0.0f);
    REQUIRE(sphericity[f] > 0.0f);
    REQUIRE(sphericity[f] <= 1.0f);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
const DataPath k_MetricGeometryPath({"Surface Metrics"});
const DataPath k_MetricCellPath = k_MetricGeometryPath.createChildPath("Cell Data");
const DataPath k_MetricFeaturePath = k_MetricGeometryPath.createChildPath("Feature Data");
const DataPath k_MetricIdsPath = k_MetricCellPath.createChildPath("Feature Ids");
const DataPath k_MetricCountsPath = k_MetricFeaturePath.createChildPath("Cell Counts");
const DataPath k_MetricRatioPath = k_MetricFeaturePath.createChildPath("Analytical Ratio");
const DataPath k_MetricSphericityPath = k_MetricFeaturePath.createChildPath("Analytical Sphericity");

Arguments BuildSurfaceMetricFixture(DataStructure& dataStructure, const FloatVec3& spacing, const std::vector<int32>& featureIds, const std::vector<int32>& counts, bool calculateSphericity)
{
  REQUIRE(featureIds.size() == 125);
  auto* imageGeom = ImageGeom::Create(dataStructure, k_MetricGeometryPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({5, 5, 5});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  imageGeom->setSpacing(spacing);
  auto* cellData = AttributeMatrix::Create(dataStructure, k_MetricCellPath.getTargetName(), {5, 5, 5}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto* featureData = AttributeMatrix::Create(dataStructure, k_MetricFeaturePath.getTargetName(), {counts.size()}, imageGeom->getId());
  REQUIRE(featureData != nullptr);
  auto idsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_MetricIdsPath, {5, 5, 5}, {1});
  auto countsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_MetricCountsPath, {counts.size()}, {1});
  REQUIRE(idsStore != nullptr);
  REQUIRE(countsStore != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, k_MetricIdsPath.getTargetName(), idsStore, cellData->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, k_MetricCountsPath.getTargetName(), countsStore, featureData->getId()) != nullptr);
  const auto idsWrite = idsStore->copyFromBuffer(0, nonstd::span<const int32>(featureIds.data(), featureIds.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWrite);
  const auto countsWrite = countsStore->copyFromBuffer(0, nonstd::span<const int32>(counts.data(), counts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsWrite);

  auto args = ComputeSurfaceAreaToVolumeFilter().getDefaultArguments();
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_MetricGeometryPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MetricIdsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(k_MetricCountsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(calculateSphericity));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>(k_MetricRatioPath.getTargetName()));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>(k_MetricSphericityPath.getTargetName()));
  return args;
}

template <typename T>
std::vector<T> ReadSurfaceMetricArray(DataStructure& dataStructure, const DataPath& path, const ShapeType& tupleShape, const UnitTest::AlgorithmTestScope& scope)
{
  CAPTURE(path.toString());
  const DataArray<T>* array = nullptr;
  REQUIRE_NOTHROW(array = &dataStructure.getDataRefAs<DataArray<T>>(path));
  scope.requireExpectedStore(*array);
  REQUIRE(array->getTupleShape() == tupleShape);
  REQUIRE(array->getComponentShape() == ShapeType{1});
  REQUIRE(array->getDataType() == GetDataType<T>());
  const AttributeMatrix* parent = nullptr;
  REQUIRE_NOTHROW(parent = &dataStructure.getDataRefAs<AttributeMatrix>(path.getParent()));
  REQUIRE(array->getParentIds().size() == 1);
  REQUIRE(array->getParentIds().front() == parent->getId());
  const auto& store = array->getDataStoreRef();

  std::vector<T> values(array->getSize());
  const auto readResult = store.copyIntoBuffer(0, nonstd::span<T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  return values;
}

void RequireSurfaceMetricGeometry(DataStructure& dataStructure, const ImageGeom& imageGeom, const FloatVec3& spacing, usize featureCount)
{
  const SizeVec3 dimensions{5, 5, 5};
  const FloatVec3 origin{0.0f, 0.0f, 0.0f};
  REQUIRE(dataStructure.getData(k_MetricGeometryPath) == &imageGeom);
  REQUIRE(imageGeom.getDimensions() == dimensions);
  REQUIRE(imageGeom.getSpacing() == spacing);
  REQUIRE(imageGeom.getOrigin() == origin);
  REQUIRE(imageGeom.getCellDataPath() == k_MetricCellPath);
  const AttributeMatrix* cellData = nullptr;
  const AttributeMatrix* featureData = nullptr;
  REQUIRE_NOTHROW(cellData = &dataStructure.getDataRefAs<AttributeMatrix>(k_MetricCellPath));
  REQUIRE_NOTHROW(featureData = &dataStructure.getDataRefAs<AttributeMatrix>(k_MetricFeaturePath));
  REQUIRE(imageGeom.getCellData() == cellData);
  const ShapeType cellShape{5, 5, 5};
  REQUIRE(cellData->getShape() == cellShape);
  REQUIRE(featureData->getShape() == ShapeType{featureCount});
  REQUIRE(cellData->getParentIds().size() == 1);
  REQUIRE(cellData->getParentIds().front() == imageGeom.getId());
  REQUIRE(featureData->getParentIds().size() == 1);
  REQUIRE(featureData->getParentIds().front() == imageGeom.getId());
}
} // namespace

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: analytical interior features", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool calculateSphericity = GENERATE(false, true);
  DYNAMIC_SECTION(scenario << ", sphericity=" << calculateSphericity)
  {
    UnitTest::AlgorithmTestScope scope(scenario);
    DataStructure dataStructure;
    const FloatVec3 spacing{1.0f, 1.0f, 1.0f};
    std::vector<int32> featureIds(125, 1);
    // A positive enclosing feature separates the interior shapes from the image boundary.
    featureIds[31] = 2;
    featureIds[92] = 3;
    featureIds[93] = 3;
    const std::vector<int32> counts{0, 122, 1, 2};
    const auto args = BuildSurfaceMetricFixture(dataStructure, spacing, featureIds, counts, calculateSphericity);
    const ImageGeom* imageGeom = nullptr;
    REQUIRE_NOTHROW(imageGeom = &dataStructure.getDataRefAs<ImageGeom>(k_MetricGeometryPath));
    REQUIRE(imageGeom->getIndex(1.5f, 1.5f, 1.5f) == std::optional<usize>{31});
    REQUIRE(imageGeom->getIndex(2.5f, 3.5f, 3.5f) == std::optional<usize>{92});
    REQUIRE(imageGeom->getIndex(3.5f, 3.5f, 3.5f) == std::optional<usize>{93});
    REQUIRE(ReadSurfaceMetricArray<int32>(dataStructure, k_MetricIdsPath, {5, 5, 5}, scope) == featureIds);
    REQUIRE(ReadSurfaceMetricArray<int32>(dataStructure, k_MetricCountsPath, {4}, scope) == counts);

    ComputeSurfaceAreaToVolumeFilter filter;
    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(dataStructure.getData(k_MetricRatioPath) == nullptr);
    REQUIRE(dataStructure.getData(k_MetricSphericityPath) == nullptr);
    const auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto ratios = ReadSurfaceMetricArray<float32>(dataStructure, k_MetricRatioPath, {4}, scope);
    REQUIRE(ratios.size() == 4);
    REQUIRE(ratios[0] == 0.0f);
    // The enclosing feature has 16 internal faces; the filter omits its exterior faces.
    REQUIRE(std::abs(static_cast<float64>(ratios[1]) - 8.0 / 61.0) <= 1.0e-7);
    REQUIRE(ratios[2] == 6.0f);
    REQUIRE(ratios[3] == 5.0f);
    if(calculateSphericity)
    {
      const auto sphericities = ReadSurfaceMetricArray<float32>(dataStructure, k_MetricSphericityPath, {4}, scope);
      REQUIRE(sphericities.size() == 4);
      REQUIRE(sphericities[0] == 0.0f);
      const std::array<float64, 4> volumes{0.0, 122.0, 1.0, 2.0};
      const std::array<float64, 4> areas{0.0, 16.0, 6.0, 10.0};
      for(usize featureIdx = 1; featureIdx < volumes.size(); ++featureIdx)
      {
        CAPTURE(featureIdx);
        // The sphere of equal volume has area cbrt(36*pi*V*V).
        const float64 expectedSphericity = std::cbrt(36.0 * std::numbers::pi_v<float64> * volumes[featureIdx] * volumes[featureIdx]) / areas[featureIdx];
        // This tolerance permits float32 arithmetic and the algorithm's rounded fractional exponents.
        REQUIRE(std::abs(static_cast<float64>(sphericities[featureIdx]) - expectedSphericity) <= 5.0e-5 * expectedSphericity);
      }
    }
    else
    {
      REQUIRE(dataStructure.getData(k_MetricSphericityPath) == nullptr);
    }
    REQUIRE(ReadSurfaceMetricArray<int32>(dataStructure, k_MetricIdsPath, {5, 5, 5}, scope) == featureIds);
    REQUIRE(ReadSurfaceMetricArray<int32>(dataStructure, k_MetricCountsPath, {4}, scope) == counts);
    RequireSurfaceMetricGeometry(dataStructure, *imageGeom, spacing, counts.size());
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

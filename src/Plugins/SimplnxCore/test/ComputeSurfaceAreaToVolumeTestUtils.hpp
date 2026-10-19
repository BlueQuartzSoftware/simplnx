#pragma once

#include "SimplnxCore/Filters/ComputeSurfaceAreaToVolumeFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace surface_area_to_volume_test
{
/**
 * @brief Builds isolated cells that exercise large feature tables in a small image.
 * @param dataStructure Receives the geometry, labels, and supplied cell counts.
 * @param numFeatures Number of feature tuples, including background. Zero creates empty inputs.
 * @return Sorted feature identifiers for the isolated one-cell boxes.
 */
inline std::vector<nx::core::usize> BuildSparseFeatureSurfaceData(nx::core::DataStructure& dataStructure, nx::core::usize numFeatures)
{
  using namespace nx::core;
  const DataPath geomPath({"ImageGeom"});
  const DataPath cellPath = geomPath.createChildPath("CellData");
  const DataPath featurePath = geomPath.createChildPath("CellFeatureData");
  const DataPath idsPath = cellPath.createChildPath("FeatureIds");
  const DataPath countsPath = featurePath.createChildPath("NumCells");
  const ShapeType cellShape = numFeatures == 0 ? ShapeType{0, 0, 0} : ShapeType{3, 3, 19};
  const ShapeType featureShape = {numFeatures};
  auto* geomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({cellShape[2], cellShape[1], cellShape[0]});
  geomPtr->setSpacing({2.0f, 3.0f, 5.0f});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, cellPath.getTargetName(), cellShape, geomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  geomPtr->setCellData(*cellAMPtr);
  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, featurePath.getTargetName(), featureShape, geomPtr->getId());
  REQUIRE(featureAMPtr != nullptr);

  auto idsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, cellShape, {1});
  auto countsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, countsPath, featureShape, {1});
  REQUIRE(idsStorePtr != nullptr);
  REQUIRE(countsStorePtr != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, idsPath.getTargetName(), idsStorePtr, cellAMPtr->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, countsPath.getTargetName(), countsStorePtr, featureAMPtr->getId()) != nullptr);
  if(numFeatures == 0)
  {
    return {};
  }

  // Probe both sides of the half-table and fixed-cap boundaries, plus the last tuple.
  std::vector<usize> occupiedFeatures = {1, numFeatures / 2, (numFeatures / 2) + 1, numFeatures - 1, 65535, 65536, 65537};
  if(numFeatures > 3)
  {
    occupiedFeatures.push_back((numFeatures / 2) - 1);
  }
  std::erase_if(occupiedFeatures, [numFeatures](usize feature) { return feature == 0 || feature >= numFeatures; });
  std::ranges::sort(occupiedFeatures);
  occupiedFeatures.erase(std::ranges::unique(occupiedFeatures).begin(), occupiedFeatures.end());
  REQUIRE(occupiedFeatures.size() <= 8);
  std::vector<int32> labels(usize{19} * 3 * 3, 0);
  std::vector<int32> counts(numFeatures, 0);
  for(usize index = 0; index < occupiedFeatures.size(); ++index)
  {
    // Background separates every one-cell box from its neighbors and the image exterior.
    labels[(((1 * 3) + 1) * 19) + 1 + (2 * index)] = static_cast<int32>(occupiedFeatures[index]);
    counts[occupiedFeatures[index]] = 1;
  }
  counts[0] = static_cast<int32>(labels.size() - occupiedFeatures.size());
  const auto idsWrite = idsStorePtr->copyFromBuffer(0, nonstd::span<const int32>(labels.data(), labels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWrite);
  const auto countsWrite = countsStorePtr->copyFromBuffer(0, nonstd::span<const int32>(counts.data(), counts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsWrite);
  return occupiedFeatures;
}

/**
 * @brief Checks all sparse-table metrics against box and equal-volume sphere geometry.
 * @tparam ScopeT Verifies the selected algorithm and actual array storage.
 * @param scope Active algorithm and storage scenario.
 * @param numFeatures Number of feature tuples, including background. Must be positive.
 * @param calculateSphericity Enables the optional sphere-area ratio check.
 */
template <class ScopeT>
void CheckSparseFeatureSurfaceMetrics(ScopeT& scope, nx::core::usize numFeatures, bool calculateSphericity)
{
  using namespace nx::core;
  REQUIRE(numFeatures > 0);
  DataStructure dataStructure;
  const auto occupiedFeatures = BuildSparseFeatureSurfaceData(dataStructure, numFeatures);
  const DataPath geomPath({"ImageGeom"});
  const DataPath idsPath({"ImageGeom", "CellData", "FeatureIds"});
  const DataPath featurePath({"ImageGeom", "CellFeatureData"});
  const DataPath countsPath = featurePath.createChildPath("NumCells");
  const DataPath ratioPath = featurePath.createChildPath("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = featurePath.createChildPath("Sphericity");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(idsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(idsPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(countsPath));

  const ComputeSurfaceAreaToVolumeFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(idsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(countsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(calculateSphericity));
  const auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(ratioPath));
  const auto& ratios = dataStructure.getDataRefAs<Float32Array>(ratioPath);
  scope.requireExpectedStore(ratios);
  CHECK(ratios.getTupleShape() == ShapeType{numFeatures});
  CHECK(ratios.getComponentShape() == ShapeType{1});
  std::vector<float32> values(numFeatures);
  const auto ratioRead = ratios.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(ratioRead);
  CHECK(values[0] == 0.0f);
  // An isolated 2-by-3-by-5 box has area 62 and volume 30.
  for(usize feature = 1; feature < numFeatures; ++feature)
  {
    CAPTURE(feature);
    if(std::ranges::binary_search(occupiedFeatures, feature))
    {
      REQUIRE(values[feature] == Approx(62.0 / 30.0).epsilon(1.0e-6));
    }
    else
    {
      REQUIRE(std::isnan(values[feature]));
    }
  }
  if(calculateSphericity)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(sphericityPath));
    const auto& sphericity = dataStructure.getDataRefAs<Float32Array>(sphericityPath);
    scope.requireExpectedStore(sphericity);
    CHECK(sphericity.getTupleShape() == ShapeType{numFeatures});
    CHECK(sphericity.getComponentShape() == ShapeType{1});
    const auto sphericityRead = sphericity.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(sphericityRead);
    CHECK(values[0] == 0.0f);
    // Independently calculate the equal-volume sphere from its radius.
    const double piValue = std::acos(-1.0);
    const double radius = std::cbrt(3.0 * 30.0 / (4.0 * piValue));
    const double expected = 4.0 * piValue * radius * radius / 62.0;
    for(usize feature = 1; feature < numFeatures; ++feature)
    {
      CAPTURE(feature);
      if(std::ranges::binary_search(occupiedFeatures, feature))
      {
        REQUIRE(values[feature] == Approx(expected).epsilon(1.0e-6));
      }
      else
      {
        REQUIRE(std::isnan(values[feature]));
      }
    }
  }
  else
  {
    CHECK(dataStructure.getData(sphericityPath) == nullptr);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// A wrong X/Y face weight changes the physical area of this interior box.
template <class ScopeT>
std::array<nx::core::float32, 3> CheckInteriorPairSurfaceMetrics(ScopeT& scope, nx::core::usize pairAxis, const nx::core::FloatVec3& spacing, bool calculateSphericity, bool singleSlice = false,
                                                                 nx::core::int32 suppliedPairCount = 2)
{
  using namespace nx::core;
  const SizeVec3 dimensions = singleSlice ? SizeVec3{5, 5, 1} : SizeVec3{5, 5, 5};
  DataStructure dataStructure;
  const DataPath geomPath({"ImageGeom"});
  const DataPath cellPath = geomPath.createChildPath("CellData");
  const DataPath featurePath = geomPath.createChildPath("CellFeatureData");
  const DataPath idsPath = cellPath.createChildPath("FeatureIds");
  const DataPath countsPath = featurePath.createChildPath("NumCells");
  const DataPath ratioPath = featurePath.createChildPath("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = featurePath.createChildPath("Sphericity");
  const ShapeType cellShape = {dimensions[2], dimensions[1], dimensions[0]};
  const ShapeType featureShape = {3};
  const ShapeType componentShape = {1};
  const FloatVec3 origin = {-1.0f, 4.0f, 7.0f};
  REQUIRE(pairAxis < (singleSlice ? 2 : 3));
  constexpr usize pairFeature = 2;
  constexpr int32 enclosingFeature = 1;
  const usize cellCount = dimensions[0] * dimensions[1] * dimensions[2];

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions(dimensions);
  imageGeomPtr->setSpacing(spacing);
  imageGeomPtr->setOrigin(origin);
  auto* cellAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  REQUIRE(cellAttributeMatrixPtr != nullptr);
  imageGeomPtr->setCellData(*cellAttributeMatrixPtr);
  auto* featureAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, "CellFeatureData", featureShape, imageGeomPtr->getId());
  REQUIRE(featureAttributeMatrixPtr != nullptr);

  auto idsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, cellShape, componentShape);
  auto countsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, countsPath, featureShape, componentShape);
  REQUIRE(idsStorePtr != nullptr);
  REQUIRE(countsStorePtr != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", idsStorePtr, cellAttributeMatrixPtr->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "NumCells", countsStorePtr, featureAttributeMatrixPtr->getId()) != nullptr);
  std::vector<int32> labels(cellCount, enclosingFeature);
  const std::array<usize, 3> first = {2, 2, singleSlice ? usize{0} : usize{2}};
  auto second = first;
  second.at(pairAxis)++;
  labels[(((first[2] * dimensions[1]) + first[1]) * dimensions[0]) + first[0]] = static_cast<int32>(pairFeature);
  labels[(((second[2] * dimensions[1]) + second[1]) * dimensions[0]) + second[0]] = static_cast<int32>(pairFeature);
  const std::array<int32, 3> counts = {0, singleSlice ? 23 : 123, suppliedPairCount};
  auto idsWrite = idsStorePtr->copyFromBuffer(0, nonstd::span<const int32>(labels.data(), labels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWrite);
  auto countsWrite = countsStorePtr->copyFromBuffer(0, nonstd::span<const int32>(counts.data(), counts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsWrite);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(idsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(idsPath);
  const auto& numCells = dataStructure.getDataRefAs<Int32Array>(countsPath);
  scope.requireExpectedStore(ids);
  scope.requireExpectedStore(numCells);
  const auto idsFormat = ids.getDataFormat();
  const auto countsFormat = numCells.getDataFormat();

  const ComputeSurfaceAreaToVolumeFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(idsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(countsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(calculateSphericity));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>("SurfaceAreaVolumeRatio"));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>("Sphericity"));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  // Independent rectangular-box oracle: the pair doubles one physical side.
  std::array<double, 3> sides = {spacing[0], spacing[1], spacing[2]};
  sides.at(pairAxis) *= 2.0;
  // Exterior Z faces are excluded for a single slice, as in the production boundary policy.
  const double area = 2.0 * ((singleSlice ? 0.0 : sides[0] * sides[1]) + (sides[0] * sides[2]) + (sides[1] * sides[2]));
  const double voxelVolume = static_cast<double>(spacing[0]) * spacing[1] * spacing[2];
  std::array<double, 3> volumes = {0.0, 0.0, 0.0};
  volumes[pairFeature] = suppliedPairCount * voxelVolume;
  volumes[enclosingFeature] = (singleSlice ? 23.0 : 123.0) * voxelVolume;
  CAPTURE(pairAxis, spacing[0], spacing[1], spacing[2], calculateSphericity, singleSlice, suppliedPairCount, area);
  REQUIRE(area > 0.0);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(ratioPath));
  const auto& ratios = dataStructure.getDataRefAs<Float32Array>(ratioPath);
  scope.requireExpectedStore(ratios);
  CHECK(ratios.getTupleShape() == featureShape);
  CHECK(ratios.getComponentShape() == componentShape);
  std::array<float32, 3> ratioValues = {};
  auto ratiosRead = ratios.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(ratioValues.data(), ratioValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(ratiosRead);
  CHECK(ratioValues[0] == 0.0f);
  for(usize feature = 1; feature < 3; feature++)
  {
    CAPTURE(feature);
    if(volumes.at(feature) == 0.0)
    {
      CHECK(std::isinf(ratioValues.at(feature)));
      CHECK(ratioValues.at(feature) > 0.0f);
    }
    else
    {
      CHECK(ratioValues.at(feature) == Approx(area / volumes.at(feature)).epsilon(1.0e-6));
    }
  }
  std::array<float32, 3> sphericityValues = {};
  if(calculateSphericity)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(sphericityPath));
    const auto& sphericity = dataStructure.getDataRefAs<Float32Array>(sphericityPath);
    scope.requireExpectedStore(sphericity);
    CHECK(sphericity.getTupleShape() == featureShape);
    CHECK(sphericity.getComponentShape() == componentShape);
    auto sphericityRead = sphericity.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(sphericityValues.data(), sphericityValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(sphericityRead);
    CHECK(sphericityValues[0] == 0.0f);
    for(usize feature = 1; feature < 3; feature++)
    {
      CAPTURE(feature);
      if(volumes.at(feature) < 0.0)
      {
        CHECK(std::isnan(sphericityValues.at(feature)));
      }
      else if(volumes.at(feature) == 0.0)
      {
        CHECK(sphericityValues.at(feature) == 0.0f);
      }
      else
      {
        // Derive the equal-volume sphere's area from its radius independently of the implementation formula.
        const double piValue = std::acos(-1.0);
        const double sphereRadius = std::cbrt(3.0 * volumes.at(feature) / (4.0 * piValue));
        const double expected = 4.0 * piValue * sphereRadius * sphereRadius / area;
        CHECK(sphericityValues.at(feature) == Approx(expected).epsilon(1.0e-6));
      }
    }
  }
  else
  {
    CHECK(dataStructure.getDataAs<IDataArray>(sphericityPath) == nullptr);
  }

  std::vector<int32> actualLabels(cellCount);
  std::array<int32, 3> actualCounts = {};
  auto idsRead = ids.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actualLabels.data(), actualLabels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsRead);
  auto countsRead = numCells.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actualCounts.data(), actualCounts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsRead);
  CHECK(actualLabels == labels);
  CHECK(actualCounts == counts);
  CHECK(ids.getTupleShape() == cellShape);
  CHECK(ids.getComponentShape() == componentShape);
  CHECK(numCells.getTupleShape() == featureShape);
  CHECK(numCells.getComponentShape() == componentShape);
  CHECK(ids.getDataFormat() == idsFormat);
  CHECK(numCells.getDataFormat() == countsFormat);
  CHECK(&ids.getDataStoreRef() == idsStorePtr.get());
  CHECK(&numCells.getDataStoreRef() == countsStorePtr.get());
  scope.requireExpectedStore(ids);
  scope.requireExpectedStore(numCells);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geomPath));
  const auto& finalGeom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  CHECK(finalGeom.getDimensions() == dimensions);
  CHECK(finalGeom.getSpacing() == spacing);
  CHECK(finalGeom.getOrigin() == origin);
  CHECK(cellAttributeMatrixPtr->getShape() == cellShape);
  CHECK(featureAttributeMatrixPtr->getShape() == featureShape);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
  return sphericityValues;
}

template <class ScopeT>
void CheckScaleInvariantSphericity(ScopeT& scope)
{
  using namespace nx::core;
  const auto reference = CheckInteriorPairSurfaceMetrics(scope, 0, FloatVec3{2.0f, 3.0f, 5.0f}, true);
  for(const int exponent : {-20, 20})
  {
    CAPTURE(exponent);
    const FloatVec3 spacing = {std::ldexp(2.0f, exponent), std::ldexp(3.0f, exponent), std::ldexp(5.0f, exponent)};
    // The pair helper independently checks each scale against the equal-volume sphere oracle.
    const auto scaled = CheckInteriorPairSurfaceMetrics(scope, 0, spacing, true);
    for(usize feature = 1; feature < reference.size(); feature++)
    {
      CAPTURE(feature);
      CHECK(scaled.at(feature) == Approx(reference.at(feature)).epsilon(1.0e-6));
    }
  }
}
// A connected plate with long, separated teeth exposes repeated decimal-area accumulation.
template <class ScopeT>
void CheckConnectedCombSurfaceMetrics(ScopeT& scope, const nx::core::FloatVec3& spacing, bool calculateSphericity)
{
  using namespace nx::core;
  DataStructure dataStructure;
  const SizeVec3 dimensions = {137, 113, 37};
  const ShapeType cellShape = {dimensions[2], dimensions[1], dimensions[0]};
  const ShapeType featureShape = {2};
  const ShapeType componentShape = {1};
  const DataPath geomPath({"ImageGeom"});
  const DataPath cellPath = geomPath.createChildPath("CellData");
  const DataPath featurePath = geomPath.createChildPath("CellFeatureData");
  const DataPath idsPath = cellPath.createChildPath("FeatureIds");
  const DataPath countsPath = featurePath.createChildPath("NumCells");
  const DataPath ratioPath = featurePath.createChildPath("SurfaceAreaVolumeRatio");
  const DataPath sphericityPath = featurePath.createChildPath("Sphericity");
  const FloatVec3 origin = {-1.0f, 4.0f, 7.0f};
  const usize cellCount = dimensions[0] * dimensions[1] * dimensions[2];

  // The x=1 backing plate is 111 by 35. Each odd (y,z) location extends a tooth
  // another 134 cells along X. All teeth connect through the plate and the entire
  // comb has a one-cell background border, so no exterior-domain faces enter the oracle.
  constexpr int32 plateY = 111;
  constexpr int32 plateZ = 35;
  constexpr int32 toothLength = 134;
  constexpr int32 teethY = 56;
  constexpr int32 teethZ = 18;
  constexpr int32 combCellCount = (plateY * plateZ) + (toothLength * teethY * teethZ);
  constexpr std::array<int32, 3> normalFaceCounts = {2 * plateY * plateZ, (2 * plateZ) + (2 * toothLength * teethY * teethZ), (2 * plateY) + (2 * toothLength * teethY * teethZ)};
  STATIC_REQUIRE(combCellCount == 138957);
  STATIC_REQUIRE(normalFaceCounts[0] == 7770);
  STATIC_REQUIRE(normalFaceCounts[1] == 270214);
  STATIC_REQUIRE(normalFaceCounts[2] == 270366);
  const std::array<int32, 2> counts = {static_cast<int32>(cellCount) - combCellCount, combCellCount};
  std::vector<int32> labels(cellCount, 0);
  for(usize zIndex = 1; zIndex <= plateZ; zIndex++)
  {
    for(usize yIndex = 1; yIndex <= plateY; yIndex++)
    {
      for(usize xIndex = 1; xIndex <= toothLength + 1; xIndex++)
      {
        if(xIndex == 1 || (yIndex % 2 == 1 && zIndex % 2 == 1))
        {
          labels[(((zIndex * dimensions[1]) + yIndex) * dimensions[0]) + xIndex] = 1;
        }
      }
    }
  }
  REQUIRE(std::count(labels.begin(), labels.end(), int32{1}) == combCellCount);

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions(dimensions);
  imageGeomPtr->setSpacing(spacing);
  imageGeomPtr->setOrigin(origin);
  auto* cellAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  REQUIRE(cellAttributeMatrixPtr != nullptr);
  imageGeomPtr->setCellData(*cellAttributeMatrixPtr);
  auto* featureAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, "CellFeatureData", featureShape, imageGeomPtr->getId());
  REQUIRE(featureAttributeMatrixPtr != nullptr);
  auto idsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, cellShape, componentShape);
  auto countsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, countsPath, featureShape, componentShape);
  REQUIRE(idsStorePtr != nullptr);
  REQUIRE(countsStorePtr != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", idsStorePtr, cellAttributeMatrixPtr->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "NumCells", countsStorePtr, featureAttributeMatrixPtr->getId()) != nullptr);
  auto idsWrite = idsStorePtr->copyFromBuffer(0, nonstd::span<const int32>(labels.data(), labels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWrite);
  auto countsWrite = countsStorePtr->copyFromBuffer(0, nonstd::span<const int32>(counts.data(), counts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsWrite);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(idsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(idsPath);
  const auto& numCells = dataStructure.getDataRefAs<Int32Array>(countsPath);
  scope.requireExpectedStore(ids);
  scope.requireExpectedStore(numCells);
  const auto idsFormat = ids.getDataFormat();
  const auto countsFormat = numCells.getDataFormat();

  const ComputeSurfaceAreaToVolumeFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(idsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(countsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(calculateSphericity));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>("SurfaceAreaVolumeRatio"));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>("Sphericity"));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  // Count entire plate and tooth faces analytically; do not repeat the production neighbor scan.
  const std::array<double, 3> physicalSpacing = {spacing[0], spacing[1], spacing[2]};
  const double area = (normalFaceCounts[0] * physicalSpacing[1] * physicalSpacing[2]) + (normalFaceCounts[1] * physicalSpacing[0] * physicalSpacing[2]) +
                      (normalFaceCounts[2] * physicalSpacing[0] * physicalSpacing[1]);
  const double volume = combCellCount * physicalSpacing[0] * physicalSpacing[1] * physicalSpacing[2];
  CAPTURE(spacing[0], spacing[1], spacing[2], calculateSphericity, area, volume);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(ratioPath));
  const auto& ratios = dataStructure.getDataRefAs<Float32Array>(ratioPath);
  scope.requireExpectedStore(ratios);
  CHECK(ratios.getTupleShape() == featureShape);
  CHECK(ratios.getComponentShape() == componentShape);
  std::array<float32, 2> ratioValues = {};
  auto ratiosRead = ratios.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(ratioValues.data(), ratioValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(ratiosRead);
  CHECK(ratioValues[0] == 0.0f);
  CHECK(ratioValues[1] == Approx(area / volume).epsilon(1.0e-6));
  if(calculateSphericity)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(sphericityPath));
    const auto& sphericity = dataStructure.getDataRefAs<Float32Array>(sphericityPath);
    scope.requireExpectedStore(sphericity);
    CHECK(sphericity.getTupleShape() == featureShape);
    CHECK(sphericity.getComponentShape() == componentShape);
    std::array<float32, 2> values = {};
    auto sphericityRead = sphericity.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(sphericityRead);
    CHECK(values[0] == 0.0f);
    const double expected = std::cbrt(std::acos(-1.0)) * std::pow(6.0 * volume, 2.0 / 3.0) / area;
    CHECK(values[1] == Approx(expected).epsilon(1.0e-6));
  }
  else
  {
    CHECK(dataStructure.getDataAs<IDataArray>(sphericityPath) == nullptr);
  }

  std::vector<int32> actualLabels(cellCount);
  std::array<int32, 2> actualCounts = {};
  auto idsRead = ids.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actualLabels.data(), actualLabels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsRead);
  auto countsRead = numCells.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actualCounts.data(), actualCounts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsRead);
  CHECK(actualLabels == labels);
  CHECK(actualCounts == counts);
  CHECK(ids.getTupleShape() == cellShape);
  CHECK(ids.getComponentShape() == componentShape);
  CHECK(numCells.getTupleShape() == featureShape);
  CHECK(numCells.getComponentShape() == componentShape);
  CHECK(ids.getDataFormat() == idsFormat);
  CHECK(numCells.getDataFormat() == countsFormat);
  CHECK(&ids.getDataStoreRef() == idsStorePtr.get());
  CHECK(&numCells.getDataStoreRef() == countsStorePtr.get());
  scope.requireExpectedStore(ids);
  scope.requireExpectedStore(numCells);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geomPath));
  const auto& finalGeom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  CHECK(finalGeom.getDimensions() == dimensions);
  CHECK(finalGeom.getSpacing() == spacing);
  CHECK(finalGeom.getOrigin() == origin);
  CHECK(cellAttributeMatrixPtr->getShape() == cellShape);
  CHECK(featureAttributeMatrixPtr->getShape() == featureShape);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace surface_area_to_volume_test

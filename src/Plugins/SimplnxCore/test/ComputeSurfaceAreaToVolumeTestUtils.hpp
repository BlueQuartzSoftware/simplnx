#pragma once

#include "SimplnxCore/Filters/ComputeSurfaceAreaToVolumeFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace SurfaceAreaToVolumeTest
{
// A wrong X/Y face weight changes the physical area of this interior box.
template <class ScopeT>
void CheckInteriorPairSurfaceMetrics(ScopeT& scope, nx::core::usize pairAxis, const nx::core::FloatVec3& spacing, bool calculateSphericity, const nx::core::SizeVec3& dimensions = {5, 5, 5})
{
  using namespace nx::core;
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
  const bool singleSlice = dimensions[2] == 1;
  REQUIRE(pairAxis < (singleSlice ? 2 : 3));
  REQUIRE(dimensions == (singleSlice ? SizeVec3{5, 5, 1} : SizeVec3{5, 5, 5}));
  constexpr usize pairFeature = 2;
  constexpr int32 enclosingFeature = 1;
  const usize cellCount = dimensions[0] * dimensions[1] * dimensions[2];

  auto* geom = ImageGeom::Create(dataStructure, "ImageGeom");
  REQUIRE(geom != nullptr);
  geom->setDimensions(dimensions);
  geom->setSpacing(spacing);
  geom->setOrigin(origin);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", cellShape, geom->getId());
  REQUIRE(cellAM != nullptr);
  geom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "CellFeatureData", featureShape, geom->getId());
  REQUIRE(featureAM != nullptr);

  auto idsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, cellShape, componentShape);
  auto countsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, countsPath, featureShape, componentShape);
  REQUIRE(idsStore != nullptr);
  REQUIRE(countsStore != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", idsStore, cellAM->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "NumCells", countsStore, featureAM->getId()) != nullptr);
  std::vector<int32> labels(cellCount, enclosingFeature);
  std::array<usize, 3> first = {2, 2, singleSlice ? usize{0} : usize{2}};
  auto second = first;
  second[pairAxis]++;
  labels[(first[2] * dimensions[1] + first[1]) * dimensions[0] + first[0]] = static_cast<int32>(pairFeature);
  labels[(second[2] * dimensions[1] + second[1]) * dimensions[0] + second[0]] = static_cast<int32>(pairFeature);
  const std::array<int32, 3> counts = singleSlice ? std::array<int32, 3>{0, 23, 2} : std::array<int32, 3>{0, 123, 2};
  auto idsWrite = idsStore->copyFromBuffer(0, nonstd::span<const int32>(labels.data(), labels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWrite);
  auto countsWrite = countsStore->copyFromBuffer(0, nonstd::span<const int32>(counts.data(), counts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsWrite);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(idsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(idsPath);
  const auto& numCells = dataStructure.getDataRefAs<Int32Array>(countsPath);
  scope.requireExpectedStore(ids);
  scope.requireExpectedStore(numCells);
  const auto idsFormat = ids.getDataFormat();
  const auto countsFormat = numCells.getDataFormat();

  ComputeSurfaceAreaToVolumeFilter filter;
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
  sides[pairAxis] *= 2.0;
  // Exterior Z faces are excluded for a single slice, as in the production boundary policy.
  const double area = 2.0 * ((singleSlice ? 0.0 : sides[0] * sides[1]) + sides[0] * sides[2] + sides[1] * sides[2]);
  const double voxelVolume = static_cast<double>(spacing[0]) * spacing[1] * spacing[2];
  std::array<double, 3> volumes = {0.0, 0.0, 0.0};
  volumes[pairFeature] = sides[0] * sides[1] * sides[2];
  volumes[enclosingFeature] = (singleSlice ? 23.0 : 123.0) * voxelVolume;
  CAPTURE(pairAxis, spacing[0], spacing[1], spacing[2], calculateSphericity, singleSlice, area);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(ratioPath));
  const auto& ratios = dataStructure.getDataRefAs<Float32Array>(ratioPath);
  scope.requireExpectedStore(ratios);
  CHECK(ratios.getTupleShape() == featureShape);
  CHECK(ratios.getComponentShape() == componentShape);
  std::array<float32, 3> ratioValues;
  auto ratiosRead = ratios.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(ratioValues.data(), ratioValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(ratiosRead);
  CHECK(ratioValues[0] == 0.0f);
  for(usize feature = 1; feature < 3; feature++)
  {
    CAPTURE(feature);
    CHECK(ratioValues[feature] == Approx(area / volumes[feature]).epsilon(1.0e-6));
  }
  if(calculateSphericity)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(sphericityPath));
    const auto& sphericity = dataStructure.getDataRefAs<Float32Array>(sphericityPath);
    scope.requireExpectedStore(sphericity);
    CHECK(sphericity.getTupleShape() == featureShape);
    CHECK(sphericity.getComponentShape() == componentShape);
    std::array<float32, 3> values;
    auto sphericityRead = sphericity.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(sphericityRead);
    CHECK(values[0] == 0.0f);
    for(usize feature = 1; feature < 3; feature++)
    {
      CAPTURE(feature);
      const double expected = std::cbrt(std::acos(-1.0)) * std::pow(6.0 * volumes[feature], 2.0 / 3.0) / area;
      // This tolerance accommodates the existing truncated exponents, intentionally preserved by this correction.
      CHECK(values[feature] == Approx(expected).epsilon(1.0e-4));
    }
  }
  else
  {
    CHECK(dataStructure.getDataAs<IDataArray>(sphericityPath) == nullptr);
  }

  std::vector<int32> actualLabels(cellCount);
  std::array<int32, 3> actualCounts;
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
  CHECK(&ids.getDataStoreRef() == idsStore.get());
  CHECK(&numCells.getDataStoreRef() == countsStore.get());
  scope.requireExpectedStore(ids);
  scope.requireExpectedStore(numCells);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geomPath));
  const auto& finalGeom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  CHECK(finalGeom.getDimensions() == dimensions);
  CHECK(finalGeom.getSpacing() == spacing);
  CHECK(finalGeom.getOrigin() == origin);
  CHECK(cellAM->getShape() == cellShape);
  CHECK(featureAM->getShape() == featureShape);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  constexpr int32 combCellCount = plateY * plateZ + toothLength * teethY * teethZ;
  constexpr std::array<int32, 3> normalFaceCounts = {2 * plateY * plateZ, 2 * plateZ + 2 * toothLength * teethY * teethZ, 2 * plateY + 2 * toothLength * teethY * teethZ};
  STATIC_REQUIRE(combCellCount == 138957);
  STATIC_REQUIRE(normalFaceCounts[0] == 7770);
  STATIC_REQUIRE(normalFaceCounts[1] == 270214);
  STATIC_REQUIRE(normalFaceCounts[2] == 270366);
  const std::array<int32, 2> counts = {static_cast<int32>(cellCount) - combCellCount, combCellCount};
  std::vector<int32> labels(cellCount, 0);
  for(usize z = 1; z <= plateZ; z++)
  {
    for(usize y = 1; y <= plateY; y++)
    {
      for(usize x = 1; x <= toothLength + 1; x++)
      {
        if(x == 1 || (y % 2 == 1 && z % 2 == 1))
        {
          labels[(z * dimensions[1] + y) * dimensions[0] + x] = 1;
        }
      }
    }
  }
  REQUIRE(std::count(labels.begin(), labels.end(), int32{1}) == combCellCount);

  auto* geom = ImageGeom::Create(dataStructure, "ImageGeom");
  REQUIRE(geom != nullptr);
  geom->setDimensions(dimensions);
  geom->setSpacing(spacing);
  geom->setOrigin(origin);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", cellShape, geom->getId());
  REQUIRE(cellAM != nullptr);
  geom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "CellFeatureData", featureShape, geom->getId());
  REQUIRE(featureAM != nullptr);
  auto idsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, cellShape, componentShape);
  auto countsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, countsPath, featureShape, componentShape);
  REQUIRE(idsStore != nullptr);
  REQUIRE(countsStore != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", idsStore, cellAM->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "NumCells", countsStore, featureAM->getId()) != nullptr);
  auto idsWrite = idsStore->copyFromBuffer(0, nonstd::span<const int32>(labels.data(), labels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWrite);
  auto countsWrite = countsStore->copyFromBuffer(0, nonstd::span<const int32>(counts.data(), counts.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsWrite);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(idsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(idsPath);
  const auto& numCells = dataStructure.getDataRefAs<Int32Array>(countsPath);
  scope.requireExpectedStore(ids);
  scope.requireExpectedStore(numCells);
  const auto idsFormat = ids.getDataFormat();
  const auto countsFormat = numCells.getDataFormat();

  ComputeSurfaceAreaToVolumeFilter filter;
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
  const double area =
      normalFaceCounts[0] * physicalSpacing[1] * physicalSpacing[2] + normalFaceCounts[1] * physicalSpacing[0] * physicalSpacing[2] + normalFaceCounts[2] * physicalSpacing[0] * physicalSpacing[1];
  const double volume = combCellCount * physicalSpacing[0] * physicalSpacing[1] * physicalSpacing[2];
  CAPTURE(spacing[0], spacing[1], spacing[2], calculateSphericity, area, volume);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(ratioPath));
  const auto& ratios = dataStructure.getDataRefAs<Float32Array>(ratioPath);
  scope.requireExpectedStore(ratios);
  CHECK(ratios.getTupleShape() == featureShape);
  CHECK(ratios.getComponentShape() == componentShape);
  std::array<float32, 2> ratioValues;
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
    std::array<float32, 2> values;
    auto sphericityRead = sphericity.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(sphericityRead);
    CHECK(values[0] == 0.0f);
    const double expected = std::cbrt(std::acos(-1.0)) * std::pow(6.0 * volume, 2.0 / 3.0) / area;
    // Allow the existing truncated sphericity exponents until their separate correction.
    CHECK(values[1] == Approx(expected).epsilon(1.0e-4));
  }
  else
  {
    CHECK(dataStructure.getDataAs<IDataArray>(sphericityPath) == nullptr);
  }

  std::vector<int32> actualLabels(cellCount);
  std::array<int32, 2> actualCounts;
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
  CHECK(&ids.getDataStoreRef() == idsStore.get());
  CHECK(&numCells.getDataStoreRef() == countsStore.get());
  scope.requireExpectedStore(ids);
  scope.requireExpectedStore(numCells);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geomPath));
  const auto& finalGeom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  CHECK(finalGeom.getDimensions() == dimensions);
  CHECK(finalGeom.getSpacing() == spacing);
  CHECK(finalGeom.getOrigin() == origin);
  CHECK(cellAM->getShape() == cellShape);
  CHECK(featureAM->getShape() == featureShape);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace SurfaceAreaToVolumeTest

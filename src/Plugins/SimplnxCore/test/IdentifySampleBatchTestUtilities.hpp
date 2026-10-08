#pragma once

#include "SimplnxCore/Filters/IdentifySampleFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <memory>
#include <utility>

namespace IdentifySampleBatchTest
{
/**
 * @namespace IdentifySampleBatchTest
 * @brief Supplies independent mask fixtures for resident and HDF5 Identify Sample tests.
 */
using namespace nx::core;

/**
 * @brief Identifies the geometry created by the fixture.
 */
inline const DataPath k_ImagePath{{"BatchImage"}};
/**
 * @brief Identifies the mask updated by the filter.
 */
inline const DataPath k_MaskPath{{"BatchImage", "CellData", "Mask"}};

/**
 * @class ScopedPreferenceRestore
 * @brief Preserves explicit-key presence and JSON number types around the scenario scopes.
 */
class ScopedPreferenceRestore
{
public:
  /** @brief Captures both preference values before a scenario changes them. */
  ScopedPreferenceRestore()
  : m_Preferences(*Application::GetOrCreateInstance()->getPreferences())
  , m_SizePresent(m_Preferences.contains(Preferences::k_LargeDataSize_Key))
  , m_ModePresent(m_Preferences.contains(Preferences::k_DataStorageMode_Key))
  , m_Size(m_Preferences.value(Preferences::k_LargeDataSize_Key))
  , m_Mode(m_Preferences.value(Preferences::k_DataStorageMode_Key))
  {
  }

  /** @brief Restores the captured values and explicit-key presence without saving a file. */
  ~ScopedPreferenceRestore()
  {
    // Recompute the dependent default before restoring an absent threshold key.
    m_Preferences.setValue(Preferences::k_LargeDataSize_Key, m_Size);
    if(!m_SizePresent)
    {
      m_Preferences.removeValue(Preferences::k_LargeDataSize_Key);
    }
    if(m_ModePresent)
    {
      m_Preferences.setValue(Preferences::k_DataStorageMode_Key, m_Mode);
    }
    else
    {
      m_Preferences.removeValue(Preferences::k_DataStorageMode_Key);
    }
  }

  ScopedPreferenceRestore(const ScopedPreferenceRestore&) = delete;
  ScopedPreferenceRestore& operator=(const ScopedPreferenceRestore&) = delete;

private:
  Preferences& m_Preferences;
  bool m_SizePresent;
  bool m_ModePresent;
  nlohmann::json m_Size;
  nlohmann::json m_Mode;
};

/**
 * @brief Rotates nineteen 11 by 13 planes through the three supported orientations.
 * @param plane XY=0, XZ=1, or YZ=2.
 * @param fixedCount Number of planes along the remaining axis.
 * @return Geometry dimensions in X, Y, Z order.
 */
inline SizeVec3 Dimensions(ChoicesParameter::ValueType plane, usize fixedCount = 19)
{
  REQUIRE(fixedCount > 0);
  switch(plane)
  {
  case 0:
    return {11, 13, fixedCount};
  case 1:
    return {11, fixedCount, 13};
  case 2:
    return {fixedCount, 11, 13};
  default:
    FAIL("The fixture supports only XY, XZ, and YZ planes");
    return {};
  }
}

/**
 * @brief Maps geometry coordinates to the independent plane predicate.
 * @param plane XY=0, XZ=1, or YZ=2.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param z Z coordinate.
 * @return Coordinates u, v, and fixed-plane index.
 */
inline std::array<usize, 3> PlaneCoordinates(ChoicesParameter::ValueType plane, usize x, usize y, usize z)
{
  switch(plane)
  {
  case 0:
    return {x, y, z};
  case 1:
    return {x, z, y};
  case 2:
    return {y, z, x};
  default:
    FAIL("The fixture supports only XY, XZ, and YZ planes");
    return {};
  }
}

/**
 * @brief Defines a 47-cell component, an enclosed false cell, and one isolated true cell.
 * @param u First plane coordinate.
 * @param v Second plane coordinate.
 * @param fixed Plane index that selects the alternating rectangle position.
 * @return Initial mask value.
 */
inline bool InputValue(usize u, usize v, usize fixed)
{
  const usize shift = fixed % 2;
  const bool rectangle = u >= 2 + shift && u <= 7 + shift && v >= 2 && v <= 9;
  const bool hole = u == 4 + shift && v == 5;
  const bool isolated = u == 0 && v == 12;
  return (rectangle && !hole) || isolated;
}

/**
 * @brief Defines the retained rectangle independently of the filter output.
 * @param u First plane coordinate.
 * @param v Second plane coordinate.
 * @param fixed Plane index that selects the alternating rectangle position.
 * @param fillHoles True to include the enclosed false cell.
 * @return Expected mask value after the isolated component is removed.
 */
inline bool ExpectedValue(usize u, usize v, usize fixed, bool fillHoles)
{
  const usize shift = fixed % 2;
  return u >= 2 + shift && u <= 7 + shift && v >= 2 && v <= 9 && (fillHoles || u != 4 + shift || v != 5);
}

/**
 * @brief Builds the mask through the configured factory unless a resident reporting store is supplied.
 * @tparam T Bool or UInt8 mask element type.
 * @param plane XY=0, XZ=1, or YZ=2.
 * @param fixedCount Number of independent planes.
 * @param reportingStore Optional resident store with matching tuple and component dimensions.
 * @return Geometry and mask initialized through bounded XY transfers.
 */
template <class T>
DataStructure CreateFixture(ChoicesParameter::ValueType plane, usize fixedCount = 19, std::shared_ptr<AbstractDataStore<T>> reportingStore = {})
{
  const auto dimensions = Dimensions(plane, fixedCount);
  const ShapeType tupleShape{dimensions[2], dimensions[1], dimensions[0]};
  DataStructure dataStructure;
  auto* image = ImageGeom::Create(dataStructure, k_ImagePath.getTargetName());
  REQUIRE(image != nullptr);
  image->setDimensions(dimensions);
  image->setSpacing({1.0F, 1.0F, 1.0F});
  image->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cellData = AttributeMatrix::Create(dataStructure, "CellData", tupleShape, image->getId());
  REQUIRE(cellData != nullptr);
  image->setCellData(*cellData);
  auto store = reportingStore ? std::move(reportingStore) : DataStoreUtilities::CreateDataStore<T>(dataStructure, k_MaskPath, tupleShape, ShapeType{1});
  REQUIRE(store != nullptr);
  REQUIRE(store->getTupleShape() == tupleShape);
  REQUIRE(store->getComponentShape() == ShapeType{1});
  auto* mask = DataArray<T>::Create(dataStructure, "Mask", store, cellData->getId());
  REQUIRE(mask != nullptr);

  const usize xySize = dimensions[0] * dimensions[1];
  auto xy = std::make_unique<T[]>(xySize);
  for(usize z = 0; z < dimensions[2]; z++)
  {
    for(usize y = 0; y < dimensions[1]; y++)
    {
      for(usize x = 0; x < dimensions[0]; x++)
      {
        const auto coordinates = PlaneCoordinates(plane, x, y, z);
        xy[y * dimensions[0] + x] = static_cast<T>(InputValue(coordinates[0], coordinates[1], coordinates[2]));
      }
    }
    const auto writeResult = store->copyFromBuffer(z * xySize, nonstd::span<const T>(xy.get(), xySize));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
  return dataStructure;
}

/**
 * @brief Selects slice processing for the fixture mask.
 * @param plane XY=0, XZ=1, or YZ=2.
 * @param fillHoles Selects hole filling.
 * @return Complete filter arguments.
 */
inline Arguments ArgumentsFor(ChoicesParameter::ValueType plane, bool fillHoles)
{
  Arguments args;
  args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImagePath));
  args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(fillHoles));
  args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(true));
  args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(plane));
  return args;
}

/**
 * @brief Checks every output cell against a rectangle predicate that does not call production classification code.
 * @tparam T Bool or UInt8 mask element type.
 * @param dataStructure Filter output.
 * @param plane XY=0, XZ=1, or YZ=2.
 * @param fillHoles True when the enclosed false cell must become true.
 * @param fixedCount Number of independent planes.
 */
template <class T>
void RequireOutput(const DataStructure& dataStructure, ChoicesParameter::ValueType plane, bool fillHoles, usize fixedCount = 19)
{
  const auto dimensions = Dimensions(plane, fixedCount);
  const ShapeType tupleShape{dimensions[2], dimensions[1], dimensions[0]};
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_ImagePath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_MaskPath.getParent()));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_MaskPath));
  const auto& image = dataStructure.getDataRefAs<ImageGeom>(k_ImagePath);
  const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(k_MaskPath.getParent());
  const auto& mask = dataStructure.getDataRefAs<DataArray<T>>(k_MaskPath);
  REQUIRE(image.getDimensions() == dimensions);
  REQUIRE(image.getSpacing() == FloatVec3(1.0F, 1.0F, 1.0F));
  REQUIRE(image.getOrigin() == FloatVec3(0.0F, 0.0F, 0.0F));
  REQUIRE(cellData.getShape() == tupleShape);
  REQUIRE(mask.getTupleShape() == tupleShape);
  REQUIRE(mask.getComponentShape() == ShapeType{1});

  const usize xySize = dimensions[0] * dimensions[1];
  REQUIRE(mask.getNumberOfTuples() == xySize * dimensions[2]);
  auto xy = std::make_unique<T[]>(xySize);
  usize trueCount = 0;
  for(usize z = 0; z < dimensions[2]; z++)
  {
    const auto readResult = mask.getDataStoreRef().copyIntoBuffer(z * xySize, nonstd::span<T>(xy.get(), xySize));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    for(usize y = 0; y < dimensions[1]; y++)
    {
      for(usize x = 0; x < dimensions[0]; x++)
      {
        const auto coordinates = PlaneCoordinates(plane, x, y, z);
        const bool expected = ExpectedValue(coordinates[0], coordinates[1], coordinates[2], fillHoles);
        INFO("plane=" << plane << " fill=" << fillHoles << " xyz=" << x << ',' << y << ',' << z);
        REQUIRE(xy[y * dimensions[0] + x] == static_cast<T>(expected));
        trueCount += static_cast<usize>(xy[y * dimensions[0] + x]);
      }
    }
  }
  REQUIRE(trueCount == fixedCount * (fillHoles ? 48 : 47));
  nx::core::UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Runs a fixture under the caller's witnessed resident or actual-OOC scenario.
 * @tparam T Bool or UInt8 mask element type.
 * @tparam ScopeT Public or private algorithm scenario scope.
 * @param scope Selects and verifies algorithm and store dispatch.
 * @param dataStructure Initialized mask fixture.
 * @param plane XY=0, XZ=1, or YZ=2.
 * @param fillHoles Selects hole filling.
 * @param fixedCount Number of independent planes.
 */
template <class T, class ScopeT>
void ExecuteAndRequire(ScopeT& scope, DataStructure& dataStructure, ChoicesParameter::ValueType plane, bool fillHoles, usize fixedCount = 19)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_MaskPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<DataArray<T>>(k_MaskPath));
  IdentifySampleFilter filter;
  const auto args = ArgumentsFor(plane, fillHoles);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  RequireOutput<T>(dataStructure, plane, fillHoles, fixedCount);
}
} // namespace IdentifySampleBatchTest

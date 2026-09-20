#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

/**
 * @struct InitializeImageGeomCellDataInputValues
 * @brief Stores arrays, inclusive bounds, mode, values, range, and seed behavior.
 */
struct SIMPLNXCORE_EXPORT InitializeImageGeomCellDataInputValues
{
  MultiArraySelectionParameter::ValueType CellArrays;
  VectorFloat64Parameter::ValueType InitRange;
  ChoicesParameter::ValueType InitTypeIndex;
  Float64Parameter::ValueType InitValue;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  VectorUInt64Parameter::ValueType MaxPoint;
  VectorUInt64Parameter::ValueType MinPoint;
  DataObjectNameParameter::ValueType SeedArrayName;
  UInt64Parameter::ValueType SeedValue;
  BoolParameter::ValueType UseSeed;
};

/**
 * @class InitializeImageGeomCellData
 * @brief Initializes an inclusive ImageGeom subvolume in selected cell arrays.
 *
 * Manual mode repeats one cast value for every component of a tuple. Random
 * modes also generate one value per tuple and repeat it across components.
 * Selected arrays process sequentially. Their effective seeds start at the
 * stored seed and increment once per array, so selection order affects output.
 *
 * Row writes target 65,536 values but retain one complete tuple. A wider tuple
 * creates a larger buffer. In-core and out-of-core telemetry labels use this
 * same checked bulk-write implementation. The effective first seed is written
 * to a top-level UInt64 array before any selected cell array changes.
 *
 * Floating full-range mode uses numeric_limits<T>::min(), which is the least
 * positive normal value, not the lowest negative value. Integral random modes
 * use uniform_int_distribution<int>; ranges outside int are not representable.
 * Boolean arrays reach ExecuteNeighborFunction's unsupported-type exception.
 */
class SIMPLNXCORE_EXPORT InitializeImageGeomCellData
{
public:
  /**
   * @brief Initializes the ImageGeom subvolume generator.
   * @param dataStructure Contains geometry, selected arrays, and seed output.
   * @param mesgHandler Preserves the common algorithm constructor signature.
   * @param shouldCancel Signals cancellation between row chunks and arrays.
   * @param inputValues Selects arrays, bounds, values, range, and seed behavior.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  InitializeImageGeomCellData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeImageGeomCellDataInputValues* inputValues);
  /**
   * @brief Destroys the ImageGeom subvolume generator.
   */
  ~InitializeImageGeomCellData() noexcept;

  InitializeImageGeomCellData(const InitializeImageGeomCellData&) = delete;
  InitializeImageGeomCellData(InitializeImageGeomCellData&&) noexcept = delete;
  InitializeImageGeomCellData& operator=(const InitializeImageGeomCellData&) = delete;
  InitializeImageGeomCellData& operator=(InitializeImageGeomCellData&&) noexcept = delete;

  /**
   * @brief Initializes every selected array in selection order.
   * @return Bounds, offset, or bulk-write result.
   * @throws std::runtime_error If InitTypeIndex is invalid or an array is Boolean.
   * @pre MinPoint and MaxPoint contain three in-range values, and each minimum is not greater than its maximum.
   * @pre InitRange contains two values that convert to each selected type.
   * @pre Selected arrays match the ImageGeom cells and do not have Boolean type.
   *
   * Cancellation returns success without rollback. The seed output and earlier
   * array or row writes remain after cancellation or a later error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const InitializeImageGeomCellDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeCoordinatesImageGeomInputValues
 * @brief Stores filter values for coordinate generation.
 */
struct SIMPLNXCORE_EXPORT ComputeCoordinatesImageGeomInputValues
{
  ChoicesParameter::ValueType CoordinateOption;
  DataPath ImageGeomPath;
  DataPath CoordArrayPath;
  DataPath IndexArrayPath;
};

/**
 * @class ComputeCoordinatesImageGeom
 * @brief Generates physical cell-center coordinates and/or integer cell indices for an ImageGeom.
 *
 * In-memory outputs use a fused parallel writer over contiguous storage. Out-of-core outputs use
 * fixed-size generated chunks and bulk writes, keeping scratch memory independent of cell count.
 *
 * Direct workers use raw DataStore buffers for disjoint Z ranges. This specialized access does not
 * establish generic DataArray or DataStore thread safety.
 */
class SIMPLNXCORE_EXPORT ComputeCoordinatesImageGeom
{
public:
  /**
   * @brief Initializes the ImageGeom coordinate algorithm.
   * @param dataStructure Contains the ImageGeom and output arrays.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects outputs and identifies required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeCoordinatesImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeCoordinatesImageGeomInputValues* inputValues);
  /**
   * @brief Destroys the ImageGeom coordinate algorithm.
   */
  ~ComputeCoordinatesImageGeom() noexcept;

  ComputeCoordinatesImageGeom(const ComputeCoordinatesImageGeom&) = delete;
  ComputeCoordinatesImageGeom(ComputeCoordinatesImageGeom&&) noexcept = delete;
  ComputeCoordinatesImageGeom& operator=(const ComputeCoordinatesImageGeom&) = delete;
  ComputeCoordinatesImageGeom& operator=(ComputeCoordinatesImageGeom&&) noexcept = delete;

  /**
   * @enum OutputType
   * @brief Identifies generated output arrays.
   */
  enum OutputType : uint8
  {
    Physical = 0, ///< Generates cell-center coordinates.
    Index = 1,    ///< Generates integer cell indices.
    Both = 2      ///< Generates both output arrays.
  };

  /**
   * @brief Generates selected ImageGeom coordinates.
   * @return Success, or an output bulk-I/O error.
   * @pre When index output is selected, generated X, Y, and Z indices fit int32.
   *
   * Cancellation returns success. Direct workers stop at their next Z-slice checkpoints. Scanline
   * execution stops at its next chunk checkpoint. Each path can leave completed output ranges.
   * A scanline bulk-I/O error can leave completed ranges in one or both outputs.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeCoordinatesImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

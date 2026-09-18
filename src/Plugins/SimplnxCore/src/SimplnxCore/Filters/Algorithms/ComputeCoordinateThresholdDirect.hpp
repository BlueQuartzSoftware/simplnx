#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

struct ComputeCoordinateThresholdInputValues;

/**
 * @class ComputeCoordinateThresholdDirect
 * @brief Computes ImageGeom masks directly in contiguous memory.
 *
 * The executor uses raw buffers only after it verifies concrete in-memory stores. A forced direct
 * path with another store delegates to the bounded scanline implementation.
 */
class SIMPLNXCORE_EXPORT ComputeCoordinateThresholdDirect
{
public:
  /**
   * @brief Initializes the direct ImageGeom mask algorithm.
   * @param dataStructure Contains the ImageGeom and output mask.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects bounds and identifies required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeCoordinateThresholdDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   const ComputeCoordinateThresholdInputValues* inputValues);
  /**
   * @brief Destroys the direct ImageGeom mask algorithm.
   */
  ~ComputeCoordinateThresholdDirect() noexcept;

  ComputeCoordinateThresholdDirect(const ComputeCoordinateThresholdDirect&) = delete;
  ComputeCoordinateThresholdDirect(ComputeCoordinateThresholdDirect&&) noexcept = delete;
  ComputeCoordinateThresholdDirect& operator=(const ComputeCoordinateThresholdDirect&) = delete;
  ComputeCoordinateThresholdDirect& operator=(ComputeCoordinateThresholdDirect&&) noexcept = delete;

  /**
   * @brief Creates the ImageGeom mask with direct storage access.
   * @return Success, or a fallback scanline bulk-I/O error.
   *
   * When a slice or fallback chunk checkpoint observes cancellation, the method returns success.
   * Data written before that checkpoint remains in the mask. Later cells are not written.
   * A fallback bulk-I/O error can leave completed mask chunks.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeCoordinateThresholdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core

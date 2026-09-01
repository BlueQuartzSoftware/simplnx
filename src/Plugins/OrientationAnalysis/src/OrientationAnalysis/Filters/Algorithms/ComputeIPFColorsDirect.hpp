#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <atomic>

namespace nx::core
{

struct ComputeIPFColorsInputValues;

/**
 * @class ComputeIPFColorsDirect
 * @brief Computes IPF colors for the in-memory dispatch path.
 *
 * The dispatcher normally selects this class for in-memory data.
 * requireArraysInMemory() disables parallel scheduling when a listed array is
 * not in-memory. The remaining direct parallel access has no generic DataArray
 * or DataStore thread-safety guarantee.
 *
 * An optional mask array allows pre-indexed voxels to be skipped (colored black).
 *
 * @see ComputeIPFColorsScanline for the OOC-optimized variant.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsDirect
{
public:
  /**
   * @brief Initializes direct IPF color computation.
   * @param dataStructure Provides the selected arrays.
   * @param msgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and color settings.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeIPFColorsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, const ComputeIPFColorsInputValues* inputValues);

  /**
   * @brief Destroys the direct IPF color executor.
   */
  ~ComputeIPFColorsDirect() noexcept;

  ComputeIPFColorsDirect(const ComputeIPFColorsDirect&) = delete;
  ComputeIPFColorsDirect(ComputeIPFColorsDirect&&) = delete;
  ComputeIPFColorsDirect& operator=(const ComputeIPFColorsDirect&) = delete;
  ComputeIPFColorsDirect& operator=(ComputeIPFColorsDirect&&) = delete;

  /**
   * @brief Computes IPF colors through direct parallel access.
   * @pre Cell phase IDs are nonnegative.
   * @return Success, or error -48000 if a positive phase ID exceeds the crystal-
   *         structure array.
   *
   * Cancellation stops each worker range before its next tuple. The method
   * normally returns success with partial colors. It returns -48000 if completed
   * work recorded an out-of-range positive phase ID.
   */
  Result<> operator()();

  /**
   * @brief Records a positive phase ID outside the crystal-structure array.
   *
   * The atomic counter lets parallel workers record affected tuples. operator()
   * reads the final count after the parallel algorithm completes.
   */
  void incrementPhaseWarningCount();

  /**
   * @brief Returns the current cancellation state.
   * @return True if cancellation has been requested.
   */
  bool shouldCancel() const;

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeIPFColorsInputValues* m_InputValues = nullptr;
  std::atomic_int32_t m_PhaseWarningCount = 0;
};

} // namespace nx::core

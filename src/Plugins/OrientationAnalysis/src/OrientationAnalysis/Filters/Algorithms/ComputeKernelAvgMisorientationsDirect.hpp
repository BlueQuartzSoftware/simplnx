#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <atomic>

namespace nx::core
{

struct ComputeKernelAvgMisorientationsInputValues;

/**
 * @class ComputeKernelAvgMisorientationsDirect
 * @brief Computes Kernel Average Misorientation through direct array access.
 *
 * The dispatcher normally selects this executor for in-memory targets.
 * requireArraysInMemory() disables parallel scheduling when a target is
 * out-of-core. This executor does not provide a general DataArray or DataStore
 * concurrent-access guarantee.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeKernelAvgMisorientationsDirect
{
public:
  /**
   * @brief Initializes the direct KAM executor.
   * @param dataStructure Provides the selected arrays and Image Geometry.
   * @param msgHandler Receives progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and KAM settings.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues remain valid
   *      while this executor runs.
   * @pre inputValues identifies valid arrays and contains three nonnegative
   *      kernel radii.
   */
  ComputeKernelAvgMisorientationsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                        const ComputeKernelAvgMisorientationsInputValues* inputValues);

  /**
   * @brief Destroys the direct KAM executor.
   */
  ~ComputeKernelAvgMisorientationsDirect() noexcept;

  ComputeKernelAvgMisorientationsDirect(const ComputeKernelAvgMisorientationsDirect&) = delete;
  ComputeKernelAvgMisorientationsDirect(ComputeKernelAvgMisorientationsDirect&&) noexcept = delete;
  ComputeKernelAvgMisorientationsDirect& operator=(const ComputeKernelAvgMisorientationsDirect&) = delete;
  ComputeKernelAvgMisorientationsDirect& operator=(ComputeKernelAvgMisorientationsDirect&&) noexcept = delete;

  /**
   * @brief Executes the direct KAM traversal.
   * @return Success after the traversal or cancellation.
   *
   * Cancellation stops each active range at its next row check. The method
   * returns success and can leave output tuples not recomputed.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

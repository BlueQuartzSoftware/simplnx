#pragma once

#include "SimplnxCore/Filters/Algorithms/NearestPointFuseRegularGrids.hpp"

namespace nx::core
{
/**
 * @class NearestPointFuseRegularGridsScanline
 * @brief Resamples cell arrays with axis maps and bounded row buffers.
 *
 * Axis maps convert reference lattice coordinates to containing sampling cells.
 * Reused source rows and one checked write per destination row avoid random disk I/O.
 * Scratch contains three axis maps and two rows.
 *
 * @see NearestPointFuseRegularGridsDirect
 */
class SIMPLNXCORE_EXPORT NearestPointFuseRegularGridsScanline
{
public:
  /**
   * @brief Creates a row-buffered resampler.
   * @param dataStructure Provides both image geometries and their cell arrays.
   * @param messageHandler Is retained for the dispatched interface.
   * @param shouldCancel Stops before later arrays or reference Z slices when true.
   * @param inputValues Specifies validated paths and the fill value. The caller
   * must keep this object alive for the resampler lifetime.
   */
  NearestPointFuseRegularGridsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                       const NearestPointFuseRegularGridsInputValues* inputValues);
  /**
   * @brief Resamples each numeric or Boolean sampling-cell array.
   * @return First bulk-I/O error, or success after completion or cancellation.
   *
   * Cancellation or an I/O error can retain complete destination rows already written.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const NearestPointFuseRegularGridsInputValues* m_InputValues = nullptr;
};
} // namespace nx::core

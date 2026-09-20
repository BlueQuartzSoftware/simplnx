#pragma once

#include "SimplnxCore/Filters/Algorithms/NearestPointFuseRegularGrids.hpp"

namespace nx::core
{
/**
 * @class NearestPointFuseRegularGridsDirect
 * @brief Resamples resident cell arrays through direct nearest-cell access.
 *
 * Independent arrays run concurrently. Each reference lattice coordinate selects
 * its containing sampling cell. Direct source reads suit resident stores but cause
 * repeated chunk access for disk-backed stores.
 *
 * @see NearestPointFuseRegularGridsScanline
 */
class SIMPLNXCORE_EXPORT NearestPointFuseRegularGridsDirect
{
public:
  /**
   * @brief Creates a direct parallel resampler.
   * @param dataStructure Provides both image geometries and their cell arrays.
   * @param messageHandler Is retained for the dispatched interface.
   * @param shouldCancel Stops later arrays or reference Z slices when true.
   * @param inputValues Specifies validated paths and the fill value. The caller
   * must keep this object alive for the resampler lifetime.
   */
  NearestPointFuseRegularGridsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                     const NearestPointFuseRegularGridsInputValues* inputValues);
  /**
   * @brief Resamples each numeric or Boolean sampling-cell array.
   * @return Success after all scheduled tasks finish or observe cancellation.
   *
   * Cancellation can leave different destination arrays at different completion points.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const NearestPointFuseRegularGridsInputValues* m_InputValues = nullptr;
};
} // namespace nx::core

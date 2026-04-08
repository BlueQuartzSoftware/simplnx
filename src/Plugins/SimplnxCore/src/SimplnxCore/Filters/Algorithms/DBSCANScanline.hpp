#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct DBSCANInputValues;

/**
 * @class DBSCANScanline
 * @brief Out-of-core algorithm for DBSCAN. Uses chunked copyIntoBuffer bulk I/O
 * for grid construction and on-demand per-grid-cell reads for canMerge distance
 * computation. Selected by DispatchAlgorithm when any input array is backed by
 * ZarrStore (out-of-core storage).
 */
class SIMPLNXCORE_EXPORT DBSCANScanline
{
public:
  DBSCANScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const DBSCANInputValues* inputValues);
  ~DBSCANScanline() noexcept;

  DBSCANScanline(const DBSCANScanline&) = delete;
  DBSCANScanline(DBSCANScanline&&) noexcept = delete;
  DBSCANScanline& operator=(const DBSCANScanline&) = delete;
  DBSCANScanline& operator=(DBSCANScanline&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const DBSCANInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct MultiThresholdObjectsInputValues;

/**
 * @class MultiThresholdObjectsScanline
 * @brief Out-of-core algorithm for MultiThresholdObjects. Processes data in fixed-size
 * chunks using copyIntoBuffer/copyFromBuffer bulk I/O to avoid per-element OOC access
 * and eliminates the O(n) tempResultVector allocation. Selected by DispatchAlgorithm
 * when any input array is backed by ZarrStore (out-of-core storage).
 */
class SIMPLNXCORE_EXPORT MultiThresholdObjectsScanline
{
public:
  MultiThresholdObjectsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const MultiThresholdObjectsInputValues* inputValues);
  ~MultiThresholdObjectsScanline() noexcept;

  MultiThresholdObjectsScanline(const MultiThresholdObjectsScanline&) = delete;
  MultiThresholdObjectsScanline(MultiThresholdObjectsScanline&&) noexcept = delete;
  MultiThresholdObjectsScanline& operator=(const MultiThresholdObjectsScanline&) = delete;
  MultiThresholdObjectsScanline& operator=(MultiThresholdObjectsScanline&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const MultiThresholdObjectsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

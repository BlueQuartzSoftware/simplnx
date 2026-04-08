#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct MultiThresholdObjectsInputValues;

/**
 * @class MultiThresholdObjectsDirect
 * @brief In-core algorithm for MultiThresholdObjects. Preserves the original per-element
 * access pattern and O(n) tempResultVector allocation. Selected by DispatchAlgorithm
 * when all input arrays are backed by in-memory DataStore.
 */
class SIMPLNXCORE_EXPORT MultiThresholdObjectsDirect
{
public:
  MultiThresholdObjectsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const MultiThresholdObjectsInputValues* inputValues);
  ~MultiThresholdObjectsDirect() noexcept;

  MultiThresholdObjectsDirect(const MultiThresholdObjectsDirect&) = delete;
  MultiThresholdObjectsDirect(MultiThresholdObjectsDirect&&) noexcept = delete;
  MultiThresholdObjectsDirect& operator=(const MultiThresholdObjectsDirect&) = delete;
  MultiThresholdObjectsDirect& operator=(MultiThresholdObjectsDirect&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const MultiThresholdObjectsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct KMedoidsInputValues;

/**
 * @class ComputeKMedoidsDirect
 * @brief In-core algorithm for ComputeKMedoids. Uses direct per-element operator[]
 * access for distance computation, cluster assignment, and medoid optimization.
 * Selected by DispatchAlgorithm when all input arrays are backed by in-memory DataStore.
 */
class SIMPLNXCORE_EXPORT ComputeKMedoidsDirect
{
public:
  ComputeKMedoidsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const KMedoidsInputValues* inputValues);
  ~ComputeKMedoidsDirect() noexcept;

  ComputeKMedoidsDirect(const ComputeKMedoidsDirect&) = delete;
  ComputeKMedoidsDirect(ComputeKMedoidsDirect&&) noexcept = delete;
  ComputeKMedoidsDirect& operator=(const ComputeKMedoidsDirect&) = delete;
  ComputeKMedoidsDirect& operator=(ComputeKMedoidsDirect&&) noexcept = delete;

  Result<> operator()();

  void updateProgress(const std::string& message);
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const KMedoidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

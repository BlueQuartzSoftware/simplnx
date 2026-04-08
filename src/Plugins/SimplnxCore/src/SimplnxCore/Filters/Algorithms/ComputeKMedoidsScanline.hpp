#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct KMedoidsInputValues;

/**
 * @class ComputeKMedoidsScanline
 * @brief Out-of-core algorithm for ComputeKMedoids. Uses chunked copyIntoBuffer/copyFromBuffer
 * bulk I/O for distance computation, cluster assignment, and medoid optimization.
 * Selected by DispatchAlgorithm when any input array is backed by out-of-core storage.
 */
class SIMPLNXCORE_EXPORT ComputeKMedoidsScanline
{
public:
  ComputeKMedoidsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const KMedoidsInputValues* inputValues);
  ~ComputeKMedoidsScanline() noexcept;

  ComputeKMedoidsScanline(const ComputeKMedoidsScanline&) = delete;
  ComputeKMedoidsScanline(ComputeKMedoidsScanline&&) noexcept = delete;
  ComputeKMedoidsScanline& operator=(const ComputeKMedoidsScanline&) = delete;
  ComputeKMedoidsScanline& operator=(ComputeKMedoidsScanline&&) noexcept = delete;

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

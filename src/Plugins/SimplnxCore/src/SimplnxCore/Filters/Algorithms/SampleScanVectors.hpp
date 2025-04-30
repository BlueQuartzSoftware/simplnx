#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT SampleScanVectorsInputValues
{
  float32 ScanVectorSamplingRes;
  DataPath ScanVectorGeometryPath;
  bool CopyPowerData;
  DataPath PowerArrayPath;
  bool CopySliceIds;
  DataPath SliceIdArrayPath;
  DataPath SampledVertexGeometryPath;
  bool CalculateCumulativeSampleDistance;
  std::string CumulativeSampleDistanceArrayName;
  bool CopyEdgeIds;
  std::string EdgeIdsArrayName;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT SampleScanVectors
{
public:
  SampleScanVectors(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, SampleScanVectorsInputValues* inputValues);
  ~SampleScanVectors() noexcept;

  SampleScanVectors(const SampleScanVectors&) = delete;
  SampleScanVectors(SampleScanVectors&&) noexcept = delete;
  SampleScanVectors& operator=(const SampleScanVectors&) = delete;
  SampleScanVectors& operator=(SampleScanVectors&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const SampleScanVectorsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

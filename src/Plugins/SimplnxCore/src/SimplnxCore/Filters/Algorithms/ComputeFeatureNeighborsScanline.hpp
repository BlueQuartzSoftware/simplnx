#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeFeatureNeighborsInputValues;

class SIMPLNXCORE_EXPORT ComputeFeatureNeighborsScanline
{
public:
  ComputeFeatureNeighborsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  const ComputeFeatureNeighborsInputValues* inputValues);
  ~ComputeFeatureNeighborsScanline() noexcept;

  ComputeFeatureNeighborsScanline(const ComputeFeatureNeighborsScanline&) = delete;
  ComputeFeatureNeighborsScanline(ComputeFeatureNeighborsScanline&&) noexcept = delete;
  ComputeFeatureNeighborsScanline& operator=(const ComputeFeatureNeighborsScanline&) = delete;
  ComputeFeatureNeighborsScanline& operator=(ComputeFeatureNeighborsScanline&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

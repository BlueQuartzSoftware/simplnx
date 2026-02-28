#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct BadDataNeighborOrientationCheckInputValues;

class ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheckScanline
{
public:
  BadDataNeighborOrientationCheckScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          const BadDataNeighborOrientationCheckInputValues* inputValues);
  ~BadDataNeighborOrientationCheckScanline() noexcept;

  BadDataNeighborOrientationCheckScanline(const BadDataNeighborOrientationCheckScanline&) = delete;
  BadDataNeighborOrientationCheckScanline(BadDataNeighborOrientationCheckScanline&&) noexcept = delete;
  BadDataNeighborOrientationCheckScanline& operator=(const BadDataNeighborOrientationCheckScanline&) = delete;
  BadDataNeighborOrientationCheckScanline& operator=(BadDataNeighborOrientationCheckScanline&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const BadDataNeighborOrientationCheckInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

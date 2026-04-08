#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ComputeGBCDPoleFigureInputValues;

/**
 * @class ComputeGBCDPoleFigureScanline
 * @brief OOC-safe variant that accesses the GBCD array through the DataStore
 * without caching the entire array in RAM. Runs single-threaded since
 * DataStore per-element access is not thread-safe for concurrent reads.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeGBCDPoleFigureScanline
{
public:
  ComputeGBCDPoleFigureScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDPoleFigureInputValues* inputValues);
  ~ComputeGBCDPoleFigureScanline() noexcept;

  ComputeGBCDPoleFigureScanline(const ComputeGBCDPoleFigureScanline&) = delete;
  ComputeGBCDPoleFigureScanline(ComputeGBCDPoleFigureScanline&&) noexcept = delete;
  ComputeGBCDPoleFigureScanline& operator=(const ComputeGBCDPoleFigureScanline&) = delete;
  ComputeGBCDPoleFigureScanline& operator=(ComputeGBCDPoleFigureScanline&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeGBCDPoleFigureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

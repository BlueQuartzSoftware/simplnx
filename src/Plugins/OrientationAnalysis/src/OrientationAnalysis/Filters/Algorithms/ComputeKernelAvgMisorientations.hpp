#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <chrono>

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeKernelAvgMisorientationsInputValues
{
  VectorInt32Parameter::ValueType KernelSize;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath KernelAverageMisorientationsArrayName;
  DataPath InputImageGeometry;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeKernelAvgMisorientations
{
public:
  ComputeKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  ComputeKernelAvgMisorientationsInputValues* inputValues);
  ~ComputeKernelAvgMisorientations() noexcept;

  ComputeKernelAvgMisorientations(const ComputeKernelAvgMisorientations&) = delete;
  ComputeKernelAvgMisorientations(ComputeKernelAvgMisorientations&&) noexcept = delete;
  ComputeKernelAvgMisorientations& operator=(const ComputeKernelAvgMisorientations&) = delete;
  ComputeKernelAvgMisorientations& operator=(ComputeKernelAvgMisorientations&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindKernelAvgMisorientationsInputValues
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
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindKernelAvgMisorientations
{
public:
  FindKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindKernelAvgMisorientationsInputValues* inputValues);
  ~FindKernelAvgMisorientations() noexcept;

  FindKernelAvgMisorientations(const FindKernelAvgMisorientations&) = delete;
  FindKernelAvgMisorientations(FindKernelAvgMisorientations&&) noexcept = delete;
  FindKernelAvgMisorientations& operator=(const FindKernelAvgMisorientations&) = delete;
  FindKernelAvgMisorientations& operator=(FindKernelAvgMisorientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

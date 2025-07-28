#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeFZQuaternionsInputValues
{
  ArraySelectionParameter::ValueType CellPhasesArrayPath;
  ArraySelectionParameter::ValueType CrystalStructuresArrayPath;
  ArraySelectionParameter::ValueType InputQuatsArrayPath;
  ArraySelectionParameter::ValueType MaskArrayPath;
  DataObjectNameParameter::ValueType OutputFzQuatsArrayName;
  BoolParameter::ValueType UseMask;
};

/**
 * @class ComputeFZQuaternions
 * @brief This algorithm implements support code for the ComputeFZQuaternionsFilter
 */

class ORIENTATIONANALYSIS_EXPORT ComputeFZQuaternions
{
public:
  ComputeFZQuaternions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFZQuaternionsInputValues* inputValues);
  ~ComputeFZQuaternions() noexcept;

  ComputeFZQuaternions(const ComputeFZQuaternions&) = delete;
  ComputeFZQuaternions(ComputeFZQuaternions&&) noexcept = delete;
  ComputeFZQuaternions& operator=(const ComputeFZQuaternions&) = delete;
  ComputeFZQuaternions& operator=(ComputeFZQuaternions&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFZQuaternionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

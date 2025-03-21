#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

#include <vector>

namespace nx::core
{

namespace compute_misorientations_constants
{

const ChoicesParameter::Choices k_ComputationTypeStrings = {"Use Arrays", "Use Reference Axis Angle"};
constexpr ChoicesParameter::ValueType k_UseArraysIndex = 0;
constexpr ChoicesParameter::ValueType k_UseReferenceAxesIndex = 1;

} // namespace compute_misorientations_constants

/**
 * @brief
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeMisorientationsInputValues
{
  DataPath InputOrientationPath1;
  DataPath InputOrientationPath2;
  VectorFloat32Parameter::ValueType ReferenceOrientation;
  ChoicesParameter::ValueType ComputationType;
  DataPath InputPhasesArrayPath;
  DataPath OutputMisorientationsPath;
  DataPath InputCrystalStructuresArrayPath;
};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT ComputeMisorientations
{
public:
  ComputeMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeMisorientationsInputValues* inputValues);
  ~ComputeMisorientations() noexcept;

  ComputeMisorientations(const ComputeMisorientations&) = delete;
  ComputeMisorientations(ComputeMisorientations&&) noexcept = delete;
  ComputeMisorientations& operator=(const ComputeMisorientations&) = delete;
  ComputeMisorientations& operator=(ComputeMisorientations&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core

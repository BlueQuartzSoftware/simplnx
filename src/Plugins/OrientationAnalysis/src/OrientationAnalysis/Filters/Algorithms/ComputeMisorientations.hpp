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
 * @struct ComputeMisorientationsInputValues
 * @brief Identifies misorientation inputs and settings.
 *
 * ReferenceOrientation stores an axis and an angle in degrees when
 * ComputationType selects the reference mode.
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
 * @class ComputeMisorientations
 * @brief Computes one axis-angle misorientation for each input tuple.
 *
 * Cell arrays use bounded bulk buffers. Crystal structures remain local for
 * repeated phase lookup.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeMisorientations
{
public:
  /**
   * @brief Initializes misorientation computation.
   * @param dataStructure Provides selected arrays.
   * @param messageHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and computation mode.
   * @pre dataStructure, messageHandler, shouldCancel, and inputValues outlive
   *      this executor.
   */
  ComputeMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ComputeMisorientationsInputValues* inputValues);
  /**
   * @brief Destroys the misorientation executor.
   */
  ~ComputeMisorientations() noexcept;

  ComputeMisorientations(const ComputeMisorientations&) = delete;
  ComputeMisorientations(ComputeMisorientations&&) noexcept = delete;
  ComputeMisorientations& operator=(const ComputeMisorientations&) = delete;
  ComputeMisorientations& operator=(ComputeMisorientations&&) noexcept = delete;

  /**
   * @brief Computes misorientations.
   * @pre Positive phase IDs are within the crystal-structure array.
   * @return Success, or a bulk-I/O error.
   *
   * Cancellation returns success with completed chunks preserved.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeFeatureReferenceMisorientationsInputValues
 * @brief Identifies feature-reference misorientation inputs.
 *
 * ReferenceOrientation selects the average quaternion or the farthest
 * grain-boundary cell.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceMisorientationsInputValues
{
  ChoicesParameter::ValueType ReferenceOrientation;
  DataPath FeatureAttributeMatrixPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath GBEuclideanDistancesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FeatureReferenceMisorientationsArrayName;
  DataPath FeatureAvgMisorientationsArrayName;
  DataPath FeatureEuclideanCentersPath;
};

/**
 * @class ComputeFeatureReferenceMisorientations
 * @brief Computes cell misorientation to each feature reference.
 *
 * The reference is an average quaternion or the farthest grain-boundary cell.
 * Cell data uses 65,536-tuple buffers. Feature and ensemble data stays local
 * because feature IDs access it in random order.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceMisorientations
{
public:
  /**
   * @brief Initializes feature-reference misorientation computation.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and reference mode.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeFeatureReferenceMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                         ComputeFeatureReferenceMisorientationsInputValues* inputValues);
  /**
   * @brief Destroys the feature-reference misorientation executor.
   */
  ~ComputeFeatureReferenceMisorientations() noexcept;

  ComputeFeatureReferenceMisorientations(const ComputeFeatureReferenceMisorientations&) = delete;
  ComputeFeatureReferenceMisorientations(ComputeFeatureReferenceMisorientations&&) noexcept = delete;
  ComputeFeatureReferenceMisorientations& operator=(const ComputeFeatureReferenceMisorientations&) = delete;
  ComputeFeatureReferenceMisorientations& operator=(ComputeFeatureReferenceMisorientations&&) noexcept = delete;

  /**
   * @brief Computes feature-reference misorientations.
   * @pre Positive cell feature and phase IDs are within their selected arrays.
   * @return Result from feature-index validation.
   *
   * Cancellation returns success with completed output chunks preserved.
   * Current bulk-I/O Result values are not inspected.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureReferenceMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

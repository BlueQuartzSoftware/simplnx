#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeFeatureFaceMisorientationInputValues
 * @brief Identifies arrays for feature-face misorientation calculation.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureFaceMisorientationInputValues
{
  DataPath surfaceMeshFaceLabelsArrayPath;
  DataPath avgQuatsArrayPath;
  DataPath featurePhasesArrayPath;
  DataPath crystalStructuresArrayPath;
  DataPath misorientationArrayPath;
};

/**
 * @class ComputeFeatureFaceMisorientation
 * @brief Calculates misorientation across surface-mesh faces with matching positive Phases.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureFaceMisorientation
{
public:
  /**
   * @brief Initializes feature-face misorientation calculation.
   * @param dataStructure Provides the selected arrays and output misorientations.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and output path.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this executor.
   */
  ComputeFeatureFaceMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   ComputeFeatureFaceMisorientationInputValues* inputValues);

  /**
   * @brief Destroys the feature-face misorientation executor.
   */
  ~ComputeFeatureFaceMisorientation() noexcept;

  ComputeFeatureFaceMisorientation(const ComputeFeatureFaceMisorientation&) = delete;
  ComputeFeatureFaceMisorientation(ComputeFeatureFaceMisorientation&&) noexcept = delete;
  ComputeFeatureFaceMisorientation& operator=(const ComputeFeatureFaceMisorientation&) = delete;
  ComputeFeatureFaceMisorientation& operator=(ComputeFeatureFaceMisorientation&&) noexcept = delete;

  /**
   * @brief Calculates misorientation for each eligible face.
   * @return Success, or an error for an invalid participating Phase index or bulk I/O.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureFaceMisorientationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

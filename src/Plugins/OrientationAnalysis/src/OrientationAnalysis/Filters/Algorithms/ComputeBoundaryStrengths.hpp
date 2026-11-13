#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeBoundaryStrengthsInputValues
{
  VectorFloat64Parameter::ValueType Loading;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath SurfaceMeshF1sArrayName;
  DataPath SurfaceMeshF1sptsArrayName;
  DataPath SurfaceMeshF7sArrayName;
  DataPath SurfaceMeshmPrimesArrayName;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeBoundaryStrengths
{
public:
  ComputeBoundaryStrengths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryStrengthsInputValues* inputValues);
  ~ComputeBoundaryStrengths() noexcept;

  ComputeBoundaryStrengths(const ComputeBoundaryStrengths&) = delete;
  ComputeBoundaryStrengths(ComputeBoundaryStrengths&&) noexcept = delete;
  ComputeBoundaryStrengths& operator=(const ComputeBoundaryStrengths&) = delete;
  ComputeBoundaryStrengths& operator=(ComputeBoundaryStrengths&&) noexcept = delete;

  /**
   * @brief Checks the Loading Direction. preflightImpl() and operator()() both call it.
   * @param loading The x, y and z components of the loading direction.
   * @return k_InvalidLoadingDirectionError when a component is NaN or infinite or the direction is [0, 0, 0].
   */
  static Result<> ValidateLoadingDirection(const std::vector<float64>& loading);

  /// Error code for a Loading Direction that is not finite or has length 0.
  static constexpr int32 k_InvalidLoadingDirectionError = -94744;

  /**
   * @brief Computes boundary-strength metrics for internal feature faces.
   * @return An error for an invalid Loading Direction or an invalid phase or Laue index. Returns a warning
   *         when the algorithm processes a non-cubic phase.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryStrengthsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

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
  VectorFloat64ParameterType::ValueType loading;
  DataPath surface_mesh_face_labels_array_path;
  DataPath avg_quats_array_path;
  DataPath feature_phases_array_path;
  DataPath crystal_structures_array_path;
  DataPath surface_mesh_f1s_array_name;
  DataPath surface_mesh_f1spts_array_name;
  DataPath surface_mesh_f7s_array_name;
  DataPath surface_meshm_primes_array_name;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeBoundaryStrengths
{
public:
  ComputeBoundaryStrengths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryStrengthsInputValues* inputValues);
  ~ComputeBoundaryStrengths() noexcept = default;

  ComputeBoundaryStrengths(const ComputeBoundaryStrengths&) = delete;
  ComputeBoundaryStrengths(ComputeBoundaryStrengths&&) noexcept = delete;
  ComputeBoundaryStrengths& operator=(const ComputeBoundaryStrengths&) = delete;
  ComputeBoundaryStrengths& operator=(ComputeBoundaryStrengths&&) noexcept = delete;

  /**
   * @brief Computes boundary-strength metrics for internal feature faces.
   * @return An error for an invalid phase or Laue index. Returns a warning
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

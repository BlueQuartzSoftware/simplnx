#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureNeighborMisorientationsInputValues
{
  bool compute_avg_misors{};
  DataPath neighbor_list_array_path;
  DataPath avg_quats_array_path;
  DataPath feature_phases_array_path;
  DataPath crystal_structures_array_path;
  DataPath misorientation_list_array_name;
  DataPath avg_misorientations_array_name;
};

/**
 * @class ComputeFeatureNeighborMisorientations
 * @brief Computes crystallographic misorientations between feature neighbors.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureNeighborMisorientations
{
public:
  ComputeFeatureNeighborMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        ComputeFeatureNeighborMisorientationsInputValues* inputValues);
  ~ComputeFeatureNeighborMisorientations() noexcept = default;

  ComputeFeatureNeighborMisorientations(const ComputeFeatureNeighborMisorientations&) = delete;
  ComputeFeatureNeighborMisorientations(ComputeFeatureNeighborMisorientations&&) noexcept = delete;
  ComputeFeatureNeighborMisorientations& operator=(const ComputeFeatureNeighborMisorientations&) = delete;
  ComputeFeatureNeighborMisorientations& operator=(ComputeFeatureNeighborMisorientations&&) noexcept = delete;

  /**
   * @brief Computes neighbor misorientation lists and optional averages.
   * @return An error for an invalid Feature Phase index.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

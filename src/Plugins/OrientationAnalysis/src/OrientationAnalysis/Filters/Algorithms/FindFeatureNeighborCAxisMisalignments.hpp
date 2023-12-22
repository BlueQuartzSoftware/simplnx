#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT FindFeatureNeighborCAxisMisalignmentsInputValues
{
  bool FindAvgMisals;
  DataPath NeighborListArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CAxisMisalignmentListArrayName;
  DataPath AvgCAxisMisalignmentsArrayName;
};

/**
 * @class FindFeatureNeighborCAxisMisalignments
 * @brief This filter determines, for each Feature, the C-axis mis alignments with the Features that are in contact with it.
 */

class ORIENTATIONANALYSIS_EXPORT FindFeatureNeighborCAxisMisalignments
{
public:
  FindFeatureNeighborCAxisMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        FindFeatureNeighborCAxisMisalignmentsInputValues* inputValues);
  ~FindFeatureNeighborCAxisMisalignments() noexcept;

  FindFeatureNeighborCAxisMisalignments(const FindFeatureNeighborCAxisMisalignments&) = delete;
  FindFeatureNeighborCAxisMisalignments(FindFeatureNeighborCAxisMisalignments&&) noexcept = delete;
  FindFeatureNeighborCAxisMisalignments& operator=(const FindFeatureNeighborCAxisMisalignments&) = delete;
  FindFeatureNeighborCAxisMisalignments& operator=(FindFeatureNeighborCAxisMisalignments&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureNeighborCAxisMisalignmentsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

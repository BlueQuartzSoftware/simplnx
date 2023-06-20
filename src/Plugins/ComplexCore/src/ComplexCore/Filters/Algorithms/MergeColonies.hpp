#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT MergeColoniesInputValues
{
  bool UseNonContiguousNeighbors;
  DataPath NonContiguousNeighborListArrayPath;
  DataPath ContiguousNeighborListArrayPath;
  float32 AxisTolerance;
  float32 AngleTolerance;
  DataPath FeaturePhasesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CellParentIdsArrayName;
  DataPath GlobAlphaArrayName;
  DataPath NewCellFeatureAttributeMatrixName;
  DataPath FeatureParentIdsArrayName;
  DataPath ActiveArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT MergeColonies
{
public:
  MergeColonies(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeColoniesInputValues* inputValues);
  ~MergeColonies() noexcept;

  MergeColonies(const MergeColonies&) = delete;
  MergeColonies(MergeColonies&&) noexcept = delete;
  MergeColonies& operator=(const MergeColonies&) = delete;
  MergeColonies& operator=(MergeColonies&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MergeColoniesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

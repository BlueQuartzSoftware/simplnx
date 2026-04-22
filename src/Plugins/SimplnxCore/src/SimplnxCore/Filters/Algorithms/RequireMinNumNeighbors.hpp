#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT RequireMinNumNeighborsInputValues
{
  bool ApplyToSinglePhase;
  DataPath FeaturePhasesPath;
  uint64 PhaseNumber;
  uint64 MinNumNeighbors;
  DataPath ImageGeomPath;
  DataPath FeatureIdsPath;
  DataPath NumNeighborsPath;
  MultiArraySelectionParameter::ValueType IgnoredVoxelArrayPaths;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT RequireMinNumNeighbors
{
public:
  RequireMinNumNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RequireMinNumNeighborsInputValues* inputValues);
  ~RequireMinNumNeighbors() noexcept;

  RequireMinNumNeighbors(const RequireMinNumNeighbors&) = delete;
  RequireMinNumNeighbors(RequireMinNumNeighbors&&) noexcept = delete;
  RequireMinNumNeighbors& operator=(const RequireMinNumNeighbors&) = delete;
  RequireMinNumNeighbors& operator=(RequireMinNumNeighbors&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> CheckForAvailablePhase();
  Result<> CheckNumNeighbors(Int32AbstractDataStore& numNeighbors, usize totalFeatures, std::vector<bool>& activeObjects);
  Result<> FindFeatureCount(ImageGeom& imageGeom, std::vector<DataPath>& cellDataArrayPaths, Int32AbstractDataStore& featureIds, Int32AbstractDataStore& numNeighbors);

private:
  DataStructure& m_DataStructure;
  const RequireMinNumNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

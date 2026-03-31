#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

#include <random>

namespace nx::core
{
struct SIMPLNXCORE_EXPORT DBSCANInputValues
{
  DataPath ClusteringArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  float32 Epsilon;
  int32 MinPoints;
  ClusterUtilities::DistanceMetric DistanceMetric;
  DataPath FeatureAM;
  ChoicesParameter::ValueType ParseOrder;
  std::mt19937_64::result_type Seed;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT DBSCANDirect
{
public:
  DBSCANDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues);
  ~DBSCANDirect() noexcept;

  DBSCANDirect(const DBSCANDirect&) = delete;
  DBSCANDirect(DBSCANDirect&&) noexcept = delete;
  DBSCANDirect& operator=(const DBSCANDirect&) = delete;
  DBSCANDirect& operator=(DBSCANDirect&&) noexcept = delete;

  enum ParseOrder
  {
    LowDensityFirst,
    Random,
    SeededRandom
  };

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const DBSCANInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

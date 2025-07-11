#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT ComputeBoundingBoxStatsInputValues
{
  // Options
  bool CalculateLength;
  bool CalculateMin;
  bool CalculateMax;
  bool CalculateSummation;
  bool CalculateMean;
  bool CalculateMedian;
  bool CalculateMode;
  bool CalculateNumUniqueValues;
  bool CalculateStdDev;

  // Input
  DataPath GeometryPath;
  DataPath UnifiedPath;
  DataPath InputPath;

  // Output
  DataPath BoundsHasDataPath;
  DataPath LengthPath;
  DataPath MinPath;
  DataPath MaxPath;
  DataPath SummationPath;
  DataPath MeanPath;
  DataPath MedianPath;
  DataPath ModePath;
  DataPath NumUniqueValuesPath;
  DataPath StdDevPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeBoundingBoxStats
{
public:
  ComputeBoundingBoxStats(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundingBoxStatsInputValues* inputValues);
  ~ComputeBoundingBoxStats() noexcept;

  ComputeBoundingBoxStats(const ComputeBoundingBoxStats&) = delete;
  ComputeBoundingBoxStats(ComputeBoundingBoxStats&&) noexcept = delete;
  ComputeBoundingBoxStats& operator=(const ComputeBoundingBoxStats&) = delete;
  ComputeBoundingBoxStats& operator=(ComputeBoundingBoxStats&&) noexcept = delete;

  enum OutputDataType : uint8
  {
    Split = 0,
    Unified = 1
  };

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundingBoxStatsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core

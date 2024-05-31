#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreateAMScanPathsInputValues
{
  float32 StripeWidth;
  float32 HatchSpacing;
  float32 Power;
  float32 Speed;
  DataPath CADSliceDataContainerName;
  DataPath CADSliceIdsArrayPath;
  DataPath CADRegionIdsArrayPath;
  DataPath HatchDataContainerName;
  DataObjectNameParameter::ValueType VertexAttributeMatrixName;
  DataObjectNameParameter::ValueType HatchAttributeMatrixName;
  DataObjectNameParameter::ValueType TimeArrayName;
  DataObjectNameParameter::ValueType RegionIdsArrayName;
  DataObjectNameParameter::ValueType PowersArrayName;
};

/**
 * @class CreateAMScanPaths
 * @brief This filter will generate additive manufacturing scan paths from an edge geometry
 */

class SIMPLNXCORE_EXPORT CreateAMScanPaths
{
public:
  CreateAMScanPaths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAMScanPathsInputValues* inputValues);
  ~CreateAMScanPaths() noexcept;

  CreateAMScanPaths(const CreateAMScanPaths&) = delete;
  CreateAMScanPaths(CreateAMScanPaths&&) noexcept = delete;
  CreateAMScanPaths& operator=(const CreateAMScanPaths&) = delete;
  CreateAMScanPaths& operator=(CreateAMScanPaths&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  char determineIntersectCoord(const std::array<float32, 2>& p1, const std::array<float32, 2>& q1, const std::array<float32, 2>& p2, const std::array<float32, 2>& q2, float32& coordX);

private:
  DataStructure& m_DataStructure;
  const CreateAMScanPathsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

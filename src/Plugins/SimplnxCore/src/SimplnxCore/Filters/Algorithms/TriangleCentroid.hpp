#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT TriangleCentroidInputValues
{
  DataPath TriangleGeometryDataPath;
  DataObjectNameParameter::ValueType CentroidsArrayName;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT TriangleCentroid
{
public:
  TriangleCentroid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TriangleCentroidInputValues* inputValues);
  ~TriangleCentroid() noexcept;

  TriangleCentroid(const TriangleCentroid&) = delete;
  TriangleCentroid(TriangleCentroid&&) noexcept = delete;
  TriangleCentroid& operator=(const TriangleCentroid&) = delete;
  TriangleCentroid& operator=(TriangleCentroid&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const TriangleCentroidInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

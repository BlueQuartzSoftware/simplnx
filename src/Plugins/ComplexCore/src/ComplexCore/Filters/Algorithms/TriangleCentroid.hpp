#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT TriangleCentroidInputValues
{
  DataPath TriangleGeometryDataPath;
  DataObjectNameParameter::ValueType CentroidsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT TriangleCentroid
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

} // namespace complex

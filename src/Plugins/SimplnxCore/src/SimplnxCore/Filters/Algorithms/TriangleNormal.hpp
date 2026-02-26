#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT TriangleNormalInputValues
{
  GeometrySelectionParameter::ValueType InputTriangleGeometryPath;
  DataObjectNameParameter::ValueType OutputNormalsArrayName;
};

/**
 * @class TriangleNormal
 * @brief This algorithm implements support code for the TriangleNormalFilter
 */

class SIMPLNXCORE_EXPORT TriangleNormal
{
public:
  TriangleNormal(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TriangleNormalInputValues* inputValues);
  ~TriangleNormal() noexcept;

  TriangleNormal(const TriangleNormal&) = delete;
  TriangleNormal(TriangleNormal&&) noexcept = delete;
  TriangleNormal& operator=(const TriangleNormal&) = delete;
  TriangleNormal& operator=(TriangleNormal&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const TriangleNormalInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

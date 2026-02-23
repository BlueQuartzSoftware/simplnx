#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReverseTriangleWindingInputValues
{
  GeometrySelectionParameter::ValueType InputTriangleGeometryPath;
};

/**
 * @class ReverseTriangleWinding
 * @brief This algorithm implements support code for the ReverseTriangleWindingFilter
 */

class SIMPLNXCORE_EXPORT ReverseTriangleWinding
{
public:
  ReverseTriangleWinding(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReverseTriangleWindingInputValues* inputValues);
  ~ReverseTriangleWinding() noexcept;

  ReverseTriangleWinding(const ReverseTriangleWinding&) = delete;
  ReverseTriangleWinding(ReverseTriangleWinding&&) noexcept = delete;
  ReverseTriangleWinding& operator=(const ReverseTriangleWinding&) = delete;
  ReverseTriangleWinding& operator=(ReverseTriangleWinding&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReverseTriangleWindingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT NearestPointFuseRegularGridsInputValues
{
  DataPath ReferenceGeometryPath;
  DataPath SamplingGeometryPath;
  DataPath ReferenceCellAttributeMatrixPath;
  DataPath SamplingCellAttributeMatrixPath;
  float64 fillValue;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT NearestPointFuseRegularGrids
{
public:
  NearestPointFuseRegularGrids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, NearestPointFuseRegularGridsInputValues* inputValues);
  ~NearestPointFuseRegularGrids() noexcept;

  NearestPointFuseRegularGrids(const NearestPointFuseRegularGrids&) = delete;
  NearestPointFuseRegularGrids(NearestPointFuseRegularGrids&&) noexcept = delete;
  NearestPointFuseRegularGrids& operator=(const NearestPointFuseRegularGrids&) = delete;
  NearestPointFuseRegularGrids& operator=(NearestPointFuseRegularGrids&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const NearestPointFuseRegularGridsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

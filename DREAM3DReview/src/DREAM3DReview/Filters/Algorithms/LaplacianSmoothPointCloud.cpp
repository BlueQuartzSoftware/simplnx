#include "LaplacianSmoothPointCloud.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
LaplacianSmoothPointCloud::LaplacianSmoothPointCloud(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     LaplacianSmoothPointCloudInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
LaplacianSmoothPointCloud::~LaplacianSmoothPointCloud() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& LaplacianSmoothPointCloud::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> LaplacianSmoothPointCloud::operator()()
{

  return {};
}

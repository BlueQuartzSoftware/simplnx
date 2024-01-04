#include "GeneratePythonSkeleton.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <Eigen/Dense>

using namespace nx::core;

namespace
{
// typedef Eigen::Array<float, 3, 1> ArrayType;
typedef Eigen::Map<Eigen::Vector3f> VectorMapType;
} // namespace

// -----------------------------------------------------------------------------
GeneratePythonSkeleton::GeneratePythonSkeleton(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GeneratePythonSkeletonInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GeneratePythonSkeleton::~GeneratePythonSkeleton() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GeneratePythonSkeleton::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GeneratePythonSkeleton::operator()()
{

  auto result = nx::core::WritePythonPluginFiles(m_InputValues->pluginOutputDir, m_InputValues->pluginName,
                                                m_InputValues->pluginName,
                                               "Description",  m_InputValues->filterNames);


  return {};
}

#include "IParallelAlgorithm.hpp"

#include "simplnx/Core/Application.hpp"

namespace nx::core
{
// -----------------------------------------------------------------------------
bool IParallelAlgorithm::CheckArraysInMemory(const AlgorithmArrays& arrays)
{
  if(arrays.empty())
  {
    return true;
  }

  for(const auto* arrayPtr : arrays)
  {
    if(arrayPtr == nullptr)
    {
      continue;
    }

    if(!arrayPtr->getIDataStoreRef().getDataFormat().empty())
    {
      return false;
    }
  }

  return true;
}

// -----------------------------------------------------------------------------
IParallelAlgorithm::IParallelAlgorithm()
{
#ifdef SIMPLNX_ENABLE_MULTICORE
  // Do not run OOC data in parallel by default.
  m_RunParallel = !Application::GetOrCreateInstance()->getPreferences()->useOocData();
#endif
}

// -----------------------------------------------------------------------------
IParallelAlgorithm::~IParallelAlgorithm() = default;

// -----------------------------------------------------------------------------
bool IParallelAlgorithm::getParallelizationEnabled() const
{
  return m_RunParallel;
}

// -----------------------------------------------------------------------------
void IParallelAlgorithm::setParallelizationEnabled(bool doParallel)
{
#ifdef SIMPLNX_ENABLE_MULTICORE
  m_RunParallel = doParallel;
#endif
}
// -----------------------------------------------------------------------------
void IParallelAlgorithm::requireArraysInMemory(const AlgorithmArrays& arrays)
{
  setParallelizationEnabled(CheckArraysInMemory(arrays));
}
} // namespace nx::core

#include "ParallelAlgorithm.hpp"

#include "simplnx/Core/Application.hpp"

namespace
{
// -----------------------------------------------------------------------------
bool CheckStoresInMemory(const nx::core::ParallelAlgorithm::AlgorithmStores& stores)
{
  if(stores.empty())
  {
    return true;
  }

  for(const auto* storePtr : stores)
  {
    if(storePtr == nullptr)
    {
      continue;
    }

    if(!storePtr->getDataFormat().empty())
    {
      return false;
    }
  }

  return true;
}

// -----------------------------------------------------------------------------
bool CheckArraysInMemory(const nx::core::ParallelAlgorithm::AlgorithmArrays& arrays)
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
} // namespace

namespace nx::core
{
// -----------------------------------------------------------------------------
ParallelAlgorithm::ParallelAlgorithm()
{
#ifdef SIMPLNX_ENABLE_MULTICORE
  // Do not run OOC data in parallel by default.
  m_RunParallel = !Application::GetOrCreateInstance()->getPreferences()->useOocData();
#endif
}

// -----------------------------------------------------------------------------
ParallelAlgorithm::~ParallelAlgorithm() = default;

// -----------------------------------------------------------------------------
bool ParallelAlgorithm::getParallelizationEnabled() const
{
  return m_RunParallel;
}

// -----------------------------------------------------------------------------
void ParallelAlgorithm::setParallelizationEnabled(bool doParallel)
{
#ifdef SIMPLNX_ENABLE_MULTICORE
  m_RunParallel = doParallel;
#endif
}
// -----------------------------------------------------------------------------
void ParallelAlgorithm::requireArraysInMemory(const AlgorithmArrays& arrays)
{
  setParallelizationEnabled(CheckArraysInMemory(arrays));
}

// -----------------------------------------------------------------------------
void ParallelAlgorithm::requireStoresInMemory(const AlgorithmStores& stores)
{
  setParallelizationEnabled(::CheckStoresInMemory(stores));
}
} // namespace nx::core

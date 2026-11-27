#include "IParallelAlgorithm.hpp"

#include "simplnx/DataStructure/IDataStore.hpp"

namespace
{
// -----------------------------------------------------------------------------
bool CheckStoresInMemory(const nx::core::IParallelAlgorithm::AlgorithmStoresType& stores)
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

    if(storePtr->getStoreType() == nx::core::IDataStore::StoreType::OutOfCore)
    {
      return false;
    }
  }

  return true;
}

// -----------------------------------------------------------------------------
bool CheckArraysInMemory(const nx::core::IParallelAlgorithm::AlgorithmArraysType& arrays)
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

    if(arrayPtr->getIDataStoreRef().getStoreType() == nx::core::IDataStore::StoreType::OutOfCore)
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
IParallelAlgorithm::IParallelAlgorithm()
{
  // Out-of-core stores do not support concurrent DataArray or IDataStore access.
  // requireArraysInMemory() and requireStoresInMemory() disable parallel work for these stores.
}

// -----------------------------------------------------------------------------

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
void IParallelAlgorithm::requireArraysInMemory(const AlgorithmArraysType& arrays)
{
  setParallelizationEnabled(getParallelizationEnabled() && CheckArraysInMemory(arrays));
}

// -----------------------------------------------------------------------------
void IParallelAlgorithm::requireStoresInMemory(const AlgorithmStoresType& stores)
{
  setParallelizationEnabled(getParallelizationEnabled() && ::CheckStoresInMemory(stores));
}
} // namespace nx::core

#include "IParallelAlgorithm.hpp"

#include "complex/Core/Application.hpp"

namespace complex
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
#ifdef COMPLEX_ENABLE_MULTICORE
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
#ifdef COMPLEX_ENABLE_MULTICORE
  m_RunParallel = doParallel;
#endif
}
// -----------------------------------------------------------------------------
void IParallelAlgorithm::requireArraysInMemory(const AlgorithmArrays& arrays)
{
  setParallelizationEnabled(!CheckArraysInMemory(arrays));
}
} // namespace complex

#include "IParallelAlgorithm.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/Preferences.hpp"

namespace complex
{
// -----------------------------------------------------------------------------
bool IParallelAlgorithm::CheckArraysInMemory(const AlgorithmArrays& arrays)
{
  if(arrays.size() == 0)
  {
    true;
  }

  for(const auto* array : arrays)
  {
    if(array == nullptr)
    {
      continue;
    }

    if(array->getIDataStoreRef().getDataFormat().empty() == false)
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
  bool inMemory = CheckArraysInMemory(arrays);
  setParallelizationEnabled(!inMemory);
}
} // namespace complex

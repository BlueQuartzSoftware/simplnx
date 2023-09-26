#include "IParallelAlgorithm.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/Preferences.hpp"

namespace complex
{
IParallelAlgorithm::IParallelAlgorithm()
{
#ifdef COMPLEX_ENABLE_MULTICORE
  // Do not run OOC data in parallel by default.
  m_RunParallel = !Application::GetOrCreateInstance()->getPreferences()->useOocData();
#endif
}
IParallelAlgorithm::~IParallelAlgorithm() = default;

// -----------------------------------------------------------------------------
bool IParallelAlgorithm::getParallelizationEnabled() const
{
  return m_RunParallel;
}

// -----------------------------------------------------------------------------
void IParallelAlgorithm::setParallelizationEnabled(bool doParallel)
{
  m_RunParallel = doParallel;
}
// -----------------------------------------------------------------------------
void IParallelAlgorithm::requireArraysInMemory(const std::vector<const IDataArray*>& arrays)
{
  for(const auto* array : arrays)
  {
    if(array->getIDataStoreRef().getDataFormat().empty() == false)
    {
      setParallelizationEnabled(false);
      return;
    }
  }
  setParallelizationEnabled(true);
}
} // namespace complex

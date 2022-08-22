#include "ParallelTaskAlgorithm.hpp"

#include <algorithm>
#include <thread>

using namespace complex;

// -----------------------------------------------------------------------------
ParallelTaskAlgorithm::ParallelTaskAlgorithm()
: m_Parallelization(true)
, m_MaxThreads(std::thread::hardware_concurrency())
#ifdef COMPLEX_ENABLE_MULTICORE
, m_TaskGroup(new tbb::task_group)
#endif
{
}

// -----------------------------------------------------------------------------
ParallelTaskAlgorithm::~ParallelTaskAlgorithm()
{
#ifdef COMPLEX_ENABLE_MULTICORE
  m_TaskGroup->wait();
#endif
}

// -----------------------------------------------------------------------------
bool ParallelTaskAlgorithm::getParallelizationEnabled() const
{
  return m_Parallelization;
}

// -----------------------------------------------------------------------------
void ParallelTaskAlgorithm::setParallelizationEnabled(bool doParallel)
{
  m_Parallelization = doParallel;
}

// -----------------------------------------------------------------------------
uint32_t ParallelTaskAlgorithm::getMaxThreads() const
{
#ifdef COMPLEX_ENABLE_MULTICORE
  return m_MaxThreads;
#else
  return std::thread::hardware_concurrency();
#endif
}

// -----------------------------------------------------------------------------
void ParallelTaskAlgorithm::setMaxThreads(uint32_t threads)
{
  m_MaxThreads = std::min(threads, std::thread::hardware_concurrency());
}

// -----------------------------------------------------------------------------
void ParallelTaskAlgorithm::wait()
{
#ifdef COMPLEX_ENABLE_MULTICORE
  // This will spill over if the number of files to process does not divide evenly by the number of threads.
  m_TaskGroup->wait();
  m_CurThreads = 0;
#endif
}

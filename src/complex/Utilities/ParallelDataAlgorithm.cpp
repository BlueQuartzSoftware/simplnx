#include "ParallelDataAlgorithm.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ParallelDataAlgorithm::ParallelDataAlgorithm() = default;

// -----------------------------------------------------------------------------
ParallelDataAlgorithm::~ParallelDataAlgorithm() = default;

// -----------------------------------------------------------------------------
bool ParallelDataAlgorithm::getParallelizationEnabled() const
{
  return m_RunParallel;
}

// -----------------------------------------------------------------------------
void ParallelDataAlgorithm::setParallelizationEnabled(bool doParallel)
{
  m_RunParallel = doParallel;
}

// -----------------------------------------------------------------------------
Range ParallelDataAlgorithm::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
void ParallelDataAlgorithm::setRange(const Range& range)
{
  m_Range = range;
}

// -----------------------------------------------------------------------------
void ParallelDataAlgorithm::setRange(size_t min, size_t max)
{
  m_Range = {min, max};
}

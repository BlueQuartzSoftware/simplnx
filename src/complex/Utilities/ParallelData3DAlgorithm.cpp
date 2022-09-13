#include "ParallelData3DAlgorithm.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ParallelData3DAlgorithm::ParallelData3DAlgorithm() = default;

// -----------------------------------------------------------------------------
ParallelData3DAlgorithm::~ParallelData3DAlgorithm() = default;

// -----------------------------------------------------------------------------
bool ParallelData3DAlgorithm::getParallelizationEnabled() const
{
  return m_RunParallel;
}

// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setParallelizationEnabled(bool doParallel)
{
  m_RunParallel = doParallel;
}

// -----------------------------------------------------------------------------
ParallelData3DAlgorithm::RangeType ParallelData3DAlgorithm::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setRange(const RangeType& range)
{
  m_Range = range;
}

// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setRange(size_t xMax, size_t yMax, size_t zMax)
{
  m_Range = {0, xMax, 0, yMax, 0, zMax};
}

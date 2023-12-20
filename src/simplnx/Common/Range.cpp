#include "simplnx/Common/Range.hpp"

#include <stdexcept>
using namespace nx::core;

// -----------------------------------------------------------------------------
Range::Range()
: m_Range({{0, 0}})
{
}

// -----------------------------------------------------------------------------
Range::Range(size_t begin, size_t end)
: m_Range({{begin, end}})
{
}
#ifdef SIMPLNX_ENABLE_MULTICORE
// -----------------------------------------------------------------------------
Range::Range(const tbb::blocked_range<size_t>& r)
: m_Range({{r.begin(), r.end()}})
{
}
#endif

// -----------------------------------------------------------------------------
Range::RangeType Range::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
size_t Range::min() const
{
  return m_Range[0];
}

// -----------------------------------------------------------------------------
size_t Range::max() const
{
  return m_Range[1];
}

// -----------------------------------------------------------------------------
size_t Range::size() const
{
  return m_Range[1] - m_Range[0];
}

// -----------------------------------------------------------------------------
bool Range::empty() const
{
  return size() == 0;
}

// -----------------------------------------------------------------------------
size_t Range::operator[](size_t index) const
{
  switch(index)
  {
  case 0:
    return min();
  case 1:
    return max();
  default:
    throw std::range_error("Range must be 0 or 1");
  }
}

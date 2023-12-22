#include "simplnx/Common/Range2D.hpp"

#include <stdexcept>
using namespace nx::core;
// -----------------------------------------------------------------------------
Range2D::Range2D()
: m_Range({{0, 0, 0, 0}})
{
}

// -----------------------------------------------------------------------------
Range2D::Range2D(size_t colStart, size_t colEnd, size_t rowStart, size_t rowEnd)
: m_Range({{colStart, colEnd, rowStart, rowEnd}})
{
}

#ifdef SIMPLNX_ENABLE_MULTICORE
// -----------------------------------------------------------------------------
Range2D::Range2D(const tbb::blocked_range2d<size_t, size_t>& r)
: m_Range({{r.cols().begin(), r.cols().end(), r.rows().begin(), r.rows().end()}})
{
}
#endif

// -----------------------------------------------------------------------------
Range2D::RangeType Range2D::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
size_t Range2D::minRow() const
{
  return m_Range[2];
}

// -----------------------------------------------------------------------------
size_t Range2D::minCol() const
{
  return m_Range[0];
}

// -----------------------------------------------------------------------------
size_t Range2D::maxRow() const
{
  return m_Range[3];
}

// -----------------------------------------------------------------------------
size_t Range2D::maxCol() const
{
  return m_Range[1];
}

// -----------------------------------------------------------------------------
size_t Range2D::numRows() const
{
  return maxRow() - minRow();
}

// -----------------------------------------------------------------------------
size_t Range2D::numCols() const
{
  return maxCol() - minCol();
}

// -----------------------------------------------------------------------------
size_t Range2D::size() const
{
  return numRows() * numCols();
}

// -----------------------------------------------------------------------------
bool Range2D::empty() const
{
  const bool emptyRows = (m_Range[2] == m_Range[3]) && (m_Range[2] == 0);
  const bool emptyCols = (m_Range[0] == m_Range[1]) && (m_Range[0] == 0);
  return emptyRows && emptyCols;
}

// -----------------------------------------------------------------------------
size_t Range2D::operator[](size_t index) const
{
  if(index < 4)
  {
    return m_Range[index];
  }
  throw std::range_error("Index out of Range error. Must be less than 4.");
}

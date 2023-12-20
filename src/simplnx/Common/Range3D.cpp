#include "simplnx/Common/Range3D.hpp"

#include <stdexcept>

using namespace nx::core;
// -----------------------------------------------------------------------------
Range3D::Range3D()
: m_Range({{0, 0, 0, 0, 0, 0}})
{
}

// -----------------------------------------------------------------------------
Range3D::Range3D(size_t x, size_t y, size_t z)
: m_Range({{0, x, 0, y, 0, z}})
{
}

// -----------------------------------------------------------------------------
Range3D::Range3D(size_t xMin, size_t xMax, size_t yMin, size_t yMax, size_t zMin, size_t zMax)
: m_Range({{xMin, xMax, yMin, yMax, zMin, zMax}})
{
}

#ifdef SIMPLNX_ENABLE_MULTICORE
// -----------------------------------------------------------------------------
Range3D::Range3D(const tbb::blocked_range3d<size_t, size_t, size_t>& r)
: m_Range({{r.cols().begin(), r.cols().end(), r.rows().begin(), r.rows().end(), r.pages().begin(), r.pages().end()}})
{
}
#endif

// -----------------------------------------------------------------------------
Range3D::RangeType Range3D::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
Range3D::DimensionRange Range3D::getXRange() const
{
  return {{m_Range[0], m_Range[1]}};
}

// -----------------------------------------------------------------------------
Range3D::DimensionRange Range3D::getYRange() const
{
  return {{m_Range[2], m_Range[3]}};
}

// -----------------------------------------------------------------------------
Range3D::DimensionRange Range3D::getZRange() const
{
  return {{m_Range[4], m_Range[5]}};
}

// -----------------------------------------------------------------------------
bool Range3D::empty() const
{
  return (m_Range[0] == m_Range[1]) && (m_Range[2] == m_Range[3]) && (m_Range[4] == m_Range[5]);
}

// -----------------------------------------------------------------------------
size_t Range3D::operator[](size_t index) const
{
  if(index < 6)
  {
    return m_Range[index];
  }

  throw std::range_error("Index out of Range. Must be less than 6");
}

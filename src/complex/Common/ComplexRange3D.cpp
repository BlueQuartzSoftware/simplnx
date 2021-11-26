/* ============================================================================
 * Copyright (c) 2019 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The code contained herein was partially funded by the following contracts:
 *    United States Air Force Prime Contract FA8650-15-D-5231
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "complex/Common/ComplexRange3D.hpp"

#include <stdexcept>
using namespace complex;
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange3D::ComplexRange3D()
: m_Range({{0, 0, 0, 0, 0, 0}})
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange3D::ComplexRange3D(size_t x, size_t y, size_t z)
: m_Range({{0, x, 0, y, 0, z}})
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange3D::ComplexRange3D(size_t xMin, size_t xMax, size_t yMin, size_t yMax, size_t zMin, size_t zMax)
: m_Range({{xMin, xMax, yMin, yMax, zMin, zMax}})
{
}

#ifdef COMPLEX_ENABLE_MULTICORE
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange3D::ComplexRange3D(const tbb::blocked_range3d<size_t>& r)
: m_Range({{r.pages().begin(), r.pages().end(), r.rows().begin(), r.rows().end(), r.cols().begin(), r.cols().end()}})
{
}
#endif

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange3D::RangeType ComplexRange3D::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange3D::DimensionRange ComplexRange3D::getXRange() const
{
  return {{m_Range[0], m_Range[1]}};
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange3D::DimensionRange ComplexRange3D::getYRange() const
{
  return {{m_Range[2], m_Range[3]}};
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange3D::DimensionRange ComplexRange3D::getZRange() const
{
  return {{m_Range[4], m_Range[5]}};
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool ComplexRange3D::empty() const
{
  return (m_Range[0] == m_Range[1]) && (m_Range[2] == m_Range[3]) && (m_Range[4] == m_Range[5]);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange3D::operator[](size_t index) const
{
  if(index < 6)
  {
    return m_Range[index];
  }

  throw std::range_error("Range must be 0 or 1");
}

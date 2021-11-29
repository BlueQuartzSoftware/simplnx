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

#include "complex/Common/ComplexRange2D.hpp"

#include <stdexcept>
using namespace complex;
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange2D::ComplexRange2D()
: m_Range({{0, 0, 0, 0}})
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange2D::ComplexRange2D(size_t initRow, size_t initCol, size_t endRow, size_t endCol)
: m_Range({{initRow, initCol, endRow, endCol}})
{
}

#ifdef COMPLEX_ENABLE_MULTICORE
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange2D::ComplexRange2D(const tbb::blocked_range2d<size_t, size_t>& r)
: m_Range({{r.rows().begin(), r.cols().begin(), r.rows().end(), r.cols().end()}})
{
}
#endif

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange2D::RangeType ComplexRange2D::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange2D::minRow() const
{
  return m_Range[0];
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange2D::minCol() const
{
  return m_Range[1];
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange2D::maxRow() const
{
  return m_Range[2];
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange2D::maxCol() const
{
  return m_Range[3];
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange2D::numRows() const
{
  return maxRow() - minRow();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange2D::numCols() const
{
  return maxCol() - minCol();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange2D::size() const
{
  return numRows() * numCols();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool ComplexRange2D::empty() const
{
  const bool emptyRows = (m_Range[0] == m_Range[2]) && (m_Range[0] == 0);
  const bool emptyCols = (m_Range[1] == m_Range[3]) && (m_Range[1] == 0);
  return emptyRows && emptyCols;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange2D::operator[](size_t index) const
{
  if(index < 4)
  {
    return m_Range[index];
  }
  throw std::range_error("Range out of bounds");
}

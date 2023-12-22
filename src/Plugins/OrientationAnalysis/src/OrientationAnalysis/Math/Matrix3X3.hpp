#pragma once

#include "Matrix3X1.hpp"

#include <nonstd/span.hpp>

#include <cmath>
#include <memory>

namespace nx::core
{
/**
 * @brief 3X3 Matrix that is row major, i.e., the data is laid out in memory as follows:
 * Row major 3x3 matrix
 * 0  1  2
 * 3  4  5
 * 6  7  8
 *
 * @tparam T
 */
template <typename T>
class Matrix3X3
{
public:
  using SelfType = Matrix3X3<T>;
  Matrix3X3() = default;

  /**
   * @brief Copies the values from the pointer.
   * @param ptr
   */
  Matrix3X3(T* ptr)
  : m_Data(std::array<T, 9>{ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8]})
  {
  }

  /**
   * @brief Copies the values for the matrix from the arguments
   * @param v0
   * @param v1
   * @param v2
   * @param v3
   * @param v4
   * @param v5
   * @param v6
   * @param v7
   * @param v8
   */
  Matrix3X3(T v0, T v1, T v2, T v3, T v4, T v5, T v6, T v7, T v8)
  : m_Data(std::array<T, 9>{v0, v1, v2, v3, v4, v5, v6, v7, v8})
  {
  }

  /**
   * @brief Creates a 3x3 Matrix from the "C" style 2D Matrix. This assumes the data
   * is properly row major, i.e., the fastest moving dimension is the second. So to
   * be clear, the each row is rastered before the next row.
   * @param g
   */
  Matrix3X3(const T g[3][3])
  {
    m_Data[0] = g[0][0];
    m_Data[1] = g[0][1];
    m_Data[2] = g[0][2];
    m_Data[3] = g[1][0];
    m_Data[4] = g[1][1];
    m_Data[5] = g[1][2];
    m_Data[6] = g[2][0];
    m_Data[7] = g[2][1];
    m_Data[8] = g[2][2];
  }

  Matrix3X3(const Matrix3X3&) = default;                // Copy Constructor Default Implemented
  Matrix3X3(Matrix3X3&&) noexcept = default;            // Move Constructor Default Implemented
  Matrix3X3& operator=(const Matrix3X3&) = default;     // Copy Assignment Default Implemented
  Matrix3X3& operator=(Matrix3X3&&) noexcept = default; // Move Assignment Default Implemented

  ~Matrix3X3() = default;

  /**
   * @brief Returns a reference to the value at index
   * @param index
   * @return
   */
  T& operator[](size_t index)
  {
    return m_Data[index]; // No bounds checking.. living life on the edge.
  }

  /**
   * @brief Returns a reference to the value at index
   * @param index
   * @return
   */
  const T& operator[](size_t index) const
  {
    return m_Data[index];
  }

  /**
   * @brief Returns a pointer to the underlying data
   * @return
   */
  T* data()
  {
    return m_Data.data();
  }

  /**
   * @brief Performs the Matrix Multiplication returns the result into outMat.
   * @param rhs
   * @param outMat
   */
  Matrix3X3 operator*(const SelfType& rhs) const
  {
    Matrix3X3 outMat;
    outMat[0] = m_Data[0] * rhs[0] + m_Data[1] * rhs[3] + m_Data[2] * rhs[6];
    outMat[1] = m_Data[0] * rhs[1] + m_Data[1] * rhs[4] + m_Data[2] * rhs[7];
    outMat[2] = m_Data[0] * rhs[2] + m_Data[1] * rhs[5] + m_Data[2] * rhs[8];
    outMat[3] = m_Data[3] * rhs[0] + m_Data[4] * rhs[3] + m_Data[5] * rhs[6];
    outMat[4] = m_Data[3] * rhs[1] + m_Data[4] * rhs[4] + m_Data[5] * rhs[7];
    outMat[5] = m_Data[3] * rhs[2] + m_Data[4] * rhs[5] + m_Data[5] * rhs[8];
    outMat[6] = m_Data[6] * rhs[0] + m_Data[7] * rhs[3] + m_Data[8] * rhs[6];
    outMat[7] = m_Data[6] * rhs[1] + m_Data[7] * rhs[4] + m_Data[8] * rhs[7];
    outMat[8] = m_Data[6] * rhs[2] + m_Data[7] * rhs[5] + m_Data[8] * rhs[8];
    return outMat;
  }

  /**
   * @brief Performs the Matrix Multiplication of this and rhs and does it in place.
   * @param rhs
   * @return
   */
  Matrix3X3& multiplyInPlace(SelfType& rhs)
  {
    Matrix3X3 outMat;
    outMat[0] = m_Data[0] * rhs[0] + m_Data[1] * rhs[3] + m_Data[2] * rhs[6];
    outMat[1] = m_Data[0] * rhs[1] + m_Data[1] * rhs[4] + m_Data[2] * rhs[7];
    outMat[2] = m_Data[0] * rhs[2] + m_Data[1] * rhs[5] + m_Data[2] * rhs[8];
    outMat[3] = m_Data[3] * rhs[0] + m_Data[4] * rhs[3] + m_Data[5] * rhs[6];
    outMat[4] = m_Data[3] * rhs[1] + m_Data[4] * rhs[4] + m_Data[5] * rhs[7];
    outMat[5] = m_Data[3] * rhs[2] + m_Data[4] * rhs[5] + m_Data[5] * rhs[8];
    outMat[6] = m_Data[6] * rhs[0] + m_Data[7] * rhs[3] + m_Data[8] * rhs[6];
    outMat[7] = m_Data[6] * rhs[1] + m_Data[7] * rhs[4] + m_Data[8] * rhs[7];
    outMat[8] = m_Data[6] * rhs[2] + m_Data[7] * rhs[5] + m_Data[8] * rhs[8];
    this->m_Data = outMat.m_Data;
    return *this;
  }

  /**
   * @brief Performs the Matrix Addition of g1 and g2 and puts the result into outMat.
   * @param rhs
   * @param outMat
   */
  Matrix3X3 operator+(const Matrix3X3& rhs) const
  {
    Matrix3X3 outMat;
    outMat[0] = m_Data[0] + rhs[0];
    outMat[1] = m_Data[1] + rhs[1];
    outMat[2] = m_Data[2] + rhs[2];
    outMat[3] = m_Data[3] + rhs[3];
    outMat[4] = m_Data[4] + rhs[4];
    outMat[5] = m_Data[5] + rhs[5];
    outMat[6] = m_Data[6] + rhs[6];
    outMat[7] = m_Data[7] + rhs[7];
    outMat[8] = m_Data[8] + rhs[8];
    return outMat;
  }

  /**
   * @brief Performs the Matrix Subtraction of g2 from g1 and puts the result into outMat.
   * @param rhs
   * @param outMat
   */
  Matrix3X3 operator-(const Matrix3X3& rhs) const
  {
    Matrix3X3 outMat;
    outMat[0] = m_Data[0] - rhs[0];
    outMat[1] = m_Data[1] - rhs[1];
    outMat[2] = m_Data[2] - rhs[2];
    outMat[3] = m_Data[3] - rhs[3];
    outMat[4] = m_Data[4] - rhs[4];
    outMat[5] = m_Data[5] - rhs[5];
    outMat[6] = m_Data[6] - rhs[6];
    outMat[7] = m_Data[7] - rhs[7];
    outMat[8] = m_Data[8] - rhs[8];
    return outMat;
  }

  /**
   * @brief Multiplies this 3x3 by a 3x1 matrix
   * @param rhs
   * @return a 3x1 Matrix
   */
  Matrix3X1<T> operator*(const Matrix3X1<T>& rhs) const
  {
    Matrix3X1<T> outMat;
    outMat[0] = m_Data[0] * rhs[0] + m_Data[1] * rhs[1] + m_Data[2] * rhs[2];
    outMat[1] = m_Data[3] * rhs[0] + m_Data[4] * rhs[1] + m_Data[5] * rhs[2];
    outMat[2] = m_Data[6] * rhs[0] + m_Data[7] * rhs[1] + m_Data[8] * rhs[2];
    return outMat;
  }

  /**
   * @brief Multiplies this 3x3 by a 3x1 matrix
   * @param rhs
   * @return a 3x1 Matrix
   */
  std::array<T, 3> operator*(const std::array<T, 3>& rhs)
  {
    std::array<T, 3> outMat;
    outMat[0] = m_Data[0] * rhs[0] + m_Data[1] * rhs[1] + m_Data[2] * rhs[2];
    outMat[1] = m_Data[3] * rhs[0] + m_Data[4] * rhs[1] + m_Data[5] * rhs[2];
    outMat[2] = m_Data[6] * rhs[0] + m_Data[7] * rhs[1] + m_Data[8] * rhs[2];
    return outMat;
  }

  /**
   * @brief Multiplies each element of a 3x1 matrix by a scalar value and returns the result
   * @param scalar to multiply each element by.
   */
  Matrix3X3 operator*(T scalar)
  {
    return {
        m_Data[0] * scalar, m_Data[1] * scalar, m_Data[2] * scalar, m_Data[3] * scalar, m_Data[4] * scalar, m_Data[5] * scalar, m_Data[6] * scalar, m_Data[7] * scalar, m_Data[8] * scalar,
    };
  }

  /**
   * @brief Transposes the 3x3 matrix and places the result into outMat
   * @param g
   * @param outMat
   */

  Matrix3X3 transpose() const
  {
    Matrix3X3 outMat;
    outMat[0] = m_Data[0];
    outMat[1] = m_Data[3];
    outMat[2] = m_Data[6];
    outMat[3] = m_Data[1];
    outMat[4] = m_Data[4];
    outMat[5] = m_Data[7];
    outMat[6] = m_Data[2];
    outMat[7] = m_Data[5];
    outMat[8] = m_Data[8];
    return outMat;
  }

  /**
   * @brief Inverts the 3x3 matrix and returns the result
   * @return outMat
   */
  void invert(T g[3][3], T outMat[3][3])
  {
    SelfType adjoint = this->adjoint();
    T oneOverDeterminant = 1.0 / this->determinant();
    return adjoint * oneOverDeterminant;
  }

  /**
   * @brief Calculates the Adjoint matrix of the 3x3 matrix returns the result
   * @return outMat
   */

  void adjoint()
  {
    SelfType temp = this->cofactor();
    return temp.transpose();
  }

  /**
   * @brief Calculates the cofactor matrix and returns the result
   * @return outMat
   */

  SelfType cofactor() const
  {
    SelfType temp = this->minors3X3();
    SelfType outMat;

    // Row 0
    outMat[0] = temp[0];
    outMat[1] = -temp[1];
    outMat[2] = temp[2];
    // Row 1
    outMat[3] = -temp[3];
    outMat[4] = temp[4];
    outMat[5] = -temp[5];
    // Row 2
    outMat[6] = temp[6];
    outMat[7] = -temp[7];
    outMat[8] = temp[8];
    return outMat;
  }

  /**
   * @brief Calculates the matrix of minors of the 3x3 matrix and places the result into outMat
   * @return outMat
   */

  Matrix3X3 minors()
  {
    Matrix3X3 outMat;
    outMat[0] = m_Data[4] * m_Data[8] - m_Data[7] * m_Data[5];
    outMat[1] = m_Data[3] * m_Data[8] - m_Data[6] * m_Data[5];
    outMat[2] = m_Data[3] * m_Data[7] - m_Data[6] * m_Data[4];
    outMat[3] = m_Data[1] * m_Data[8] - m_Data[7] * m_Data[2];
    outMat[4] = m_Data[0] * m_Data[8] - m_Data[6] * m_Data[2];
    outMat[5] = m_Data[0] * m_Data[7] - m_Data[6] * m_Data[1];
    outMat[6] = m_Data[1] * m_Data[5] - m_Data[4] * m_Data[2];
    outMat[7] = m_Data[0] * m_Data[5] - m_Data[3] * m_Data[2];
    outMat[8] = m_Data[0] * m_Data[4] - m_Data[3] * m_Data[1];
    return outMat;
  }

  /**
   * @brief The determinant of a 3x3 matrix
   * @param g 3x3 Vector
   * @return
   */

  float determinant()
  {
    return (m_Data[0] * (m_Data[4] * m_Data[8] - m_Data[5] * m_Data[7])) - (m_Data[1] * (m_Data[3] * m_Data[8] - m_Data[5] * m_Data[6])) +
           (m_Data[2] * (m_Data[3] * m_Data[7] - m_Data[4] * m_Data[6]));
  }

  /**
   * @brief Initializes the 3x3 matrix to the "Identity" matrix
   * @param g
   */

  Matrix3X3 identity()
  {
    return {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  }

  /**
   * @brief Performs an "in place" normalization of the 3x1 vector.
   * @param g
   */

  Matrix3X3 normalize()
  {
    T denom = m_Data[0] * m_Data[0] + m_Data[3] * m_Data[3] + m_Data[6] * m_Data[6];
    if(denom == 0.0)
    {
      return {};
    }
    Matrix3X3 outMat = this;

    denom = sqrt(denom);
    outMat[0] = outMat[0] / denom;
    if(outMat[0] > 1)
    {
      outMat[0] = 1;
    }
    outMat[3] = outMat[3] / denom;
    if(outMat[3] > 1)
    {
      outMat[3] = 1;
    }
    outMat[6] = outMat[6] / denom;
    if(outMat[6] > 1)
    {
      outMat[6] = 1;
    }
    denom = outMat[1] * outMat[1] + outMat[4] * outMat[4] + outMat[7] * outMat[7];
    if(denom == 0.0)
    {
      return {};
    }
    denom = sqrt(denom);
    outMat[1] = outMat[1] / denom;
    if(outMat[1] > 1)
    {
      outMat[1] = 1;
    }
    outMat[4] = outMat[4] / denom;
    if(outMat[4] > 1)
    {
      outMat[4] = 1;
    }
    outMat[7] = outMat[7] / denom;
    if(outMat[7] > 1)
    {
      outMat[7] = 1;
    }
    denom = outMat[2] * outMat[2] + outMat[5] * outMat[5] + outMat[8] * outMat[8];
    if(denom == 0.0)
    {
      return {};
    }
    denom = sqrt(denom);
    outMat[2] = outMat[2] / denom;
    if(outMat[2] > 1)
    {
      outMat[2] = 1;
    }
    outMat[5] = outMat[5] / denom;
    if(outMat[5] > 1)
    {
      outMat[5] = 1;
    }
    outMat[8] = outMat[8] / denom;
    if(outMat[8] > 1)
    {
      outMat[8] = 1;
    }
    return outMat;
  }

private:
  std::array<T, 9> m_Data = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
};
} // namespace nx::core

#pragma once

#include "complex/Common/Array.hpp"

#include <nonstd/span.hpp>

#include <cmath>
#include <memory>

namespace complex
{
/**
 * @brief 3x1 Matrix as a row.
 * @tparam
 */
template <typename T>
class Matrix3x1

{
public:
  using SelfType = Matrix3x1<T>;

  /**
   * @brief Default constructor will create [0,0,0] matrix.
   */
  Matrix3x1() = default;

  /**
   * @brief Copies from the pointer ptr into the 3x1 Matrix
   * @param ptr
   */
  Matrix3x1(nonstd::span<T> ptr)
  : m_Data(std::array<T, 3>{ptr[0], ptr[1], ptr[2]})
  {
  }

  /**
   * @brief Copies the values into the matrix
   * @param v0
   * @param v1
   * @param v2
   */
  Matrix3x1(T value0, T value1, T value2)
  : m_Data(std::array<T, 3>{value0, value1, value2})
  {
  }

  /**
   * @brief Takes a complex::Array<T, 3> type as an argument
   * @param vec3
   */
  Matrix3x1(const Vec3<T>& vec3)
  : m_Data(std::array<T, 3>{vec3[0], vec3[1], vec3[2]})
  {
  }

  Matrix3x1(const Matrix3x1&) = default;                // Copy Constructor Default Implemented
  Matrix3x1(Matrix3x1&&) noexcept = default;            // Move Constructor Default Implemented
  Matrix3x1& operator=(const Matrix3x1&) = default;     // Copy Assignment Default Implemented
  Matrix3x1& operator=(Matrix3x1&&) noexcept = default; // Move Assignment Default Implemented

  ~Matrix3x1() = default;

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
   * @brief Returns the pointer to the underlying array
   * @return
   */
  T* data()
  {
    return m_Data.data();
  }

  /**
   * @brief Performs the Matrix Addition.
   * @param rhs
   * @return result
   */
  SelfType operator+(const SelfType& rhs)
  {
    return {m_Data[0] + rhs[0], m_Data[1] + rhs[1], m_Data[2] + rhs[2]};
  }

  /**
   * @brief Performs the Matrix Subtraction
   * @param rhs
   * @return outMat result
   */
  SelfType operator-(const SelfType& rhs)
  {
    return {m_Data[0] - rhs[0], m_Data[1] - rhs[1], m_Data[2] - rhs[2]};
  }

  /**
   * @brief Multiplies each element of a 3x1 matrix by the value scalar.
   * @param scalar Value to multiply each element by.
   */
  SelfType operator*(T scalar) const
  {
    return {m_Data[0] * scalar, m_Data[1] * scalar, m_Data[2] * scalar};
  }

  /**
   * @brief Initializes the 3x3 matrix to the "Identity" matrix
   * @return Matrix3x1 (1,0,0);
   */
  Matrix3x1 identity()
  {
    return {1.0f, 0.0f, 0.0f};
  }

  /**
   * @brief returs the normalization of the 3x1 vector.
   * @param g
   */
  SelfType normalize()
  {
    SelfType outMat = this;

    T denom = outMat[0] * outMat[0] + outMat[1] * outMat[1] + outMat[2] * outMat[2];
    denom = sqrt(denom);
    outMat[0] = outMat[0] / denom;
    if(outMat[0] > 1.0)
    {
      outMat[0] = 1.0;
    }
    outMat[1] = outMat[1] / denom;
    if(outMat[1] > 1.0)
    {
      outMat[1] = 1.0;
    }
    outMat[2] = outMat[2] / denom;
    if(outMat[2] > 1.0)
    {
      outMat[2] = 1.0;
    }
    return outMat;
  }

  /**
   * @brief Performs an "in place" normalization of the passed in vector comprising the i, j, k values
   * @param i
   * @param j
   * @param k
   */
  static bool normalize(T& i, T& j, T& k)
  {
    T denom;
    denom = std::sqrt(((i * i) + (j * j) + (k * k)));
    if(denom == 0)
    {
      return false;
    }
    i = i / denom;
    j = j / denom;
    k = k / denom;
  }

  /**
   * @brief Performs an "in place" sort of the 3x1 vector in ascending order.
   * @return new Matrix3x1
   * sorted ascending
   */
  SelfType sortAscending()
  {
    SelfType outMat = this; // copy constructor
    T temp;

    if(outMat[0] <= outMat[1] && outMat[0] <= outMat[2])
    {
      if(outMat[1] <= outMat[2])
      {
        outMat[0] = outMat[0];
        outMat[1] = outMat[1];
        outMat[2] = outMat[2];
      }
      else
      {
        outMat[0] = outMat[0];
        temp = outMat[1];
        outMat[1] = outMat[2];
        outMat[2] = temp;
      }
    }
    else if(outMat[1] <= outMat[0] && outMat[1] <= outMat[2])
    {
      if(outMat[0] <= outMat[2])
      {
        temp = outMat[0];
        outMat[0] = outMat[1];
        outMat[1] = temp;
        outMat[2] = outMat[2];
      }
      else
      {
        temp = outMat[0];
        outMat[0] = outMat[1];
        outMat[1] = outMat[2];
        outMat[2] = temp;
      }
    }
    else if(outMat[2] <= outMat[0] && outMat[2] <= outMat[1])
    {
      if(outMat[0] <= outMat[1])
      {
        temp = outMat[0];
        outMat[0] = outMat[2];
        outMat[2] = outMat[1];
        outMat[1] = temp;
      }
      else
      {
        temp = outMat[0];
        outMat[0] = outMat[2];
        outMat[1] = outMat[1];
        outMat[2] = temp;
      }
    }
  }

  /**
   * @brief Returns index of maximum value.
   */
  size_t maxValueIndex()
  {
    float a = fabs(m_Data[0]);
    float b = fabs(m_Data[1]);
    float c = fabs(m_Data[2]);
    if(a >= b && a >= c)
    {
      return 0;
    }
    if(b >= a && b >= c)
    {
      return 1;
    }
    return 2;
  }

  /**
   * @brief Returns the magnitude of the 3x1 vector
   */
  T magnitude()
  {
    return sqrt(dot(this));
  }

  /**
   * @brief The dot product of 2 vectors a & b
   * @param a 1x3 Vector
   * @param b 1x3 Vector
   * @return
   */
  T dot(const SelfType& b)
  {
    return (m_Data[0] * b[0] + m_Data[1] * b[1] + m_Data[2] * b[2]);
  }

  /**
   * @brief Performs a Cross Product of "this into b" and returns ths result.
   * A X B = C
   * @param b
   * @return
   */

  SelfType cross(const SelfType& b)
  {
    SelfType c;
    c[0] = m_Data[1] * b[2] - m_Data[2] * b[1];
    c[1] = m_Data[2] * b[0] - m_Data[0] * b[2];
    c[2] = m_Data[0] * b[1] - m_Data[1] * b[0];
    return c;
  }

private:
  std::array<T, 3> m_Data = {0.0, 0.0, 0.0};
};
} // namespace complex

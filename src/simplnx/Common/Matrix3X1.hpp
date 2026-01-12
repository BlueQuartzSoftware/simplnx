#pragma once

#include <cmath>
#include <memory>

namespace nx::core
{
/**
 * @brief 3x1 Matrix as a row.
 * @tparam
 */
template <typename T>
class Matrix3X1
{
public:
  using SelfType = Matrix3X1<T>;

  /**
   * @brief Default constructor will create [0,0,0] matrix.
   */
  Matrix3X1() = default;

  /**
   * @brief Copies from the pointer ptr into the 3x1 Matrix
   * @param ptr
   */
  Matrix3X1(T* ptr)
  : m_Data(std::array<T, 3>{ptr[0], ptr[1], ptr[2]})
  {
  }

  /**
   * @brief Copies the values into the matrix
   * @param v0
   * @param v1
   * @param v2
   */
  Matrix3X1(T v0, T v1, T v2)
  : m_Data(std::array<T, 3>{v0, v1, v2})
  {
  }

  Matrix3X1(const Matrix3X1&) = default;                // Copy Constructor Default Implemented
  Matrix3X1(Matrix3X1&&) noexcept = default;            // Move Constructor Default Implemented
  Matrix3X1& operator=(const Matrix3X1&) = default;     // Copy Assignment Default Implemented
  Matrix3X1& operator=(Matrix3X1&&) noexcept = default; // Move Assignment Default Implemented

  ~Matrix3X1() = default;

  template <typename K>
  Matrix3X1<K> to() const
  {
    return Matrix3X1<K>(m_Data[0], m_Data[1], m_Data[2]);
  }

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
  SelfType operator+(const SelfType& rhs) const
  {
    return {m_Data[0] + rhs[0], m_Data[1] + rhs[1], m_Data[2] + rhs[2]};
  }

  /**
   * @brief Performs the Matrix Addition.
   * @param rhs
   * @return result
   */
  SelfType operator+(T scalar) const
  {
    return {m_Data[0] + scalar, m_Data[1] + scalar, m_Data[2] + scalar};
  }

  /**
   * @brief Performs the Matrix Subtraction
   * @param rhs
   * @return outMat result
   */
  SelfType operator-(const SelfType& rhs) const
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
   * @return Matrix3X1 (1,0,0);
   */

  Matrix3X1 identity() const
  {
    return {1.0f, 0.0f, 0.0f};
  }

  T cosTheta(const SelfType& b) const
  {
    T norm1 = this->magnitude();
    T norm2 = b.magnitude();
    if(norm1 == 0 || norm2 == 0)
    {
      return 1.0;
    }
    return this->dot(b) / (norm1 * norm2);
  }

  /**
   * @brief returns a normalization of the 3x1 vector.
   * @param g
   */
  SelfType normalize() const
  {
    SelfType outMat(*this);

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
   * @brief Performs an "in place" normalization of the 3x1 vector
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
   * @return new Matrix3x1 sorted ascending
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
  size_t maxValueIndex() const
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
  T magnitude() const
  {
    return sqrt(dot(*this));
  }

  /**
   * @brief The dot product of 2 vectors a & b
   * @param a 1x3 Vector
   * @param b 1x3 Vector
   * @return
   */
  T dot(const SelfType& b) const
  {
    return (m_Data[0] * b[0] + m_Data[1] * b[1] + m_Data[2] * b[2]);
  }

  T dot() const
  {
    return (m_Data[0] * m_Data[0] + m_Data[1] * m_Data[1] + m_Data[2] * m_Data[2]);
  }

  SelfType abs() const
  {
    return {std::abs(m_Data[0]), std::abs(m_Data[1]), std::abs(m_Data[2])};
  }

  /**
   * @brief Performs a Cross Product of "this into b" and returns ths result.
   * A X B = C
   * @param b
   * @return
   */

  SelfType cross(const SelfType& b) const
  {
    SelfType c;
    c[0] = m_Data[1] * b[2] - m_Data[2] * b[1];
    c[1] = m_Data[2] * b[0] - m_Data[0] * b[2];
    c[2] = m_Data[0] * b[1] - m_Data[1] * b[0];
    return c;
  }

  /**
   * @brief Copies the values into the pointer
   * @param ptr The pointer to the destination
   */
  template <typename K>
  void copyInto(K* ptr) const
  {
    ptr[0] = static_cast<K>(m_Data[0]);
    ptr[1] = static_cast<K>(m_Data[1]);
    ptr[2] = static_cast<K>(m_Data[2]);
  }

private:
  std::array<T, 3> m_Data = {0.0, 0.0, 0.0};
};

using Matrix3X1F = Matrix3X1<float>;
using Matrix3X1D = Matrix3X1<double>;

// 1. Define the first ordering: Vector<T1> * T2 (scalar)
template <typename T1, typename T2>
auto operator*(const Matrix3X1<T1>& v, const T2& scalar)
    // Use std::common_type to determine the resulting type (e.g., int * float -> float)
    -> Matrix3X1<typename std::common_type<T1, T2>::type>
{
  using ResultType = typename std::common_type<T1, T2>::type;
  return {v[0] * static_cast<ResultType>(scalar), v[1] * static_cast<ResultType>(scalar), v[2] * static_cast<ResultType>(scalar)};
}

// 2. Define the second ordering: T1 (scalar) * Vector<T2>
template <typename T1, typename T2>
auto operator*(const T1& scalar, const Matrix3X1<T2>& v)
    // The return type is the same as the first operator*
    -> Matrix3X1<typename std::common_type<T1, T2>::type>
{
  // This implementation can simply call the first operator* to ensure consistency
  return v * scalar;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const Matrix3X1<T>& obj)
{
  os << "<" << obj[0] << ", " << obj[1] << ", " << obj[2] << ">";

  return os;
}

} // namespace nx::core

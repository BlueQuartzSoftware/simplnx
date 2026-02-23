#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Types.hpp"

#include <Eigen/Dense>

#include <cmath>

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
   * @brief Copies the values from the pointer. Assumes row major ordering.
   * @param ptr
   */
  explicit Matrix3X3(const T* ptr)
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

  const T& operator()(size_t row, size_t col) const
  {
    if(row > 2 || col > 2)
    {
      throw std::out_of_range("Matrix3X3::operator() Row or Column out of range");
    }
    return m_Data[row * 3 + col];
  }
  T& operator()(size_t row, size_t col)
  {
    if(row > 2 || col > 2)
    {
      throw std::out_of_range("Matrix3X3::operator() Row or Column out of range");
    }
    return m_Data[row * 3 + col];
  }
  /**
   *
   * @param latticeParameters The lattice Parameters in the order, a, b, c, alpha, beta, gamma. Note that alpha, beta, gamma are all stored as degrees.
   * @return
   */
  static Matrix3X3<T> DirectStructureMatrix(std::array<T, 6> latticeParameters)
  {
    /* This code is take from EMsoftOO/mod_crystallography.f90 - computeMatrices() function */

    T a = latticeParameters[0];
    T b = latticeParameters[1];
    T c = latticeParameters[2];
    T alpha = latticeParameters[3];
    T beta = latticeParameters[4];
    T gamma = latticeParameters[5];

    // auxiliary variables for the various tensors
    double pirad = Constants::k_PiOver180D;
    double ca = std::cos(pirad * alpha);
    double cb = std::cos(pirad * beta);
    double cg = std::cos(pirad * gamma);
    double sg = std::sin(pirad * gamma);

    // cell volume via the determinant of dmt
    T det = (a * b * c) * (a * b * c) * (1.0 - ca * ca - cb * cb - cg * cg + 1.0 * ca * cb * cg);
    T vol = std::sqrt(det);

    Matrix3X3 dsm;
    dsm(0, 0) = a;
    dsm(0, 1) = b * cg;
    dsm(0, 2) = c * cb;
    dsm(1, 0) = 0.0;
    dsm(1, 1) = b * sg;
    dsm(1, 2) = -c * (cb * cg - ca) / sg;
    dsm(2, 0) = 0.0;
    dsm(2, 1) = 0.0;
    dsm(2, 2) = vol / (a * b * sg);
    return dsm;
  }

  /**
   * @brief Converts to an Eigen Row Major Matrix3x3
   * @return
   */
  Eigen::Matrix<T, 3, 3, Eigen::RowMajor> toEigenMatrix() const
  {
    Eigen::Matrix<T, 3, 3, Eigen::RowMajor> g1;
    g1(0, 0) = (*this)[0];
    g1(0, 1) = (*this)[1];
    g1(0, 2) = (*this)[2];
    g1(1, 0) = (*this)[3];
    g1(1, 1) = (*this)[4];
    g1(1, 2) = (*this)[5];
    g1(2, 0) = (*this)[6];
    g1(2, 1) = (*this)[7];
    g1(2, 2) = (*this)[8];
    return g1;
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
   * @brief Returns a pointer to the underlying data
   * @return
   */
  T* data()
  {
    return m_Data.data();
  }

  /**
   * @brief Returns a const pointer to the underlying data
   * @return
   */
  const T* data() const
  {
    return m_Data.data();
  }

  /**
   * @brief Performs the Matrix Multiplication returns the result into outMat.
   * @param rhs
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
  SelfType& multiplyInPlace(SelfType& rhs)
  {
    SelfType outMat;
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
  SelfType operator+(const Matrix3X3& rhs) const
  {
    SelfType outMat;
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
  SelfType operator-(const SelfType& rhs) const
  {
    SelfType outMat;
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
  std::array<T, 3> operator*(const std::array<T, 3>& rhs) const
  {
    std::array<T, 3> outMat;
    outMat[0] = m_Data[0] * rhs[0] + m_Data[1] * rhs[1] + m_Data[2] * rhs[2];
    outMat[1] = m_Data[3] * rhs[0] + m_Data[4] * rhs[1] + m_Data[5] * rhs[2];
    outMat[2] = m_Data[6] * rhs[0] + m_Data[7] * rhs[1] + m_Data[8] * rhs[2];
    return outMat;
  }

  /**
   * @brief Returns a colum vector as a Matrix3X1<T>
   * @param col
   * @return
   */
  Matrix3X1<T> col(size_t col) const
  {
    return {m_Data[col], m_Data[col + 3], m_Data[col + 6]};
  }

  /**
   *  @breif returns a row as a Matrix3x1<T>
   * @param row
   * @return
   */
  Matrix3X1<T> row(size_t row) const
  {
    return {m_Data[row * 3], m_Data[row * 3 + 1], m_Data[row * 3 + 2]};
  }

  /**
   * @brief Multiplies each element of a 3x1 matrix by a scalar value and returns the result
   * @param scalar to multiply each element by.
   */
  SelfType operator*(T scalar) const
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
  SelfType transpose() const
  {
    SelfType outMat;
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
  SelfType invert() const
  {
    SelfType adjoint = this->adjoint();
    T oneOverDeterminant = 1.0 / this->determinant();
    return adjoint * oneOverDeterminant;
  }

  /**
   * @brief Calculates the Adjoint matrix of the 3x3 matrix returns the result
   * @return outMat
   */
  SelfType adjoint() const
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
    SelfType temp = this->minors();
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

  /**b
   * @brief Calculates the matrix of minors of the 3x3 matrix and places the result into outMat
   * @return outMat
   */
  SelfType minors() const
  {
    SelfType outMat;
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
  float determinant() const
  {
    return (m_Data[0] * (m_Data[4] * m_Data[8] - m_Data[5] * m_Data[7])) - (m_Data[1] * (m_Data[3] * m_Data[8] - m_Data[5] * m_Data[6])) +
           (m_Data[2] * (m_Data[3] * m_Data[7] - m_Data[4] * m_Data[6]));
  }

  /**
   * @brief Initializes the 3x3 matrix to the "Identity" matrix
   * @param g
   */
  static SelfType Identity()
  {
    return {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  }

  /**
   * @brief Performs normalization of the 3x3 vector.
   * @param g
   */
  SelfType normalize() const
  {
    T denom = m_Data[0] * m_Data[0] + m_Data[3] * m_Data[3] + m_Data[6] * m_Data[6];
    if(denom == 0.0)
    {
      return {};
    }
    SelfType outMat(*this);

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

  /**
   * @brief Copies the values into the pointer
   * @param ptr The pointer to the destination
   */
  template <typename K>
  void copyInto(K* ptr)
  {
    ptr[0] = static_cast<K>(m_Data[0]);
    ptr[1] = static_cast<K>(m_Data[1]);
    ptr[2] = static_cast<K>(m_Data[2]);
    ptr[3] = static_cast<K>(m_Data[3]);
    ptr[4] = static_cast<K>(m_Data[4]);
    ptr[5] = static_cast<K>(m_Data[5]);
    ptr[6] = static_cast<K>(m_Data[6]);
    ptr[7] = static_cast<K>(m_Data[7]);
    ptr[8] = static_cast<K>(m_Data[8]);
  }

private:
  std::array<T, 9> m_Data = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
};

using Matrix3X3F = Matrix3X3<float>;
using Matrix3X3D = Matrix3X3<double>;

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const Matrix3X3<T>& obj)
{
  os << std::setw(3) << std::setprecision(16) << "| " << obj[0] << ", " << obj[1] << ", " << obj[2] << " |\n";
  os << std::setw(3) << std::setprecision(16) << "| " << obj[3] << ", " << obj[4] << ", " << obj[5] << " |\n";
  os << std::setw(3) << std::setprecision(16) << "| " << obj[6] << ", " << obj[7] << ", " << obj[8] << " |";
  return os;
}

} // namespace nx::core

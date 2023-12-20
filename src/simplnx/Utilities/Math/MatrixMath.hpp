/* ============================================================================
 * Copyright (c) 2009-2016 BlueQuartz Software, LLC
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
 *    United States Air Force Prime Contract FA8650-07-D-5800
 *    United States Air Force Prime Contract FA8650-10-D-5210
 *    United States Prime Contract Navy N00173-07-C-2068
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once

#include "simplnx/simplnx_export.hpp"

#include <cmath>
#include <memory>

namespace nx::core
{
/*
 * @class MatrixMath MatrixMath.h simplnx/Common/MatrixMath.h
 * @brief This class performs
 * @version 5.0
 */
namespace MatrixMath
{

/**
 * @brief Performs the Matrix Multiplication of g1 and g2 and puts the result into outMat. (Single Precision version)
 * @param g1
 * @param g2
 * @param outMat
 */
template <typename T>
void Multiply3x3with3x3(T g1[3][3], T g2[3][3], T outMat[3][3])
{
  outMat[0][0] = g1[0][0] * g2[0][0] + g1[0][1] * g2[1][0] + g1[0][2] * g2[2][0];
  outMat[0][1] = g1[0][0] * g2[0][1] + g1[0][1] * g2[1][1] + g1[0][2] * g2[2][1];
  outMat[0][2] = g1[0][0] * g2[0][2] + g1[0][1] * g2[1][2] + g1[0][2] * g2[2][2];
  outMat[1][0] = g1[1][0] * g2[0][0] + g1[1][1] * g2[1][0] + g1[1][2] * g2[2][0];
  outMat[1][1] = g1[1][0] * g2[0][1] + g1[1][1] * g2[1][1] + g1[1][2] * g2[2][1];
  outMat[1][2] = g1[1][0] * g2[0][2] + g1[1][1] * g2[1][2] + g1[1][2] * g2[2][2];
  outMat[2][0] = g1[2][0] * g2[0][0] + g1[2][1] * g2[1][0] + g1[2][2] * g2[2][0];
  outMat[2][1] = g1[2][0] * g2[0][1] + g1[2][1] * g2[1][1] + g1[2][2] * g2[2][1];
  outMat[2][2] = g1[2][0] * g2[0][2] + g1[2][1] * g2[1][2] + g1[2][2] * g2[2][2];
}

/**
 * @brief Performs the Matrix Multiplication of g1 and g2 and puts the result into outMat. (Single Precision version)
 * @param g1
 * @param g2
 * @param outMat
 */
template <typename T>
void Multiply3x3with3x1(const T g1[3][3], const T* g2, T* outMat)
{
  outMat[0] = g1[0][0] * g2[0] + g1[0][1] * g2[1] + g1[0][2] * g2[2];
  outMat[1] = g1[1][0] * g2[0] + g1[1][1] * g2[1] + g1[1][2] * g2[2];
  outMat[2] = g1[2][0] * g2[0] + g1[2][1] * g2[1] + g1[2][2] * g2[2];
}

void SIMPLNX_EXPORT Multiply3x3with3x1(const double g1[3][3], const double g2[3], float outMat[3]);

/**
 * @brief Performs the Matrix Addition of g1 and g2 and puts the result into outMat. (Single Precision version)
 * @param g1
 * @param g2
 * @param outMat
 */
template <typename T>
void Add3x1s(const T g1[3], const T g2[3], T outMat[3])
{
  outMat[0] = g1[0] + g2[0];
  outMat[1] = g1[1] + g2[1];
  outMat[2] = g1[2] + g2[2];
}

/**
 * @brief Performs the Matrix Subtraction of g2 from g1 and puts the result into outMat. (Single Precision version)
 * @param g1
 * @param g2
 * @param outMat
 */
template <typename T>
void Subtract3x1s(const T g1[3], const T g2[3], T outMat[3])
{
  outMat[0] = g1[0] - g2[0];
  outMat[1] = g1[1] - g2[1];
  outMat[2] = g1[2] - g2[2];
}

/**
 * @brief Performs the Matrix Addition of g1 and g2 and puts the result into outMat. (Single Precision version)
 * @param g1
 * @param g2
 * @param outMat
 */
template <typename T>
void Add3x3s(const T g1[3][3], const T g2[3][3], float outMat[3][3])
{
  outMat[0][0] = g1[0][0] + g2[0][0];
  outMat[0][1] = g1[0][1] + g2[0][1];
  outMat[0][2] = g1[0][2] + g2[0][2];
  outMat[1][0] = g1[1][0] + g2[1][0];
  outMat[1][1] = g1[1][1] + g2[1][1];
  outMat[1][2] = g1[1][2] + g2[1][2];
  outMat[2][0] = g1[2][0] + g2[2][0];
  outMat[2][1] = g1[2][1] + g2[2][1];
  outMat[2][2] = g1[2][2] + g2[2][2];
}

/**
 * @brief Performs the Matrix Subtraction of g2 from g1 and puts the result into outMat. (Single Precision version)
 * @param g1
 * @param g2
 * @param outMat
 */
template <typename T>
void Subtract3x3s(const T g1[3][3], const T g2[3][3], T outMat[3][3])
{
  outMat[0][0] = g1[0][0] - g2[0][0];
  outMat[0][1] = g1[0][1] - g2[0][1];
  outMat[0][2] = g1[0][2] - g2[0][2];
  outMat[1][0] = g1[1][0] - g2[1][0];
  outMat[1][1] = g1[1][1] - g2[1][1];
  outMat[1][2] = g1[1][2] - g2[1][2];
  outMat[2][0] = g1[2][0] - g2[2][0];
  outMat[2][1] = g1[2][1] - g2[2][1];
  outMat[2][2] = g1[2][2] - g2[2][2];
}

/**
 * @brief Multiplies each element of a 3x1 matrix by the value v.
 * @param g Input Matrix
 * @param v Value to mutliply each element by.
 */
template <typename T>
void Multiply3x1withConstant(T g[3], T constant)
{
  g[0] *= constant;
  g[1] *= constant;
  g[2] *= constant;
}

/**
 * @brief Multiplies each element of a 3x1 matrix by the value v.
 * @param g Input Matrix
 * @param v Value to mutliply each element by.
 */
template <typename T>
void Divide4x1withConstant(T g[4], T v)
{
  g[0] = g[0] / v;
  g[1] = g[1] / v;
  g[2] = g[2] / v;
  g[3] = g[3] / v;
}

/**
 * @brief Multiplies each element of a 3x1 matrix by the value v.
 * @param g Input Matrix
 * @param v Value to mutliply each element by.
 */
template <typename T>
void Multiply3x3withConstant(T g[3][3], T constant)
{
  g[0][0] *= constant;
  g[0][1] *= constant;
  g[0][2] *= constant;
  g[1][0] *= constant;
  g[1][1] *= constant;
  g[1][2] *= constant;
  g[2][0] *= constant;
  g[2][1] *= constant;
  g[2][2] *= constant;
}

/**
 * @brief Transposes the 3x3 matrix and places the result into outMat
 * @param g
 * @param outMat
 */
template <typename T>
void Transpose3x3(const T g[3][3], T outMat[3][3])
{
  outMat[0][0] = g[0][0];
  outMat[0][1] = g[1][0];
  outMat[0][2] = g[2][0];
  outMat[1][0] = g[0][1];
  outMat[1][1] = g[1][1];
  outMat[1][2] = g[2][1];
  outMat[2][0] = g[0][2];
  outMat[2][1] = g[1][2];
  outMat[2][2] = g[2][2];
}

/**
 * @brief Inverts the 3x3 matrix and places the result into outMat
 * @param g
 * @param outMat
 */
template <typename T>
void Invert3x3(T g[3][3], T outMat[3][3])
{
  Adjoint3x3(g, outMat);
  T determinant = Determinant3x3(g);
  T oneOverDeterminant = 1.0 / determinant;
  Multiply3x3withConstant(outMat, oneOverDeterminant);
}

/**
 * @brief Calculates the Adjoint matrix of the 3x3 matrix and places the result into outMat
 * @param g
 * @param outMat
 */
template <typename T>
void Adjoint3x3(T g[3][3], T outMat[3][3])
{
  T temp[3][3];
  Cofactor3x3(g, temp);
  Transpose3x3(temp, outMat);
}

/**
 * @brief Calculates the cofactor matrix of the 3x3 matrix and places the result into outMat
 * @param g
 * @param outMat
 */
template <typename T>
void Cofactor3x3(T g[3][3], T outMat[3][3])
{
  T temp[3][3];
  Minors3x3(g, temp);
  // Row 0
  outMat[0][0] = temp[0][0];
  outMat[0][1] = -temp[0][1];
  outMat[0][2] = temp[0][2];
  // Row 1
  outMat[1][0] = -temp[1][0];
  outMat[1][1] = temp[1][1];
  outMat[1][2] = -temp[1][2];
  // Row 2
  outMat[2][0] = temp[2][0];
  outMat[2][1] = -temp[2][1];
  outMat[2][2] = temp[2][2];
}

/**
 * @brief Calculates the matrix of minors of the 3x3 matrix and places the result into outMat
 * @param g
 * @param outMat
 */
template <typename T>
void Minors3x3(const T g[3][3], T outMat[3][3])
{
  outMat[0][0] = g[1][1] * g[2][2] - g[2][1] * g[1][2];
  outMat[0][1] = g[1][0] * g[2][2] - g[2][0] * g[1][2];
  outMat[0][2] = g[1][0] * g[2][1] - g[2][0] * g[1][1];
  outMat[1][0] = g[0][1] * g[2][2] - g[2][1] * g[0][2];
  outMat[1][1] = g[0][0] * g[2][2] - g[2][0] * g[0][2];
  outMat[1][2] = g[0][0] * g[2][1] - g[2][0] * g[0][1];
  outMat[2][0] = g[0][1] * g[1][2] - g[1][1] * g[0][2];
  outMat[2][1] = g[0][0] * g[1][2] - g[1][0] * g[0][2];
  outMat[2][2] = g[0][0] * g[1][1] - g[1][0] * g[0][1];
}

/**
 * @brief The determinant of a 3x3 matrix
 * @param g 3x3 Vector
 * @return
 */
template <typename T>
float Determinant3x3(const T g[3][3])
{
  return (g[0][0] * (g[1][1] * g[2][2] - g[1][2] * g[2][1])) - (g[0][1] * (g[1][0] * g[2][2] - g[1][2] * g[2][0])) + (g[0][2] * (g[1][0] * g[2][1] - g[1][1] * g[2][0]));
}

/**
 * @brief Copies a 3x3 matrix into another 3x3 matrix
 * @param g
 * @param outMat
 */
template <typename T>
void Copy3x3(const T g[3][3], T outMat[3][3])
{
  outMat[0][0] = g[0][0];
  outMat[0][1] = g[0][1];
  outMat[0][2] = g[0][2];
  outMat[1][0] = g[1][0];
  outMat[1][1] = g[1][1];
  outMat[1][2] = g[1][2];
  outMat[2][0] = g[2][0];
  outMat[2][1] = g[2][1];
  outMat[2][2] = g[2][2];
}

/**
 * @brief Copies a 3x1 matrix into another 3x1 matrix
 * @param g
 * @param outMat
 */
template <typename T>
void Copy3x1(const T g[3], T outMat[3])
{
  outMat[0] = g[0];
  outMat[1] = g[1];
  outMat[2] = g[2];
}

/**
 * @brief Initializes the 3x3 matrix to the "Identity" matrix
 * @param g
 */
template <typename T>
void Identity3x3(T g[3][3])
{
  g[0][0] = 1.0f;
  g[0][1] = 0.0f;
  g[0][2] = 0.0f;
  g[1][0] = 0.0f;
  g[1][1] = 1.0f;
  g[1][2] = 0.0f;
  g[2][0] = 0.0f;
  g[2][1] = 0.0f;
  g[2][2] = 1.0f;
}

/**
 * @brief Performs an "in place" normalization of the 3x1 vector.
 * @param g
 */
template <typename T>
void Normalize3x3(T g[3][3])
{
  float denom;
  denom = g[0][0] * g[0][0] + g[1][0] * g[1][0] + g[2][0] * g[2][0];
  if(denom == 0.0)
  {
    return;
  }
  denom = sqrt(denom);
  g[0][0] = g[0][0] / denom;
  if(g[0][0] > 1)
  {
    g[0][0] = 1;
  }
  g[1][0] = g[1][0] / denom;
  if(g[1][0] > 1)
  {
    g[1][0] = 1;
  }
  g[2][0] = g[2][0] / denom;
  if(g[2][0] > 1)
  {
    g[2][0] = 1;
  }
  denom = g[0][1] * g[0][1] + g[1][1] * g[1][1] + g[2][1] * g[2][1];
  if(denom == 0.0)
  {
    return;
  }
  denom = sqrt(denom);
  g[0][1] = g[0][1] / denom;
  if(g[0][1] > 1)
  {
    g[0][1] = 1;
  }
  g[1][1] = g[1][1] / denom;
  if(g[1][1] > 1)
  {
    g[1][1] = 1;
  }
  g[2][1] = g[2][1] / denom;
  if(g[2][1] > 1)
  {
    g[2][1] = 1;
  }
  denom = g[0][2] * g[0][2] + g[1][2] * g[1][2] + g[2][2] * g[2][2];
  if(denom == 0.0)
  {
    return;
  }
  denom = sqrt(denom);
  g[0][2] = g[0][2] / denom;
  if(g[0][2] > 1)
  {
    g[0][2] = 1;
  }
  g[1][2] = g[1][2] / denom;
  if(g[1][2] > 1)
  {
    g[1][2] = 1;
  }
  g[2][2] = g[2][2] / denom;
  if(g[2][2] > 1)
  {
    g[2][2] = 1;
  }
}

/**
 * @brief Performs an "in place" normalization of the 3x1 vector. Single Precision Variant
 * @param g
 */
template <typename T>
void Normalize3x1(T* g)
{
  T denom;
  denom = g[0] * g[0] + g[1] * g[1] + g[2] * g[2];
  denom = sqrt(denom);
  g[0] = g[0] / denom;
  if(g[0] > 1.0)
  {
    g[0] = 1.0;
  }
  g[1] = g[1] / denom;
  if(g[1] > 1.0)
  {
    g[1] = 1.0;
  }
  g[2] = g[2] / denom;
  if(g[2] > 1.0)
  {
    g[2] = 1.0;
  }
}

/**
 * @brief Performs an "in place" sort of the 3x1 vector in ascending order. Single Precision Variant
 * @param g
 */
template <typename T>
void Sort3x1Ascending(T g[3])
{
  float temp;

  if(g[0] <= g[1] && g[0] <= g[2])
  {
    if(g[1] <= g[2])
    {
      g[0] = g[0];
      g[1] = g[1];
      g[2] = g[2];
    }
    else
    {
      g[0] = g[0];
      temp = g[1];
      g[1] = g[2];
      g[2] = temp;
    }
  }
  else if(g[1] <= g[0] && g[1] <= g[2])
  {
    if(g[0] <= g[2])
    {
      temp = g[0];
      g[0] = g[1];
      g[1] = temp;
      g[2] = g[2];
    }
    else
    {
      temp = g[0];
      g[0] = g[1];
      g[1] = g[2];
      g[2] = temp;
    }
  }
  else if(g[2] <= g[0] && g[2] <= g[1])
  {
    if(g[0] <= g[1])
    {
      temp = g[0];
      g[0] = g[2];
      g[2] = g[1];
      g[1] = temp;
    }
    else
    {
      temp = g[0];
      g[0] = g[2];
      g[1] = g[1];
      g[2] = temp;
    }
  }
}

/**
 * @brief Returns index of maximum value. Single Precision Variant
 * @param g
 */
template <typename T>
int FindIndexOfMaxVal3x1(const T g[3])
{
  float a = fabs(g[0]);
  float b = fabs(g[1]);
  float c = fabs(g[2]);
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
 * @brief Performs an "in place" normalization of the 3x1 vector. Double Precision Variant
 * @param i
 * @param j
 * @param k
 */
template <typename T>
void Normalize3x1(T& i, T& j, T& k)
{
  T denom;
  denom = std::sqrt(((i * i) + (j * j) + (k * k)));
  if(denom == 0)
  {
    return;
  }
  i = i / denom;
  j = j / denom;
  k = k / denom;
}

/**
 * @brief Returns the magnitude of the 3x1 vector. Double Precision Variant
 * @param a
 */
template <typename T>
T Magnitude3x1(T a[3])
{
  return (sqrt((a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2])));
}

/**
 * @brief Returns the magnitude of the 4x1 vector. Single Precision Variant
 * @param a
 */
template <typename T>
T Magnitude4x1(T a[4])
{
  return (sqrt((a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]) + (a[3] * a[3])));
}

/**
 * @brief The dot product of 2 vectors a & b. Single Precision Variant
 * @param a 1x3 Vector
 * @param b 1x3 Vector
 * @return
 */
template <typename T>
T DotProduct3x1(T a[3], T b[3])
{
  return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

/**
 * @brief Performs a Cross Product of "a into b" and places the result into c. Double Precision Variant
 * A X B = C
 * @param a
 * @param b
 * @param c
 */
template <typename T>
void CrossProduct(const T a[3], const T b[3], T c[3])
{
  c[0] = a[1] * b[2] - a[2] * b[1];
  c[1] = a[2] * b[0] - a[0] * b[2];
  c[2] = a[0] * b[1] - a[1] * b[0];
}

} // namespace MatrixMath

} // namespace nx::core

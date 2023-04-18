#pragma once

#include "Eigen/Dense"

namespace complex
{
template<template <typename T> class PointT>
struct BoundingBox
{
  Point<T> lowerLeftPoint;
  Point<T> upperRightPoint;
};

/**
 * @brief The ZXZEuler alias is used to describe Euler angles and their format
 * where they are used. There is nothing preventing a user from entering Euler
 * angles in alternative formats, but by describing the specific format, it
 * becomes harder to accidentally use the wrong Euler angle format in code.
 */
using ZXZEuler = Eigen::Vector3f;
} // namespace complex

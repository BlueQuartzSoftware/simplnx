#pragma once

#include "Eigen/Dense"
//#include "complex/Common/Array.hpp"

namespace complex
{
/**
 * @brief The ZXZEuler alias is used to describe Euler angles and their format
 * where they are used. There is nothing preventing a user from entering Euler
 * angles in alternative formats, but by describing the specific format, it
 * becomes harder to accidentally use the wrong Euler angle format in code.
 * @tparam T 
*/
//template <typename T>
//using ZXZEuler = complex::Vec3<T>;

using ZXZEuler = Eigen::Vector3f;
} // namespace complex

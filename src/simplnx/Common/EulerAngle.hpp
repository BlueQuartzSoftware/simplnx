#pragma once

#include "Eigen/Dense"

namespace nx::core
{
/**
 * @brief The ZXZEuler alias is used to describe Euler angles and their format
 * where they are used. There is nothing preventing a user from entering Euler
 * angles in alternative formats, but by describing the specific format, it
 * becomes harder to accidentally use the wrong Euler angle format in code.
 */
using ZXZEuler = Eigen::Vector3f;
} // namespace nx::core

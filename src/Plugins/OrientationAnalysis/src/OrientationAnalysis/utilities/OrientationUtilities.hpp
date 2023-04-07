#pragma once

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/Orientation.hpp"

#include <Eigen/Dense>

namespace complex
{
namespace OrientationUtilities
{
using Matrix3fR = Eigen::Matrix<float, 3, 3, Eigen::RowMajor>;

Matrix3fR OrientationMatrixToGMatrix(const Orientation<float>& oMatrix);

Matrix3fR OrientationMatrixToGMatrixTranspose(const Orientation<float>& oMatrix);

std::string CrystalStructureEnumToString(uint32_t crystalStructureType);
} // namespace OrientationUtilities
} // namespace complex

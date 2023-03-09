#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

#include <Eigen/Dense>

namespace complex
{

struct COMPLEXCORE_EXPORT ComputeMomentInvariants2DInputValues
{
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath FeatureRectArrayPath;
  bool NormalizeMomentInvariants;
  DataPath Omega1ArrayPath;
  DataPath Omega2ArrayPath;
  bool SaveCentralMoments;
  DataPath CentralMomentsArrayPath;
};

/**
 * @class ComputeMomentInvariants2D
 * @brief This filter computes the 2D Omega-1 and Omega 2 values from the Central Moments matrix and optionally will normalize the values to a unit circle and also optionally save the Central Moments
 * matrix as a DataArray to the Cell Feature Attribute Matrix.
 */

class COMPLEXCORE_EXPORT ComputeMomentInvariants2D
{
public:
  using DoubleMatrixType = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
  using IntMatrixType = Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

  ComputeMomentInvariants2D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeMomentInvariants2DInputValues* inputValues);
  ~ComputeMomentInvariants2D() noexcept;

  ComputeMomentInvariants2D(const ComputeMomentInvariants2D&) = delete;
  ComputeMomentInvariants2D(ComputeMomentInvariants2D&&) noexcept = delete;
  ComputeMomentInvariants2D& operator=(const ComputeMomentInvariants2D&) = delete;
  ComputeMomentInvariants2D& operator=(ComputeMomentInvariants2D&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  static DoubleMatrixType ComputeMomentInvariants(const DoubleMatrixType& input, const usize* inputDims, usize maxOrder);
  static int Factorial(int n);
  static DoubleMatrixType Binomial(usize maxOrder);
  static DoubleMatrixType GetBigX(usize maxOrder, usize dim);

private:
  DataStructure& m_DataStructure;
  const ComputeMomentInvariants2DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

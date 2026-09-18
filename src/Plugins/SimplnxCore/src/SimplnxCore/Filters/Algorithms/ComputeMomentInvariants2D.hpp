#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <Eigen/Dense>

namespace nx::core
{
/**
 * @struct ComputeMomentInvariants2DInputValues
 * @brief Stores validated paths and moment-output options.
 */
struct SIMPLNXCORE_EXPORT ComputeMomentInvariants2DInputValues
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
 * @brief Computes 2D Omega moment invariants and optional central moments for each feature.
 *
 * The algorithm retains the direct, parallel in-core path and dispatches to a
 * streaming path for disk-backed arrays. The streaming path reads fixed-size
 * cell chunks but retains per-feature moment and output buffers.
 */

class SIMPLNXCORE_EXPORT ComputeMomentInvariants2D
{
public:
  /**
   * @brief Defines the floating-point matrix used for moment calculations.
   */
  using DoubleMatrixType = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
  /**
   * @brief Defines the integer matrix used for moment-basis indexes.
   */
  using IntMatrixType = Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

  /**
   * @brief Creates a moment-invariant dispatcher.
   * @param dataStructure Provides the selected arrays and geometry.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later feature work when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the dispatcher lifetime.
   */
  ComputeMomentInvariants2D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeMomentInvariants2DInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~ComputeMomentInvariants2D() noexcept;

  ComputeMomentInvariants2D(const ComputeMomentInvariants2D&) = delete;
  ComputeMomentInvariants2D(ComputeMomentInvariants2D&&) noexcept = delete;
  ComputeMomentInvariants2D& operator=(const ComputeMomentInvariants2D&) = delete;
  ComputeMomentInvariants2D& operator=(ComputeMomentInvariants2D&&) noexcept = delete;

  /**
   * @brief Dispatches moment-invariant calculation.
   * @return Error from the selected implementation.
   *
   * Cancellation can retain output values written before the current path stops.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeMomentInvariants2DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core

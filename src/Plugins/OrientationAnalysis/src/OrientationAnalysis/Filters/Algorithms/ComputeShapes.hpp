#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeShapesInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath CentroidsArrayPath;
  DataPath Omega3sArrayPath;
  DataPath AxisLengthsArrayPath;
  DataPath AxisEulerAnglesArrayPath;
  DataPath AspectRatiosArrayPath;
  DataPath VolumesArrayPath;
  DataPath ImageGeometryPath;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeShapes
{
public:
  ComputeShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeShapesInputValues* inputValues);
  ~ComputeShapes() noexcept;

  ComputeShapes(const ComputeShapes&) = delete;
  ComputeShapes(ComputeShapes&&) noexcept = delete;
  ComputeShapes& operator=(const ComputeShapes&) = delete;
  ComputeShapes& operator=(ComputeShapes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeShapesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  double m_ScaleFactor = {1.0};
  std::vector<double> m_FeatureMoments;
  std::vector<double> m_FeatureEigenVals;
  std::vector<float> m_EFVec;

  /**
   * @brief find_moments Determines the second order moments for each Feature
   */
  void find_moments();

  /**
   * @brief find_moments2D Determines the second order moments for each Feature (2D version)
   */
  void find_moments2D();

  /**
   * @brief find_axes Determine principal axis lengths for each Feature
   */
  void find_axes();

  /**
   * @brief find_axes2D Determine principal axis lengths for each Feature (2D version)
   */
  void find_axes2D();

  /**
   * @brief find_axiseulers Determine principal axis directions for each Feature
   */
  void find_axiseulers();

  /**
   * @brief find_axiseulers2D Determine principal axis directions for each Feature (2D version)
   */
  void find_axiseulers2D();
};

} // namespace nx::core

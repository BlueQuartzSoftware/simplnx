#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <vector>

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeShapesInputValues
{
  DataPath feature_ids_array_path;
  DataPath feature_attribute_matrix_path;
  DataPath centroids_array_path;
  DataPath omega3s_array_path;
  DataPath axis_lengths_array_path;
  DataPath axis_euler_angles_array_path;
  DataPath aspect_ratios_array_path;
  DataPath volumes_array_path;
  DataPath image_geometry_path;
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
  ThrottledMessageHandler m_Throttle;

  double m_ScaleFactor = {1.0};
  std::vector<double> m_FeatureMoments;
  std::vector<double> m_FeatureEigenVals;
  std::vector<float> m_EFVec;

  /**
   * @brief Computes the second-order moments for each three-dimensional feature.
   * @return Success, cancellation, or the first store read error.
   *
   * The method returns store errors because the moment scan reads bounded slices.
   */
  Result<> findMoments();

  /**
   * @brief Computes the second-order moments for each two-dimensional feature.
   * @return Success, cancellation, or the first store read error.
   *
   * The method returns store errors because the moment scan reads bounded rows.
   */
  Result<> findMoments2D();

  /**
   * @brief find_axes Determine principal axis lengths for each Feature
   */
  void findAxes();

  /**
   * @brief find_axes2D Determine principal axis lengths for each Feature (2D version)
   */
  void findAxes2D();

  /**
   * @brief find_axiseulers Determine principal axis directions for each Feature
   */
  void findAxisEulers();

  /**
   * @brief find_axiseulers2D Determine principal axis directions for each Feature (2D version)
   */
  void findAxisEulers2D();
};

} // namespace nx::core

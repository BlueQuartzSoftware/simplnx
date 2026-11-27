#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT LaplacianSmoothingInputValues
{
  DataPath p_triangle_geometry_data_path;
  int32 p_iteration_steps{};
  float32 p_lambda{};
  bool p_use_taubin_smoothing{};
  float32 p_mu_factor{};
  float32 p_triple_line_lambda{};
  float32 p_quad_point_lambda{};
  float32 p_surface_point_lambda{};
  float32 p_surface_triple_line_lambda{};
  float32 p_surface_quad_point_lambda{};
  DataPath p_surface_mesh_node_type_array_path;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT LaplacianSmoothing
{
public:
  LaplacianSmoothing(DataStructure& dataStructure, LaplacianSmoothingInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~LaplacianSmoothing() noexcept = default;

  LaplacianSmoothing(const LaplacianSmoothing&) = delete;
  LaplacianSmoothing(LaplacianSmoothing&&) noexcept = delete;
  LaplacianSmoothing& operator=(const LaplacianSmoothing&) = delete;
  LaplacianSmoothing& operator=(LaplacianSmoothing&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const LaplacianSmoothingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  Result<> generateLambdaArray(std::vector<float>& lambdas) const;
  Result<> edgeBasedSmoothing();
};

} // namespace nx::core

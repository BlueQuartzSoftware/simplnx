#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT HierarchicalSmoothInputValues
{
  DataPath triangle_geometry_data_path;
  DataPath node_type_array_path;
  DataPath face_labels_array_path;
  int32 max_iterations{};
  float64 error_threshold{};
};

/**
 * @class HierarchicalSmooth
 * @brief Implements hierarchical smoothing for polycrystalline grain boundary networks.
 *
 * The algorithm preserves quad points (immobile), smooths triple lines as 1D curves,
 * then smooths interior surfaces with fixed boundaries using a Dirichlet boundary value
 * problem solved via conjugate gradient. Based on work by Siddharth Maddali.
 */
class SIMPLNXCORE_EXPORT HierarchicalSmooth
{
public:
  HierarchicalSmooth(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, HierarchicalSmoothInputValues* inputValues);
  ~HierarchicalSmooth() noexcept = default;

  HierarchicalSmooth(const HierarchicalSmooth&) = delete;
  HierarchicalSmooth(HierarchicalSmooth&&) noexcept = delete;
  HierarchicalSmooth& operator=(const HierarchicalSmooth&) = delete;
  HierarchicalSmooth& operator=(HierarchicalSmooth&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const HierarchicalSmoothInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

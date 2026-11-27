#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace pad_image_geometry
{
const std::string k_TempGeometryName = ".cropped_image_geometry";

}

namespace nx::core
{
struct SIMPLNXCORE_EXPORT PadImageGeometryInputValues
{
  DataPath selected_image_geometry_path;
  DataPath created_output_path;
  VectorInt32ParameterType::ValueType x_min_max;
  VectorInt32ParameterType::ValueType y_min_max;
  VectorInt32ParameterType::ValueType z_min_max;
  bool pad_in_x{};
  bool pad_in_y{};
  bool pad_in_z{};
  int32 default_fill_value{};
  bool update_origin{};
  DataPath attribute_matrix_path;
  bool remove_original_geometry{};
};

/**
 * @class PadImageGeometry
 * @brief This algorithm implements support code for the PadImageGeometryFilter
 */

class SIMPLNXCORE_EXPORT PadImageGeometry
{
public:
  PadImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PadImageGeometryInputValues* inputValues);
  ~PadImageGeometry() noexcept = default;

  PadImageGeometry(const PadImageGeometry&) = delete;
  PadImageGeometry(PadImageGeometry&&) noexcept = delete;
  PadImageGeometry& operator=(const PadImageGeometry&) = delete;
  PadImageGeometry& operator=(PadImageGeometry&&) noexcept = delete;

  /**
   * @brief Copies the source cell data into the padded destination geometry.
   * @return An error when the padded dimensions or origin computed from the final geometry cannot be represented.
   * @pre Each enabled axis has two padding values, and both are at least 0.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PadImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

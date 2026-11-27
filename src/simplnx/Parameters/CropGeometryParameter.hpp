#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Filter/ParameterTraits.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core
{
class SIMPLNX_EXPORT CropGeometryParameter : public ValueParameter
{
public:
  struct CropValues
  {
    enum class TypeEnum : uint8
    {
      NoCropping = 0,
      VoxelSubvolume,
      PhysicalSubvolume
    } type = TypeEnum::NoCropping;
    bool is_2d = false;
    bool crop_x = true;
    bool crop_y = true;
    bool crop_z = true;
    SizeVec2Type x_bound_voxels;
    SizeVec2Type y_bound_voxels;
    SizeVec2Type z_bound_voxels;
    FloatVec2Type x_bound_physical;
    FloatVec2Type y_bound_physical;
    FloatVec2Type z_bound_physical;
  };

  using ValueType = CropValues;

  CropGeometryParameter() = delete;
  CropGeometryParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue);
  ~CropGeometryParameter() override = default;

  CropGeometryParameter(const CropGeometryParameter&) = delete;
  CropGeometryParameter(CropGeometryParameter&&) noexcept = delete;

  CropGeometryParameter& operator=(const CropGeometryParameter&) = delete;
  CropGeometryParameter& operator=(CropGeometryParameter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  AcceptedTypesType acceptedTypes() const override;

  /**
   * @brief
   * @return
   */
  UniquePointerType clone() const override;

  /**
   * @brief
   * @return
   */
  std::any defaultValue() const override;

  /**
   * @brief Returns version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the json format changes.
   * @return uint64
   */
  VersionType getVersion() const override;

  /**
   * @brief
   * @return
   */
  ValueType defaultPath() const;

  /**
   * @brief
   * @param value
   * @return
   */
  Result<> validate(const std::any& value) const override;

protected:
  /**
   * @brief
   * @param value
   */
  nlohmann::json toJsonImpl(const std::any& value) const override;

  /**
   * @brief
   * @return
   */
  Result<std::any> fromJsonImpl(const nlohmann::json& json, VersionType version) const override;

private:
  ValueType m_DefaultValue = {};
};
} // namespace nx::core

SIMPLNX_DEF_PARAMETER_TRAITS(nx::core::CropGeometryParameter, "32b03ebf-02a5-40c7-a41c-2380722caeb7");

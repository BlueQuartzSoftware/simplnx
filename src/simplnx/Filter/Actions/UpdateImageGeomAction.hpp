#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Action for updating an ImageGeom's origin and spacing in a DataStructure
 */
class SIMPLNX_EXPORT UpdateImageGeomAction : public IDataAction
{
public:
  UpdateImageGeomAction() = delete;

  /**
   * @brief Constructs an UpdateImageGeomAction.
   * @param origin The new origin (optional)
   * @param spacing The new spacing (optional)
   * @param path The path to the ImageGeometry to update
   * @param centerOrigin Whether to center the origin
   */
  UpdateImageGeomAction(const std::optional<FloatVec3Type>& origin, const std::optional<FloatVec3Type>& spacing, DataPath path, bool centerOrigin = false);

  ~UpdateImageGeomAction() noexcept override;

  UpdateImageGeomAction(const UpdateImageGeomAction&) = delete;
  UpdateImageGeomAction(UpdateImageGeomAction&&) noexcept = delete;
  UpdateImageGeomAction& operator=(const UpdateImageGeomAction&) = delete;
  UpdateImageGeomAction& operator=(UpdateImageGeomAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Validates the spacing, origin, and centered origin before changing the geometry in either mode.
   * On a validation error, the geometry is unchanged.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Success, -6701 if the ImageGeom is missing, -1556 for nonfinite or nonpositive spacing,
   * or -1557 for a nonfinite origin, including the computed result of centering.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return UniquePointer A unique pointer to the cloned action
   */
  UniquePointerType clone() const override;

  /**
   * @brief Returns true if the origin should be updated. Returns false otherwise.
   * @return bool
   */
  bool shouldUpdateOrigin() const;

  /**
   * @brief Returns true if the spacing should be updated. Returns false otherwise.
   * @return bool
   */
  bool shouldUpdateSpacing() const;

  /**
   * @brief Returns the NumericType of the DataArray to be created.
   * @return const std::vector<float64>&
   */
  const std::optional<FloatVec3Type>& origin() const;

  /**
   * @brief Returns the dimensions of the DataArray to be created.
   * @return const std::vector<float64>&
   */
  const std::optional<FloatVec3Type>& spacing() const;

  /**
   * @brief Returns the path of the ImageGeometry to be updated.
   * @return const DataPath& The geometry path
   */
  const DataPath& path() const;

private:
  std::optional<FloatVec3Type> m_Origin;
  std::optional<FloatVec3Type> m_Spacing;
  DataPath m_Path;
  bool m_CenterOrigin;
};
} // namespace nx::core

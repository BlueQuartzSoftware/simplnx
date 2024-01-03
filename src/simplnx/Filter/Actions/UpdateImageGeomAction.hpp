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

  UpdateImageGeomAction(const std::optional<FloatVec3>& origin, const std::optional<FloatVec3>& spacing, const DataPath& path, bool centerOrigin = false);

  ~UpdateImageGeomAction() noexcept override;

  UpdateImageGeomAction(const UpdateImageGeomAction&) = delete;
  UpdateImageGeomAction(UpdateImageGeomAction&&) noexcept = delete;
  UpdateImageGeomAction& operator=(const UpdateImageGeomAction&) = delete;
  UpdateImageGeomAction& operator=(UpdateImageGeomAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override;

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
  const std::optional<FloatVec3>& origin() const;

  /**
   * @brief Returns the dimensions of the DataArray to be created.
   * @return const std::vector<float64>&
   */
  const std::optional<FloatVec3>& spacing() const;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return
   */
  const DataPath& path() const;

private:
  std::optional<FloatVec3> m_Origin;
  std::optional<FloatVec3> m_Spacing;
  DataPath m_Path;
  bool m_CenterOrigin;
};
} // namespace nx::core

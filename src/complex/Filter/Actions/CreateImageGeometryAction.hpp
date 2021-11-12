#pragma once

#include "complex/Common/Array.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for creating an ImageGeometry in a DataStructure
 */
class COMPLEX_EXPORT CreateImageGeometryAction : public IDataAction
{
public:
  CreateImageGeometryAction() = delete;

  CreateImageGeometryAction(DataPath path, SizeVec3 dims, FloatVec3 origin, FloatVec3 spacing);

  ~CreateImageGeometryAction() noexcept override;

  CreateImageGeometryAction(const CreateImageGeometryAction&) = delete;
  CreateImageGeometryAction(CreateImageGeometryAction&&) noexcept = delete;
  CreateImageGeometryAction& operator=(const CreateImageGeometryAction&) = delete;
  CreateImageGeometryAction& operator=(CreateImageGeometryAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the path of the ImageGeometry to be created.
   * @return
   */
  const DataPath& path() const;

  /**
   * @brief Returns the dimensions of the ImageGeometry to be created.
   * @return
   */
  const SizeVec3& dims() const;

  /**
   * @brief Returns the origin of the ImageGeometry to be created.
   * @return
   */
  const FloatVec3& origin() const;

  /**
   * @brief Returns the spacing of the ImageGeometry to be created.
   * @return
   */
  const FloatVec3& spacing() const;

private:
  DataPath m_Path;
  SizeVec3 m_Dims;
  FloatVec3 m_Origin;
  FloatVec3 m_Spacing;
};
} // namespace complex

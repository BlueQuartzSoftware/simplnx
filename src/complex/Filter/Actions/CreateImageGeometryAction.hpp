#pragma once

#include "complex/Common/Array.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for creating an ImageGeometry in a DataStructure
 */
class COMPLEX_EXPORT CreateImageGeometryAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;
  using OriginType = std::vector<float>;
  using SpacingType = std::vector<float>;

  CreateImageGeometryAction() = delete;

  CreateImageGeometryAction(const DataPath& path, const DimensionType& dims, const OriginType& origin, const SpacingType& spacing);

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
   * @return DataPath
   */
  DataPath path() const;

  /**
   * @brief Returns the dimensions of the ImageGeometry to be created.
   * @return
   */
  const DimensionType& dims() const;

  /**
   * @brief Returns the origin of the ImageGeometry to be created.
   * @return
   */
  const OriginType& origin() const;

  /**
   * @brief Returns the spacing of the ImageGeometry to be created.
   * @return
   */
  const SpacingType& spacing() const;

private:
  DimensionType m_Dims;
  OriginType m_Origin;
  SpacingType m_Spacing;
};
} // namespace complex

#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/IGridGeometry.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for creating an RectGridGeometry in a DataStructure
 */
class COMPLEX_EXPORT CreateRectGridGeometryAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;
  using OriginType = std::vector<float>;
  using SpacingType = std::vector<float>;

  CreateRectGridGeometryAction() = delete;

  CreateRectGridGeometryAction(const DataPath& path, usize xBoundsDim, usize yBoundsDim, usize zBoundsDim, const std::string& cellAttributeMatrixName, const std::string& xBoundsName,
                               const std::string& yBoundsName, const std::string& zBoundsName);

  ~CreateRectGridGeometryAction() noexcept override;

  CreateRectGridGeometryAction(const CreateRectGridGeometryAction&) = delete;
  CreateRectGridGeometryAction(CreateRectGridGeometryAction&&) noexcept = delete;
  CreateRectGridGeometryAction& operator=(const CreateRectGridGeometryAction&) = delete;
  CreateRectGridGeometryAction& operator=(CreateRectGridGeometryAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the path of the RectGridGeometry to be created.
   * @return DataPath
   */
  DataPath path() const;

  /**
   * @brief Returns the x bounds dimensions of the RectGridGeometry to be created.
   * @return
   */
  const usize& xDims() const;

  /**
   * @brief Returns the y bounds dimensions of the RectGridGeometry to be created.
   * @return
   */
  const usize& yDims() const;

  /**
   * @brief Returns the z bounds dimensions of the RectGridGeometry to be created.
   * @return
   */
  const usize& zDims() const;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
  usize m_NumXBoundTuples;
  usize m_NumYBoundTuples;
  usize m_NumZBoundTuples;
  std::string m_CellDataName;
  std::string m_XBoundsArrayName;
  std::string m_YBoundsArrayName;
  std::string m_ZBoundsArrayName;
};
} // namespace complex

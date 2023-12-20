#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Action for creating an RectGridGeometry in a DataStructure
 */
class SIMPLNX_EXPORT CreateRectGridGeometryAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;
  using OriginType = std::vector<float>;
  using SpacingType = std::vector<float>;

  /**
   * @brief Constructor to create the geometry and allocate default arrays for the x, y, and z bounds
   * @param path The path to the created geometry
   * @param xBoundsDim The dimension of the xBounds array to be created
   * @param yBoundsDim The dimension of the yBounds array to be created
   * @param zBoundsDim The dimension of the zBounds array to be created
   * @param cellAttributeMatrixName The name of the cell AttributeMatrix to be created
   * @param xBoundsName The name of the xBounds array to be created
   * @param yBoundsName The name of the yBounds array to be created
   * @param zBoundsName The name of the zBounds array to be created
   */
  CreateRectGridGeometryAction(const DataPath& path, usize xBoundsDim, usize yBoundsDim, usize zBoundsDim, const std::string& cellAttributeMatrixName, const std::string& xBoundsName,
                               const std::string& yBoundsName, const std::string& zBoundsName, std::string createdDataFormat = "");

  /**
   * @brief Constructor to create the geometry using existing x, y, and z bounds arrays by either copying, moving, or referencing them
   * @param path The path to the created geometry
   * @param inputXBoundsPath The path to the existing input x bounds array to be copied/moved/referenced
   * @param inputYBoundsPath The path to the existing input y bounds array to be copied/moved/referenced
   * @param inputZBoundsPath The path to the existing input z bounds array to be copied/moved/referenced
   * @param cellAttributeMatrixName The name of the cell AttributeMatrix to be created
   * @param arrayType Tells whether to copy, move, or reference the existing input bounds arrays
   */
  CreateRectGridGeometryAction(const DataPath& path, const DataPath& inputXBoundsPath, const DataPath& inputYBoundsPath, const DataPath& inputZBoundsPath, const std::string& cellAttributeMatrixName,
                               const ArrayHandlingType& arrayType, std::string createdDataFormat = "");

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
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override;

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

protected:
  CreateRectGridGeometryAction() = default;

private:
  usize m_NumXBoundTuples = 2;
  usize m_NumYBoundTuples = 2;
  usize m_NumZBoundTuples = 2;
  std::string m_CellDataName;
  std::string m_XBoundsArrayName;
  std::string m_YBoundsArrayName;
  std::string m_ZBoundsArrayName;
  DataPath m_InputXBounds;
  DataPath m_InputYBounds;
  DataPath m_InputZBounds;
  ArrayHandlingType m_ArrayHandlingType = ArrayHandlingType::Create;
  std::string m_CreatedDataStoreFormat;

  Float32Array* createBoundArray(DataStructure& dataStructure, Mode mode, const std::string& arrayName, usize numTuples, std::vector<Error>& errors) const;
};
} // namespace nx::core

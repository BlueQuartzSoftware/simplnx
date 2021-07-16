#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Montage/AbstractMontage.hpp"
#include "complex/DataStructure/Montage/GridTileIndex.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class GridTileIndex;

/**
 * class GridMontage
 *
 */

class COMPLEX_EXPORT GridMontage : virtual public AbstractMontage
{
public:
  friend class DataStructure;

  using Iterator = void;
  using DimensionsType = SizeVec3;
  using TileIdType = GridTileIndex;

  /**
   * @brief
   * @param other
   */
  GridMontage(const GridMontage& other);

  /**
   * @brief
   * @param other
   */
  GridMontage(GridMontage&& other) noexcept;

  /**
   * Empty Destructor
   */
  virtual ~GridMontage();

  /**
   * @brief Returns a shallow copy of the current DataObject.
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns a deep copy of hte current DataObject.
   * @return DataObject*
   */
  DataObject* deepCopy() override;

  /**
   * @brief Returns the number of rows in the montage.
   * @return size_t
   */
  size_t getRowCount() const;

  /**
   * @brief Returns the number of columns in the montage.
   * @return size_t
   */
  size_t getColumnCount() const;

  /**
   * @brief Returns the depth of the montage in number of items.
   * @return size_t
   */
  size_t getDepth() const;

  /**
   * @brief
   * @return SizeVec3
   */
  SizeVec3 getGridSize() const;

  /**
   * @brief Resizes the montage using the specified values.
   * @param row
   * @param col
   * @param depth
   */
  void resizeTileDims(size_t row, size_t col, size_t depth);

  /**
   * @brief Returns the GridTileIndex for the specified tile position. Returns an invalid
   * GridTileIndex if the tile position is out of bounds.
   * @param row
   * @param col
   * @param depth
   * @return GridTileIndex
   */
  GridTileIndex getTileIndex(size_t row, size_t col, size_t depth) const;

  GridTileIndex getTileIndex(const SizeVec3& pos) const;

  std::optional<SizeVec3> getTilePosOfGeometry(const AbstractGeometry* geom) const;

  /**
   * @brief Returns the tile index for the target geometry. Returns nullptr if the geometry
   * is not part of the montage.
   * @param geom
   * @return std::shared_ptr<AbstractTileIndex>
   */
  std::shared_ptr<AbstractTileIndex> getTileIndex(AbstractGeometry* geom) const override;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return AbstractGeometry*
   */
  AbstractGeometry* getGeometry(const AbstractTileIndex* index) override;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return const AbstractGeometry*
   */
  const AbstractGeometry* getGeometry(const AbstractTileIndex* index) const override;

  /**
   * @brief
   * @param index
   * @param geom
   */
  void setGeometry(const AbstractTileIndex* index, AbstractGeometry* geom) override;

  void setGeometry(const SizeVec3& position, AbstractGeometry* geom);

  /**
   * @brief Returns the geometry at the specified position.
   * Returns nullptr if no geometry could be found.
   * @return const AbstractGeometry*
   */
  AbstractGeometry* getGeometry(const SizeVec3& position);

  /**
   * @brief Returns the geometry at the specified position.
   * Returns nullptr if no geometry could be found.
   * @return const AbstractGeometry*
   */
  const AbstractGeometry* getGeometry(const SizeVec3& position) const;

  /**
   * @brief
   * @return TooltipGenerator
   */
  TooltipGenerator getTooltipGenerator() const override;

  /**
   * @brief
   * @return DimensionsType
   */
  DimensionsType getDimensions() const;

  /**
   * @brief
   * @return BoundsType
   */
  BoundsType getBounds() const;

  /**
   * @brief
   * @param out
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType generateXdmfText(std::ostream& out, const std::string& hdfFileName) const override;

  /**
   * @brief
   * @param in
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType readFromXdmfText(std::istream& in, const std::string& hdfFileName) override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  GridMontage(DataStructure* ds, const std::string& name);

  /**
   * @brief
   * @param tileId
   * @return size_t
   */
  size_t getOffsetFromTileId(const TileIdType& tileId) const;

  size_t getOffsetFromTilePos(const SizeVec3& tilePos) const;

  static size_t getOffsetFromTilePos(const SizeVec3& tilePos, const DimensionsType& dims);

  SizeVec3 getTilePosFromOffset(size_t offset) const;

private:
  size_t m_RowCount;
  size_t m_ColumnCount;
  size_t m_DepthCount;
};
} // namespace complex

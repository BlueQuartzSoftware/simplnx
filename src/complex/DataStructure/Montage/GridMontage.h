#pragma once

#include "SIMPL/DataStructure/Montage/AbstractMontage.h"
#include "SIMPL/DataStructure/Montage/GridTileIndex.h"
#include "SIMPL/Common/SIMPLArray.hpp"

namespace SIMPL
{
class GridTileIndex;

/**
 * class GridMontage
 *
 */

class GridMontage : virtual public AbstractMontage
{
public:
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
   * @brief
   * @return bool
   */
  bool isValid() const override;

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
   * @brief Returns the number of tiles in the montage.
   * @return size_t
   */
  size_t getTileCount() const override;

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
   * @return SIMPL::GridTileIndex
   */
  SIMPL::GridTileIndex getTileIndex(size_t row, size_t col, size_t depth) const;

  /**
   * @brief Returns the tile index for the target geometry. Returns nullptr if the geometry
   * is not part of the montage.
   * @param geom
   * @return std::shared_ptr<AbstractTileIndex>
   */
  std::shared_ptr<AbstractTileIndex> getTileIndex(AbstractGeometry* geom) const override;

  /**
   * @brief Returns a collection of geometries in the montage.
   * @return CollectionType
   */
  CollectionType getGeometries() const override;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return AbstractGeometry*
   */
  AbstractGeometry* getGeometry(AbstractTileIndex* index) const override;

  /**
   * @brief
   * @param index
   * @param geom
   */
  void setGeometry(AbstractTileIndex* index, AbstractGeometry* geom) override;

  /**
   * @brief
   * @return SIMPL::TooltipGenerator
   */
  SIMPL::TooltipGenerator getTooltipGenerator() const override;

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
   * Returns an iterator to the begining of the montage.
   * @return Iterator
   */
  Iterator begin();

  /**
   * @brief Returns an iterator to the end of the montage.
   * @return Iterator
   */
  Iterator end();

  /**
   * @brief
   * Returns a ConstIterator to the begining of the montage.
   * @return ConstIterator
   */
  ConstIterator begin() const;

  /**
   * @brief Returns a ConstIterator to the end of the montage.
   * @return ConstIterator
   */
  ConstIterator end() const;

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
   * @param row
   * @param col
   * @param depth
   */
  GridMontage(DataStructure* ds, const std::string& name, size_t row, size_t col, size_t depth);

  /**
   * @brief
   * @param tileId
   * @return size_t
   */
  size_t getOffsetFromTileId(const TileIdType& tileId) const;

private:
};
} // namespace SIMPL

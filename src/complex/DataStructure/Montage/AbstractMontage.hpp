#pragma once

#include "complex/Common/BoundingBox.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/LinkedPath.hpp"
#include "complex/Utilities/TooltipGenerator.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractTileIndex;
class AbstractGeometry;

/**
 * @class AbstractMontage
 * @brief
 */
class COMPLEX_EXPORT AbstractMontage : public BaseGroup
{
public:
  using CollectionType = std::vector<AbstractGeometry*>;
  using Iterator = CollectionType::iterator;
  using ConstIterator = CollectionType::const_iterator;
  using BoundsType = void;

  /**
   * @brief
   * @param other
   */
  AbstractMontage(const AbstractMontage& other);

  /**
   * @brief
   * @param other
   */
  AbstractMontage(AbstractMontage&& other) noexcept;

  virtual ~AbstractMontage();

  /**
   * @brief Returns the number of tiles in the montage.
   * @return size_t
   */
  size_t getTileCount() const;

  /**
   * @brief Returns a collection of geometries in the Montage.
   * @return CollectionType
   */
  CollectionType getGeometries() const;

  /**
   * @brief Returns a collection of the geometry names in the montage.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getGeometryNames() const;

  /**
   * @brief Returns a collection of LinkedPaths to geometries in the montage.
   * @return std::vector<LinkedPath>
   */
  std::vector<LinkedPath> getLinkedPaths() const;

  /**
   * @brief Returns a TooltipGenerator for the montage.
   * @return TooltipGenerator
   */
  virtual TooltipGenerator getTooltipGenerator() const = 0;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return AbstractGeometry*
   */
  virtual AbstractGeometry* getGeometry(const AbstractTileIndex* index) = 0;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return const AbstractGeometry*
   */
  virtual const AbstractGeometry* getGeometry(const AbstractTileIndex* index) const = 0;

  /**
   * @brief Returns the tile index for the specified geometry. This is a pure virtual method
   * to be implemented in derived classes. Returns nullptr if the geometry is not
   * part of the montage.
   * @param geom
   * @return std::shared_ptr<AbstractTileIndex>
   */
  virtual std::shared_ptr<AbstractTileIndex> getTileIndex(AbstractGeometry* geom) const = 0;

  /**
   * @brief
   * @param index
   * @param geom
   */
  virtual void setGeometry(const AbstractTileIndex* index, AbstractGeometry* geom) = 0;

  /**
   * @brief Returns an iterator to the begining of the montage.
   * @return iterator
   */
  virtual Iterator begin();

  /**
   * @brief Returns an iterator to the end of the montage.
   * @return iterator
   */
  virtual Iterator end();

  /**
   * @brief Returns an iterator to the begining of the montage.
   * @return iterator
   */
  virtual ConstIterator begin() const;

  /**
   * @brief Returns an iterator to the end of the montage.
   * @return iterator
   */
  virtual ConstIterator end() const;

  /**
   * @brief Reads values from HDF5
   * @param targetId
   * @param groupId
   * @return H5::ErrorType
   */
  virtual H5::ErrorType readHdf5(H5::IdType targetId, H5::IdType groupId) = 0;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractMontage(DataStructure& ds, const std::string& name);

  /**
   * @brief Checks if the specified DataObject can be added to the montage. Returns true if
   * the object can be added. Returns false otherwise.

   * This is an override of the
   * method in BaseGroup to ensure that only geometries are added directly to a
   * montage.
   * @param obj
   * @return bool
   */
  bool canInsert(const DataObject* obj) const override;

  /**
   * @brief Returns a reference of the collection for use in derived classes.
   * @return CollectionType&
   */
  CollectionType& getCollection();

  /**
   * @brief Returns a reference of the collection for use in derived classes.
   * @return const CollectionType&
   */
  const CollectionType& getCollection() const;

private:
  CollectionType m_Collection;
};
} // namespace complex

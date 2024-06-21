#pragma once

#include "simplnx/DataStructure/AbstractListStore.hpp"

#include <xtensor/xarray.hpp>

#include <memory>
#include <vector>

namespace nx::core
{
template <typename T>
class ListStore : public AbstractListStore<T>
{
public:
  using parent_type = AbstractListStore<T>;
  using value_type = T;
  using vector_type = typename parent_type::vector_type;
  using shared_vector_type = typename parent_type::shared_vector_type;
  using reference = typename parent_type::reference;
  using const_reference = typename parent_type::const_reference;
  using xarray_type = typename parent_type::xarray_type;
  using iterator = typename parent_type::iterator;
  using const_iterator = typename parent_type::const_iterator;
  using shape_type = typename std::vector<uint64>;

  /**
  * @brief Constructs a ListStore using the specified tuple shape and list size.
  * @param tupleShape
  * @param listSize
   */
  ListStore(shape_type tupleShape, usize listSize = 10)
  : parent_type()
  , m_XtensorListSize(listSize)
  {
    setSize(tupleShape, listSize);
  }

  /**
  * @brief Creates a ListStore from a vector of vectors.
  * @param vectors
  */
  ListStore(const typename std::vector<shared_vector_type>& vectors)
  : parent_type()
  {
    this->setData(vectors);
  }

  /**
  * @brief Copy constructor
  */
  ListStore(const ListStore& other)
  : parent_type(other)
  , m_XtensorListSize(other.m_XtensorListSize)
  {
    parent_type::copy(other);
  }

  /**
  * @brief Move constructor
  */
  ListStore(ListStore&& copy) noexcept
  : parent_type(std::move(copy))
  , m_Array(std::move(copy.m_Array))
  , m_XtensorListSize(std::move(copy.m_XtensorListSize))
  {
  }
  ~ListStore() = default;

  /**
  * @brief Returns a reference to the underlying xtensor array.
  * @return xarray_type&
   */
  xarray_type& xarray() override
  {
    return *m_Array.get();
  }

  /**
  * @brief Returns a const reference to the underlying xtensor array.
  * @return const xarray_type&
  */
  const xarray_type& xarray() const override
  {
    return *m_Array.get();
  }

  /**
   * @brief Returns a copy of the current list store.
   * @return std::unique<AbstractListStore<T>>
   */
  std::unique_ptr<parent_type> deepCopy() const override
  {
    return std::make_unique<ListStore>(*this);
  }

protected:
  /**
   * @brief Resizes the xtensor array for the specified tuple dimensions and internal list size.
   * Copies values from the current array.
   * @param tupleShape
   * @param internalSize
   */
  void resize(std::vector<usize> tupleShape, usize internalSize) override
  {
    std::lock_guard<std::mutex> guard(this->m_Mutex);

    usize numTuples = this->getNumberOfLists();
    usize newTupleCount = std::accumulate(tupleShape.begin(), tupleShape.end(), static_cast<usize>(1), std::multiplies<usize>());
    
    // Avoid size 0
    if(newTupleCount == 0)
    {
      tupleShape = {1};
      newTupleCount = 1;
    }
    if(internalSize == 0)
    {
      internalSize = 1;
    }

    std::vector<usize> newShape = tupleShape;
    newShape.push_back(internalSize);
    xarray_type xarr = xt::zeros<T>(newShape);
    if(m_Array != nullptr)
    {
      for(usize tuple = 0; tuple < numTuples && tuple < newTupleCount; tuple++)
      {
        usize offset = tuple * (internalSize); // New xarray
        usize offset2 = tuple * (m_XtensorListSize); // Current xarray
        // Copy individual list at tuple
        for(usize i = 0; i < m_XtensorListSize && i < internalSize; i++)
        {
          auto value = m_Array->flat(offset2 + i);
          xarr.flat(offset + i) = value;
        }
      }
    }

    this->m_TupleShape = tupleShape;
    m_XtensorListSize = internalSize;
    m_Array = std::make_shared<xarray_type>(std::move(xarr));
  }

  /**
   * @brief Creates a new internal xtensor array using the specified tuple shpae and internal list size.
   * All array values are set to 0.
   * @param tupleShape
   * @param internalSize
   */
  void setSize(std::vector<usize> tupleShape, usize internalSize) override
  {
    usize numTuples = this->getNumberOfLists();
    usize newTupleCount = std::accumulate(tupleShape.begin(), tupleShape.end(), static_cast<usize>(1), std::multiplies<usize>());

    // Avoid size 0
    if (newTupleCount == 0)
    {
      tupleShape = {1};
      newTupleCount = 1;
    }
    if (internalSize == 0)
    {
      internalSize = 1;
    }

    std::vector<usize> newShape = tupleShape;
    newShape.push_back(internalSize);

    std::lock_guard<std::mutex> guard(this->m_Mutex);
    this->m_TupleShape = tupleShape;
    m_XtensorListSize = internalSize;
    xarray_type xarr = xt::zeros<T>(newShape);
    m_Array = std::make_shared<xarray_type>(std::move(xarr));
  }

  /**
   * @brief Returns the list size used for the xtensor list dimension.
   * @return usize
   */
  usize xtensorListSize() const override
  {
    return m_XtensorListSize;
  }

private:
  std::shared_ptr<xarray_type> m_Array = nullptr;
  usize m_XtensorListSize = 10;
};
} // namespace nx::core

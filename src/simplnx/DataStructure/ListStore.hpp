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

  ListStore(shape_type tupleShape, usize listSize = 10)
  : parent_type()
  , m_XtensorListSize(listSize)
  {
    tupleShape.push_back(listSize);
    m_Array = std::make_shared<xarray_type>(xarray_type::from_shape(tupleShape));
    m_Array->fill(0);
  }
  ListStore(const typename std::vector<shared_vector_type>& vectors)
  : parent_type()
  {
    this->setData(vectors);
  }
  ListStore(const ListStore& copy)
  : parent_type(copy)
  , m_Array(copy.m_Array)
  , m_XtensorListSize(copy.m_XtensorListSize)
  {
  }
  ListStore(ListStore&& copy) noexcept
  : parent_type(std::move(copy))
  , m_Array(std::move(copy.m_Array))
  , m_XtensorListSize(std::move(copy.m_XtensorListSize))
  {
  }
  ~ListStore() = default;

  xarray_type& xarray() override
  {
    return *m_Array.get();
  }
  const xarray_type& xarray() const override
  {
    return *m_Array.get();
  }

  std::unique_ptr<parent_type> deepCopy() const override
  {
    return std::make_unique<ListStore>(*this);
  }

  //void resizeTuples(std::vector<usize> tupleShape) override
  //{
  //  resize(tupleShape, m_XtensorListSize);
  //}

protected:
  void resize(std::vector<usize> tupleShape, usize internalSize) override
  {
    std::lock_guard<std::mutex> guard(this->m_Mutex);

    usize numTuples = this->getNumberOfLists();
    usize newTupleCount = std::accumulate(tupleShape.begin(), tupleShape.end(), static_cast<usize>(1), std::multiplies<usize>());

    std::vector<usize> newShape = tupleShape;
    newShape.push_back(internalSize);
    auto xarr = std::make_shared<xarray_type>(xarray_type::from_shape(newShape));

    for(usize tuple = 0; tuple < numTuples && tuple < newTupleCount; tuple++)
    {
      usize offset = tuple * (internalSize);
      usize offset2 = tuple * (m_XtensorListSize);
      for(usize i = 0; i < m_XtensorListSize && i < internalSize; i++)
      {
        auto value = m_Array->flat(offset2 + i);
        xarr->flat(offset + i) = value;
      }
    }
    if(numTuples < newTupleCount)
    {
      // Initialize with zero length Vectors
      for(usize i = numTuples; i < newTupleCount; i++)
      {
        xarr->flat(i * internalSize) = 0;
      }
    }

    this->m_TupleShape = tupleShape;
    m_XtensorListSize = internalSize;
    m_Array = xarr;
  }

  void setSize(std::vector<usize> tupleShape, usize internalSize) override
  {
    usize numTuples = this->getNumberOfLists();
    usize newTupleCount = std::accumulate(tupleShape.begin(), tupleShape.end(), static_cast<usize>(1), std::multiplies<usize>());

    std::vector<usize> newShape = tupleShape;
    newShape.push_back(internalSize);

    this->m_TupleShape = tupleShape;
    m_XtensorListSize = internalSize;
    m_Array = std::make_shared<xarray_type>(xarray_type::from_shape(newShape));
  }

  usize xtensorListSize() const override
  {
    return m_XtensorListSize;
  }

private:
  std::shared_ptr<xarray_type> m_Array;
  usize m_XtensorListSize = 10;
};
} // namespace nx::core

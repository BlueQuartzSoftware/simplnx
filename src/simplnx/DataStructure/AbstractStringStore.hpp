#pragma once

#include "simplnx/Common/Types.hpp"

#include <xtensor/xarray.hpp>

#include <string>
#include <vector>

namespace nx::core
{
class AbstractStringStore
{
public:
  using value_type = std::string;
  using reference = value_type&;
  using const_reference = const value_type&;
  using xarray_type = typename xt::xarray<value_type>;
  using iterator = typename xarray_type::iterator;
  using const_iterator = typename xarray_type::const_iterator;

  ~AbstractStringStore() = default;

  virtual xarray_type& xarray() = 0;
  virtual const xarray_type& xarray() const = 0;

  virtual std::unique_ptr<AbstractStringStore> deepCopy() const = 0;

  usize size() const;
  bool empty() const;
  virtual void resize(usize count) = 0;

  reference operator[](usize index);
  const_reference operator[](usize index) const;
  const_reference at(usize index) const;

  virtual const_reference getValue(usize index) const;
  virtual void setValue(usize index, const value_type& value);

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  virtual AbstractStringStore& operator=(const std::vector<std::string>& values) = 0;

  bool operator==(const std::vector<std::string>& values) const;
  bool operator!=(const std::vector<std::string>& values) const;

protected:
  AbstractStringStore() = default;

private:
};
} // namespace nx::core

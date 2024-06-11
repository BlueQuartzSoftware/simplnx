#pragma once

#include "AbstractStringStore.hpp"

namespace nx::core
{
class StringStore : public AbstractStringStore
{
public:
  StringStore(uint64 count = 0);
  StringStore(const std::vector<std::string>& strings);
  ~StringStore();

  xarray_type& xarray() override;
  const xarray_type& xarray() const override;

  void resize(usize count) override;

  std::unique_ptr<AbstractStringStore> deepCopy() const override;

  AbstractStringStore& operator=(const std::vector<std::string>& values) override;

private:
  xt::xarray<std::string> m_xarray;
};
} // namespace nx::core

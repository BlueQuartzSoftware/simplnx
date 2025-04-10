#pragma once

#include "AbstractStringStore.hpp"

#include <string>
#include <vector>

namespace nx::core
{
class StringStore : public AbstractStringStore
{
public:
  StringStore(uint64 count = 0);
  StringStore(const std::vector<std::string>& strings);
  ~StringStore();

  std::unique_ptr<AbstractStringStore> deepCopy() const override;

  usize size() const override;
  bool empty() const override;
  void resize(usize count) override;

  reference operator[](usize index) override;
  const_reference operator[](usize index) const override;
  const_reference at(usize index) const override;

  const_reference getValue(usize index) const override;
  void setValue(usize index, const value_type& value) override;

  AbstractStringStore& operator=(const std::vector<std::string>& values) override;

private:
  std::vector<std::string> m_Data;
};
} // namespace nx::core

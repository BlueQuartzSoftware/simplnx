#pragma once

#include "complex/DataStructure/IO/Generic/IDataIOManager.hpp"

namespace complex
{
namespace HDF5
{
class COMPLEX_EXPORT DataIOManager : public IDataIOManager
{
public:
  DataIOManager();
  virtual ~DataIOManager() noexcept;

  std::string formatName() const override;

private:
  void addCoreFactories();

  factory_collection m_FactoryCollection;
};
} // namespace HDF5
} // namespace complex

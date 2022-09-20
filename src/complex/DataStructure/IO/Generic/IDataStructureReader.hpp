#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataStructure.hpp"

#include <memory>

namespace complex
{
class IDataFactory;
class IDataFactoryManager;

class COMPLEX_EXPORT IDataStructureReader
{
public:
  virtual ~IDataStructureReader() noexcept;

  virtual Result<DataStructure> readFromGroup(const std::any& group, bool useEmptyDataStores = false) = 0;
  virtual Result<> readObjectFromGroup(const std::any& parentgroup, std::string_view objectName, const std::optional<DataObject::IdType>& parentId = {}, bool useEmptyDataStores = false) = 0;

protected:
  IDataStructureReader(const std::shared_ptr<IDataFactoryManager>& dataFactoryManager);

  std::shared_ptr<IDataFactoryManager> getFactoryManager() const;
  std::shared_ptr<IDataFactory> getDataFactory(std::string_view typeName) const;

private:
  std::shared_ptr<IDataFactoryManager> m_FactoryManager = nullptr;
};
} // namespace complex

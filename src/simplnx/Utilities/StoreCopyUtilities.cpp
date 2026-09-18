#include "StoreCopyUtilities.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/EmptyStringStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/DataStructure/StringStore.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <limits>
#include <stdexcept>

#ifndef SIMPLNX_STORE_COPY_STRICT_FORMAT
#define SIMPLNX_STORE_COPY_STRICT_FORMAT 0
#endif

namespace nx::core
{
namespace
{
constexpr int32 k_NumericFormatValidationError = -10600;
constexpr int32 k_NumericFormatResolutionError = -10601;

/**
 * @brief Computes a shape product without overflowing the host index type.
 * @param shape Dimensions to multiply.
 * @return Checked element count.
 * @throws std::runtime_error If the product overflows.
 */
usize CheckedProduct(const ShapeType& shape)
{
  if(std::find(shape.begin(), shape.end(), 0) != shape.end())
  {
    return 0;
  }
  usize count = 1;
  for(const auto extent : shape)
  {
    if(extent > std::numeric_limits<usize>::max() / count)
    {
      throw std::runtime_error("Store copy shape product overflows usize");
    }
    count *= extent;
  }
  return count;
}

/**
 * @brief Names a manager capability query for one store category.
 */
using StoreCapability = bool (IDataIOManager::*)(const std::string&) const;

/**
 * @brief Selects the first manager that advertises the requested store capability.
 * @param format Receives the effective factory format, including an allowed in-core fallback.
 * @param capability Manager capability query for one store category.
 * @return Selected manager, or null for built-in string storage.
 * @throws std::runtime_error If the build rejects an unavailable format.
 */
std::shared_ptr<IDataIOManager> SelectManager(std::string& format, StoreCapability capability)
{
  const std::string memoryFormat = Preferences::k_InMemoryFormat.str();
  if(format.empty())
  {
    format = memoryFormat;
  }
  auto& collection = Application::GetOrCreateInstance()->getIOCollection();
  for(const auto& [name, manager] : collection)
  {
    if((manager.get()->*capability)(format))
    {
      return manager;
    }
  }
  if(format != memoryFormat)
  {
#if SIMPLNX_STORE_COPY_STRICT_FORMAT
    throw std::runtime_error("Storage format selection has no compatible factory for format '" + format + "'");
#else
    format = memoryFormat;
    return SelectManager(format, capability);
#endif
  }
  if(capability == &IDataIOManager::hasStringStoreCreationFnc)
  {
    return nullptr;
  }
  throw std::runtime_error("Storage format selection has no in-memory factory for format '" + format + "'");
}

/**
 * @struct CopyNumericFunctor
 * @brief Validates numeric stores and transfers values through bounded buffers.
 */
struct CopyNumericFunctor
{
  /**
   * @brief Creates one typed store copy.
   * @tparam T Numeric value type.
   * @param source Untyped source with matching numeric metadata.
   * @param format Requested destination format.
   * @return Owned independent store or metadata placeholder.
   */
  template <typename T>
  std::unique_ptr<IDataStore> operator()(const IDataStore& source, std::string format) const
  {
    const auto* typedSource = dynamic_cast<const AbstractDataStore<T>*>(&source);
    if(typedSource == nullptr)
    {
      throw std::runtime_error("Source numeric store has incompatible type");
    }
    const auto bytes = CalculateStoreCopyBytes(source.getTupleShape(), source.getComponentShape(), sizeof(T));
    if(CheckedProduct(source.getTupleShape()) != source.getNumberOfTuples() || CheckedProduct(source.getComponentShape()) != source.getNumberOfComponents())
    {
      throw std::runtime_error("Source numeric store has inconsistent shape metadata");
    }
    auto manager = SelectManager(format, &IDataIOManager::hasDataStoreCreationFnc);
    if(source.getStoreType() == IDataStore::StoreType::Empty)
    {
      const auto plannedFormat = format == Preferences::k_InMemoryFormat.str() ? std::string{} : format;
      auto result = EmptyDataStore<T>::Create(source.getTupleShape(), source.getComponentShape(), plannedFormat);
      if(result.invalid())
      {
        throw std::runtime_error(result.errors().front().message);
      }
      return std::move(result.value());
    }
    auto destination = manager->dataStoreCreationFnc(format)(source.getDataType(), source.getTupleShape(), source.getComponentShape(), std::nullopt);
    auto* typedDestination = dynamic_cast<AbstractDataStore<T>*>(destination.get());
    if(typedDestination == nullptr || destination->getDataType() != source.getDataType() || destination->getTupleShape() != source.getTupleShape() ||
       destination->getComponentShape() != source.getComponentShape() || destination->getNumberOfTuples() != source.getNumberOfTuples() ||
       destination->getNumberOfComponents() != source.getNumberOfComponents() || destination->getStoreType() == IDataStore::StoreType::Empty)
    {
      throw std::runtime_error("Numeric factory returned an incompatible destination");
    }
    const bool requestsMemory = format == Preferences::k_InMemoryFormat.str();
    const auto actualFormat = destination->getDataFormat();
    const bool actualMemory = actualFormat.empty() || actualFormat == Preferences::k_InMemoryFormat.str();
    if(requestsMemory ? (!actualMemory || destination->getStoreType() != IDataStore::StoreType::InMemory) : (actualFormat != format || destination->getStoreType() != IDataStore::StoreType::OutOfCore))
    {
      throw std::runtime_error("Numeric factory returned the wrong destination storage format");
    }
    if(bytes != 0)
    {
      // This four-argument overload owns the transfer buffer. Backend adapters can also allocate one complete tuple.
      const auto result = typedDestination->copyFrom(0, *typedSource, 0, source.getNumberOfTuples());
      if(result.invalid())
      {
        std::string message = "Numeric transfer failed";
        for(const auto& error : result.errors())
        {
          message += " [" + std::to_string(error.code) + "] " + error.message;
        }
        throw std::runtime_error(message);
      }
    }
    return destination;
  }
};

/**
 * @struct CopyListFunctor
 * @brief Copies one independent list at a time through the selected factory.
 */
struct CopyListFunctor
{
  /**
   * @brief Creates one typed list-store copy.
   * @tparam T List value type.
   * @param source Untyped list source.
   * @param format Requested destination format.
   * @return Owned independent list store or metadata placeholder.
   */
  template <typename T>
  std::unique_ptr<IListStore> operator()(const IListStore& source, std::string format) const
  {
    const auto* typedSource = dynamic_cast<const AbstractListStore<T>*>(&source);
    const auto count = CheckedProduct(source.getTupleShape());
    if(typedSource == nullptr || count != source.getNumberOfTuples())
    {
      throw std::runtime_error("Source list store has incompatible type or shape metadata");
    }
    auto manager = SelectManager(format, &IDataIOManager::hasListStoreCreationFnc);
    if(dynamic_cast<const EmptyListStore<T>*>(typedSource) != nullptr)
    {
      return std::make_unique<EmptyListStore<T>>(source.getTupleShape());
    }
    if(count != 0 && count - 1 > static_cast<usize>(std::numeric_limits<int32>::max()))
    {
      throw std::runtime_error("List tuple index exceeds the int32 store interface");
    }
    auto destination = manager->listStoreCreationFnc(format)(GetDataType<T>(), source.getTupleShape());
    auto* typedDestination = dynamic_cast<AbstractListStore<T>*>(destination.get());
    if(typedDestination == nullptr || destination->getTupleShape() != source.getTupleShape() || destination->getNumberOfTuples() != count ||
       dynamic_cast<EmptyListStore<T>*>(typedDestination) != nullptr)
    {
      throw std::runtime_error("List factory returned an incompatible destination");
    }
    if(destination->isOutOfCore() == (format == Preferences::k_InMemoryFormat.str()))
    {
      throw std::runtime_error("List factory returned the wrong destination storage backend");
    }
    for(usize index = 0; index < count; ++index)
    {
      const auto tupleIndex = static_cast<int32>(index);
      typedDestination->setList(tupleIndex, typedSource->getList(tupleIndex));
    }
    return destination;
  }
};
} // namespace

// -----------------------------------------------------------------------------
Result<std::string> ValidateNumericStorageFormat(const std::string& selectedFormat)
{
  try
  {
    std::string effectiveFormat = selectedFormat;
    SelectManager(effectiveFormat, &IDataIOManager::hasDataStoreCreationFnc);
    if(effectiveFormat == Preferences::k_InMemoryFormat.str())
    {
      effectiveFormat.clear();
    }
    return {std::move(effectiveFormat)};
  } catch(const std::bad_alloc&)
  {
    throw;
  } catch(const std::exception& error)
  {
    return MakeErrorResult<std::string>(k_NumericFormatValidationError,
                                        fmt::format("Cannot select numeric storage format '{}': {}", selectedFormat.empty() ? Preferences::k_InMemoryFormat.view() : selectedFormat, error.what()));
  }
}

// -----------------------------------------------------------------------------
Result<std::string> ResolveNumericStorageFormat(const DataStructure& destination, const DataPath& destinationPath, DataType numericType, uint64 logicalBytes, const std::string& requestedFormat)
{
  std::string selectedFormat = requestedFormat;
  if(selectedFormat.empty())
  {
    try
    {
      selectedFormat = destination.formatResolver().resolveFormat(destination, destinationPath, numericType, logicalBytes);
    } catch(const std::bad_alloc&)
    {
      throw;
    } catch(const std::exception& error)
    {
      return MakeErrorResult<std::string>(k_NumericFormatResolutionError,
                                          fmt::format("Cannot resolve numeric storage for array '{}' with {} logical bytes: {}", destinationPath.toString(), logicalBytes, error.what()));
    }
  }

  auto result = ValidateNumericStorageFormat(selectedFormat);
  if(result.invalid())
  {
    for(auto& error : result.errors())
    {
      error.message = fmt::format("Cannot select numeric storage for array '{}': {}", destinationPath.toString(), error.message);
    }
  }
  return result;
}

// -----------------------------------------------------------------------------
uint64 CalculateStoreCopyBytes(const ShapeType& tupleShape, const ShapeType& componentShape, usize elementSize)
{
  const usize tuples = CheckedProduct(tupleShape);
  const usize components = CheckedProduct(componentShape);
  if(components != 0 && tuples > std::numeric_limits<usize>::max() / components)
  {
    throw std::runtime_error("Store copy value count overflows usize");
  }
  const usize values = tuples * components;
  if(elementSize != 0 && (values > std::numeric_limits<usize>::max() / elementSize || values > std::numeric_limits<uint64>::max() / elementSize))
  {
    throw std::runtime_error("Store copy byte count overflows the supported size");
  }
  return static_cast<uint64>(values) * elementSize;
}

// -----------------------------------------------------------------------------
std::unique_ptr<IDataStore> CopyDataStore(const IDataStore& source, const std::string& destinationFormat)
{
  try
  {
    return ExecuteDataFunction(CopyNumericFunctor{}, source.getDataType(), source, destinationFormat);
  } catch(const std::bad_alloc&)
  {
    throw;
  } catch(const std::exception& error)
  {
    throw std::runtime_error("Numeric store copy to format '" + destinationFormat + "' failed: " + error.what());
  }
}

// -----------------------------------------------------------------------------
std::string ValidateListStoreCopyFormat(const std::string& destinationFormat)
{
  std::string format = destinationFormat;
  SelectManager(format, &IDataIOManager::hasListStoreCreationFnc);
  return format;
}

// -----------------------------------------------------------------------------
std::unique_ptr<IListStore> CopyListStore(const IListStore& source, DataType dataType, const std::string& destinationFormat)
{
  try
  {
    return ExecuteDataFunction(CopyListFunctor{}, dataType, source, destinationFormat);
  } catch(const std::bad_alloc&)
  {
    throw;
  } catch(const std::exception& error)
  {
    throw std::runtime_error("List store copy to format '" + destinationFormat + "' failed: " + error.what());
  }
}

// -----------------------------------------------------------------------------
std::unique_ptr<AbstractStringStore> CopyStringStore(const AbstractStringStore& source, const std::string& destinationFormat)
{
  try
  {
    std::string format = destinationFormat;
    auto manager = SelectManager(format, &IDataIOManager::hasStringStoreCreationFnc);
    const auto count = CheckedProduct(source.getTupleShape());
    if(count != source.getNumberOfTuples())
    {
      throw std::runtime_error("Source string store has inconsistent shape metadata");
    }
    if(source.isPlaceholder())
    {
      return std::make_unique<EmptyStringStore>(source.getTupleShape());
    }
    auto destination = manager == nullptr ? std::make_unique<StringStore>(source.getTupleShape()) : manager->stringStoreCreationFnc(format)(source.getTupleShape());
    if(destination == nullptr || destination->isPlaceholder() || destination->getTupleShape() != source.getTupleShape() || destination->getNumberOfTuples() != count)
    {
      throw std::runtime_error("String factory returned an incompatible destination");
    }
    for(usize index = 0; index < count; ++index)
    {
      destination->setValue(index, source.getValue(index));
    }
    return destination;
  } catch(const std::bad_alloc&)
  {
    throw;
  } catch(const std::exception& error)
  {
    throw std::runtime_error("String store copy to format '" + destinationFormat + "' failed: " + error.what());
  }
}
} // namespace nx::core

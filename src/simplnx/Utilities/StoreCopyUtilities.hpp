#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <memory>
#include <string>

namespace nx::core
{
class IDataStore;
class IListStore;
class AbstractStringStore;
class DataPath;
class DataStructure;

/**
 * @brief Validates a selected numeric storage format without creating a store.
 * @param selectedFormat Selected format. Empty and the canonical in-memory name select memory.
 * @return The normalized format. Memory is an empty string.
 *
 * An unavailable format falls back to memory in an in-core build. An OOC build returns an error.
 */
SIMPLNX_EXPORT Result<std::string> ValidateNumericStorageFormat(const std::string& selectedFormat);

/**
 * @brief Selects and validates the storage format for numeric metadata.
 * @param destination Data structure that will own the array.
 * @param destinationPath Planned array path.
 * @param numericType Array value type.
 * @param logicalBytes Checked logical byte count.
 * @param requestedFormat Explicit format, or empty to use destination policy.
 * @return The normalized selected format. Memory is an empty string.
 *
 * An explicit request does not call the resolver. This function validates capability without creating a store.
 */
SIMPLNX_EXPORT Result<std::string> ResolveNumericStorageFormat(const DataStructure& destination, const DataPath& destinationPath, DataType numericType, uint64 logicalBytes,
                                                               const std::string& requestedFormat);

/**
 * @brief Computes logical bytes with checked shape products.
 * @param tupleShape Tuple dimensions.
 * @param componentShape Component dimensions, or {1} for list estimates.
 * @param elementSize Bytes per value.
 * @return Logical bytes that fit both usize and uint64.
 * @throws std::runtime_error If a dimension product or byte count overflows.
 */
SIMPLNX_EXPORT uint64 CalculateStoreCopyBytes(const ShapeType& tupleShape, const ShapeType& componentShape, usize elementSize);

/**
 * @brief Copies numeric values or placeholder metadata into a selected storage format.
 * @param source Store to copy without modification.
 * @param destinationFormat Resolved destination format; empty selects in-memory storage.
 * @return Independent owned store with the same value type and shapes.
 * @throws std::runtime_error If format selection, factory validation, or transfer fails.
 * @throws std::bad_alloc If an allocation fails.
 *
 * Unknown formats fall back to memory in-core and fail in OOC builds. Failed factories never fall back.
 * Placeholders retain no values. The transfer buffer uses at most 1 MiB; backend buffers can contain a complete tuple.
 * @warning For materialized stores, explicit in-memory storage requires a complete resident destination and can exhaust available RAM.
 * Resolve destination policy or use DataArray::deepCopy for automatic selection.
 */
SIMPLNX_EXPORT std::unique_ptr<IDataStore> CopyDataStore(const IDataStore& source, const std::string& destinationFormat);

/**
 * @brief Validates list-format capability without creating a destination store.
 * @param destinationFormat Requested destination format.
 * @return Effective format, including normalization and an allowed in-core fallback.
 * @throws std::runtime_error If the build rejects the unavailable format.
 */
SIMPLNX_EXPORT std::string ValidateListStoreCopyFormat(const std::string& destinationFormat);

/**
 * @brief Copies list values or placeholder metadata into a selected storage format.
 * @param source Store to copy without modification.
 * @param dataType Numeric type of each list value.
 * @param destinationFormat Resolved destination format; empty selects in-memory storage.
 * @return Independent owned list store with the same tuple shape.
 * @throws std::runtime_error If the format, type, tuple range, factory, or transfer is invalid.
 * @throws std::bad_alloc If an allocation fails.
 *
 * Unknown formats fall back to memory in-core and fail in OOC builds. Failed factories never fall back.
 * Generic transfer holds one list plus backend buffers. Placeholders retain no values.
 * @warning For materialized stores, explicit in-memory storage requires all destination lists in RAM and can exhaust available RAM.
 * Resolve destination policy or use NeighborList::deepCopy for automatic selection.
 */
SIMPLNX_EXPORT std::unique_ptr<IListStore> CopyListStore(const IListStore& source, DataType dataType, const std::string& destinationFormat);

/**
 * @brief Copies strings or placeholder metadata into a selected storage format.
 * @param source Store to copy without modification.
 * @param destinationFormat Resolved destination format; empty selects in-memory storage.
 * @return Independent owned string store with the same tuple shape.
 * @throws std::runtime_error If format selection, factory validation, or transfer fails.
 * @throws std::bad_alloc If an allocation fails.
 *
 * Unknown formats fall back to memory in-core and fail in OOC builds. Failed factories never fall back.
 * Placeholders retain no values. Generic transfer holds one string plus backend buffers.
 * @warning For materialized stores, explicit in-memory storage requires all destination strings in RAM and can exhaust available RAM.
 * Direct callers must resolve destination policy.
 */
SIMPLNX_EXPORT std::unique_ptr<AbstractStringStore> CopyStringStore(const AbstractStringStore& source, const std::string& destinationFormat);
} // namespace nx::core

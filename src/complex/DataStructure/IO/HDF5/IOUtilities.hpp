#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class BaseGroup;
class DataMap;
class DataObject;

namespace H5
{
class GroupWriter;
class GroupReader;
class ObjectWriter;
} // namespace H5

namespace HDF5
{
class DataStructureReader;
class DataStructureWriter;

/**
 * @brief Attempts to write the DataObject attributes to HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureWriter
 * @param objectWriter
 * @param dataObject
 * @param importable
 * @return Result<>
 */
Result<> COMPLEX_EXPORT WriteObjectAttributes(DataStructureWriter& dataStructureWriter, H5::ObjectWriter& objectWriter, const DataObject* dataObject, bool importable);

/**
 * @brief Attempts to read the DataMap from HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureReader
 * @param dataMap
 * @param parentGroup
 * @param parentId
 * @param useEmptyDataStore = false
 * @return Result<>
 */
Result<> COMPLEX_EXPORT ReadDataMap(DataStructureReader& dataStructureReader, DataMap& dataMap, const H5::GroupReader& parentGroup, DataObject::IdType parentId, bool useEmptyDataStore = false);

/**
 * @brief Attempts to read the BaseGroup from HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureReader
 * @param groupReader
 * @param baseGroup
 * @param useEmptyDataStore = false
 * @return Result<>
 */
Result<> COMPLEX_EXPORT ReadBaseGroup(DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, BaseGroup* baseGroup, bool useEmptyDataStore = false);

/**
 * @brief Attempts to write the BaseGroup to HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureWriter
 * @param groupWriter
 * @param dataMap
 * @param importable = true
 * @return Result<>
 */
Result<> COMPLEX_EXPORT WriteBaseGroup(DataStructureWriter& dataStructureWriter, H5::GroupWriter& groupWriter, const BaseGroup* dataMap, bool importable = true);

/**
 * @brief Attempts to write the DataMap to HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureWriter
 * @param groupWriter
 * @param dataMap
 * @return Result<>
 */
Result<> COMPLEX_EXPORT WriteDataMap(DataStructureWriter& dataStructureWriter, H5::GroupWriter& groupWriter, const DataMap& dataMap);
} // namespace HDF5
} // namespace complex

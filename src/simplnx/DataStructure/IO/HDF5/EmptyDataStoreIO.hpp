#pragma once

#include "simplnx/DataStructure/IO/HDF5/IDataStoreIO.hpp"

#include <iterator>
#include <utility>

namespace nx::core::HDF5
{
namespace EmptyDataStoreIO
{
/**
 * @brief Reads numeric tuple and component shapes without creating a store.
 * @param datasetReader Source HDF5 dataset.
 * @return Exact stored shapes or the first attribute-read error.
 */
inline Result<std::pair<ShapeType, ShapeType>> ReadShapes(const nx::core::HDF5::DatasetIO& datasetReader)
{
  auto tupleShapeResult = IDataStoreIO::ReadTupleShape(datasetReader);
  if(tupleShapeResult.invalid())
  {
    return ConvertInvalidResult<std::pair<ShapeType, ShapeType>>(std::move(tupleShapeResult));
  }
  auto warnings = std::move(tupleShapeResult.warnings());
  auto componentShapeResult = IDataStoreIO::ReadComponentShape(datasetReader);
  if(componentShapeResult.invalid())
  {
    auto result = ConvertInvalidResult<std::pair<ShapeType, ShapeType>>(std::move(componentShapeResult));
    result.warnings().insert(result.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
    return result;
  }

  Result<std::pair<ShapeType, ShapeType>> result{std::pair<ShapeType, ShapeType>{std::move(tupleShapeResult.value()), std::move(componentShapeResult.value())}};
  result.warnings() = std::move(warnings);
  for(auto&& warning : componentShapeResult.warnings())
  {
    result.warnings().push_back(std::move(warning));
  }
  return result;
}
} // namespace EmptyDataStoreIO
} // namespace nx::core::HDF5

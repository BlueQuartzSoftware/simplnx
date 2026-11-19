#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ReadHDF5DataStackParameter.hpp"
#include "simplnx/Parameters/ReadHDF5FileListParameter.hpp"

#include <string>
#include <vector>

namespace nx::core
{
namespace HDF5
{
class DatasetIO;
}

/**
 * @brief A single resolved (file, dataset) pair to read as one slice of the
 * stack. Which HDF5 file/dataset combination corresponds to slice i depends on
 * the filter's input mode: for a single file with multiple datasets, filePath
 * is the same for every slice and datasetPath varies; for multiple files
 * sharing one dataset path, filePath varies and datasetPath is the same for
 * every slice.
 */
struct Hdf5StackSliceSource
{
  std::string filePath;
  std::string datasetPath;
};

/**
 * @brief Resolves the ordered list of slice sources for the "single file,
 * multiple datasets" input mode: one HDF5 file, with dataset paths generated
 * from pathPrefix + zero-padded index + pathSuffix over [startIndex, endIndex].
 */
SIMPLNXCORE_EXPORT std::vector<Hdf5StackSliceSource> GenerateSliceSourcesFromDataStack(const ReadHDF5DataStackParameter::ValueType& dataStackInfo);

/**
 * @brief Resolves the ordered list of slice sources for the "multiple files,
 * same dataset" input mode: a numbered stack of HDF5 files (per
 * ReadHDF5FileListParameter's own file-list generation), each read at the same
 * fixed datasetPath.
 */
SIMPLNXCORE_EXPORT std::vector<Hdf5StackSliceSource> GenerateSliceSourcesFromFileList(const ReadHDF5FileListParameter::ValueType& fileListInfo);

struct SIMPLNXCORE_EXPORT Hdf5StackReaderInputValues
{
  std::vector<Hdf5StackSliceSource> sliceSources;
  std::string montageName;
  std::string geometryName;
  std::string matrixName;
  std::string dataArrayName;
  NumericType numericType = NumericType::int32;
  bool useMontage = false;
};

/**
 * @brief The 2D tuple shape (dims[0], dims[1]) and trailing component shape
 * (dims[2..]) of a single dataset in the stack. Per-filter convention: the
 * first two HDF5 dataset dimensions are treated as the 2D tuple dimensions;
 * any remaining dimensions are the component shape.
 */
struct Hdf5StackSliceShape
{
  usize dimY = 0;
  usize dimX = 0;
  std::vector<usize> componentDims;

  bool operator==(const Hdf5StackSliceShape& rhs) const
  {
    return dimY == rhs.dimY && dimX == rhs.dimX && componentDims == rhs.componentDims;
  }
  bool operator!=(const Hdf5StackSliceShape& rhs) const
  {
    return !(*this == rhs);
  }
};

/**
 * @brief Reads the shape of a single slice dataset and splits it into tuple
 * dims (first 2 entries) and component dims (remaining entries, defaulting
 * to a single scalar component when the dataset is exactly 2D).
 * Returns an error if the dataset has fewer than 2 dimensions.
 */
SIMPLNXCORE_EXPORT Result<Hdf5StackSliceShape> ReadHdf5StackSliceShape(const HDF5::DatasetIO& datasetReader, const std::string& datasetPath);

/**
 * @class Hdf5StackReader
 * @brief Reads a numbered stack of 2D HDF5 datasets into either a single
 * Z-stacked ImageGeom or a GridMontage of single-slice ImageGeoms.
 */
class SIMPLNXCORE_EXPORT Hdf5StackReader
{
public:
  Hdf5StackReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Hdf5StackReaderInputValues* inputValues);
  ~Hdf5StackReader() noexcept;

  Hdf5StackReader(const Hdf5StackReader&) = delete;
  Hdf5StackReader(Hdf5StackReader&&) noexcept = delete;
  Hdf5StackReader& operator=(const Hdf5StackReader&) = delete;
  Hdf5StackReader& operator=(Hdf5StackReader&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const Hdf5StackReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

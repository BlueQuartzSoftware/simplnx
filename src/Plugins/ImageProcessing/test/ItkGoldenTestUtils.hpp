#pragma once

#include <catch2/catch.hpp>

#include "ImageProcessing/ImageProcessing_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/MD5.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "ImageProcessing/Filters/ReadImageFilter.hpp"
#include "ImageProcessing/Filters/ReadMhaFileFilter.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace ip_golden
{
using namespace nx::core;

//------------------------------------------------------------------------------
inline fs::path InputPath(std::string_view fileName)
{
  return fs::path(std::string(nx::core::unit_test::k_ItkInputDir.view())) / fileName;
}
inline fs::path BaselinePath(std::string_view fileName)
{
  return fs::path(std::string(nx::core::unit_test::k_ItkBaselineDir.view())) / fileName;
}

//------------------------------------------------------------------------------
// Lower-cased file extension without the leading dot (e.g. "nrrd").
inline std::string LowerExtension(const fs::path& file)
{
  std::string ext = file.extension().string();
  if(!ext.empty() && ext.front() == '.')
  {
    ext.erase(ext.begin());
  }
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

//------------------------------------------------------------------------------
// Reads @p file into a fresh ImageGeom @p geomPath / cell AM @p cellAmName / scalar array @p arrayName, routing by
// extension to the ITK-free reader that owns that format. Returns the reader's execute Result. NOTE: reading through
// OUR readers under ForceInCore is safe (unlike the legacy ITK reader, which bad_casts an OOC store).
inline Result<> ReadInputImage(DataStructure& dataStructure, const fs::path& file, const DataPath& geomPath, const std::string& cellAmName, const std::string& arrayName)
{
  const std::string ext = LowerExtension(file);

  if(ext == "nrrd" || ext == "nhdr")
  {
    // ReadImageFilter is dimension-aware and reads .nrrd/.nhdr via its NRRD backend (the retired
    // standalone NRRD reader folded into it). Its NRRD path builds identical geometry + voxel data.
    ReadImageFilter filter;
    Arguments args;
    args.insertOrAssign(ReadImageFilter::k_FileName_Key, std::make_any<FileSystemPathParameter::ValueType>(file));
    args.insertOrAssign(ReadImageFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(CropGeometryParameter::ValueType{}));
    args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, std::make_any<std::string>(cellAmName));
    args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, std::make_any<std::string>(arrayName));
    auto preflight = filter.preflight(dataStructure, args);
    if(preflight.outputActions.invalid())
    {
      return ConvertResult(std::move(preflight.outputActions));
    }
    return filter.execute(dataStructure, args).result;
  }

  if(ext == "mha" || ext == "mhd")
  {
    ReadMhaFileFilter filter;
    Arguments args;
    args.insertOrAssign(ReadMhaFileFilter::k_InputFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(file));
    args.insertOrAssign(ReadMhaFileFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(CropGeometryParameter::ValueType{}));
    args.insertOrAssign(ReadMhaFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(ReadMhaFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(cellAmName));
    args.insertOrAssign(ReadMhaFileFilter::k_ImageDataArrayName_Key, std::make_any<std::string>(arrayName));
    args.insertOrAssign(ReadMhaFileFilter::k_ApplyImageTransformation_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadMhaFileFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
    args.insertOrAssign(ReadMhaFileFilter::k_TransposeTransformMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadMhaFileFilter::k_SaveImageTransformation_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadMhaFileFilter::k_TransformationMatrixPath_Key, std::make_any<DataPath>(geomPath.createChildPath("TransformationMatrix")));
    auto preflight = filter.preflight(dataStructure, args);
    if(preflight.outputActions.invalid())
    {
      return ConvertResult(std::move(preflight.outputActions));
    }
    return filter.execute(dataStructure, args).result;
  }

  if(ext == "png" || ext == "tif" || ext == "tiff" || ext == "jpg" || ext == "jpeg" || ext == "bmp")
  {
    ReadImageFilter filter;
    Arguments args;
    args.insertOrAssign(ReadImageFilter::k_FileName_Key, std::make_any<FileSystemPathParameter::ValueType>(file));
    args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, std::make_any<std::string>(cellAmName));
    args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, std::make_any<std::string>(arrayName));
    args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadImageFilter::k_ChangeDataType_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadImageFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(CropGeometryParameter::ValueType{}));
    auto preflight = filter.preflight(dataStructure, args);
    if(preflight.outputActions.invalid())
    {
      return ConvertResult(std::move(preflight.outputActions));
    }
    return filter.execute(dataStructure, args).result;
  }

  return MakeErrorResult(-9001, fmt::format("ip_golden::ReadInputImage: unsupported extension '{}' for file '{}'", ext, file.string()));
}

//------------------------------------------------------------------------------
namespace detail
{
template <class T>
Result<> CompareScalarTyped(const IDataArray& outputArray, const IDataArray& baselineArray, float64 tolerance)
{
  const auto& out = dynamic_cast<const DataArray<T>&>(outputArray);
  const auto& base = dynamic_cast<const DataArray<T>&>(baselineArray);
  float64 largestError = 0.0;
  const usize n = out.getSize();
  for(usize i = 0; i < n; ++i)
  {
    const float64 diff = std::abs(static_cast<float64>(out[i]) - static_cast<float64>(base[i]));
    largestError = std::max(diff, largestError);
  }
  if(largestError > tolerance)
  {
    return MakeErrorResult(-20, fmt::format("Comparing output and baseline produced too large an error. Tolerance {}, error {}", tolerance, largestError));
  }
  return {};
}
} // namespace detail

// Scalar-only mirror of ITKTestBase::CompareImages (same checks + error codes). @p outputGeom owns @p outputData.
inline Result<> CompareImages(DataStructure& dataStructure, const DataPath& baselineGeom, const DataPath& baselineData, const DataPath& outputGeom, const DataPath& outputData, float64 tolerance)
{
  const auto* baselineImageGeom = dataStructure.getDataAs<ImageGeom>(baselineGeom);
  if(baselineImageGeom == nullptr)
  {
    return MakeErrorResult(-10, "Could not get ImageGeometry for Baseline");
  }
  const auto* baselineDataArray = dataStructure.getDataAs<IDataArray>(baselineData);
  if(baselineDataArray == nullptr)
  {
    return MakeErrorResult(-11, "Could not get DataArray for Baseline");
  }
  const auto* outputImageGeom = dataStructure.getDataAs<ImageGeom>(outputGeom);
  if(outputImageGeom == nullptr)
  {
    return MakeErrorResult(-12, "Could not get ImageGeometry for Output");
  }
  const auto* outputDataArray = dataStructure.getDataAs<IDataArray>(outputData);
  if(outputDataArray == nullptr)
  {
    return MakeErrorResult(-13, "Could not get DataArray for Output");
  }
  const DataType baselineType = baselineDataArray->getDataType();
  const DataType outputType = outputDataArray->getDataType();
  if(baselineType != outputType)
  {
    return MakeErrorResult(-14, fmt::format("DataTypes do not match. Output: {} Baseline: {}", fmt::underlying(outputType), fmt::underlying(baselineType)));
  }
  if(baselineImageGeom->getDimensions() != outputImageGeom->getDimensions())
  {
    return MakeErrorResult(-15, fmt::format("Image Dimensions do not match. Output: {} Baseline: {}", StringUtilities::formatDimensions3D(outputImageGeom->getDimensions()),
                                            StringUtilities::formatDimensions3D(baselineImageGeom->getDimensions())));
  }
  if(baselineDataArray->getIDataStoreRef().getTupleShape() != outputDataArray->getIDataStoreRef().getTupleShape())
  {
    return MakeErrorResult(-16, "Tuple Shape does not match between Output and Baseline");
  }
  if(baselineDataArray->getIDataStoreRef().getComponentShape() != outputDataArray->getIDataStoreRef().getComponentShape())
  {
    return MakeErrorResult(-18, "Component Shape does not match between Output and Baseline");
  }

  switch(outputType)
  {
  case DataType::float32:
    return detail::CompareScalarTyped<float32>(*outputDataArray, *baselineDataArray, tolerance);
  case DataType::float64:
    return detail::CompareScalarTyped<float64>(*outputDataArray, *baselineDataArray, tolerance);
  case DataType::int8:
    return detail::CompareScalarTyped<int8>(*outputDataArray, *baselineDataArray, tolerance);
  case DataType::uint8:
    return detail::CompareScalarTyped<uint8>(*outputDataArray, *baselineDataArray, tolerance);
  case DataType::int16:
    return detail::CompareScalarTyped<int16>(*outputDataArray, *baselineDataArray, tolerance);
  case DataType::uint16:
    return detail::CompareScalarTyped<uint16>(*outputDataArray, *baselineDataArray, tolerance);
  case DataType::int32:
    return detail::CompareScalarTyped<int32>(*outputDataArray, *baselineDataArray, tolerance);
  case DataType::uint32:
    return detail::CompareScalarTyped<uint32>(*outputDataArray, *baselineDataArray, tolerance);
  case DataType::int64:
    return detail::CompareScalarTyped<int64>(*outputDataArray, *baselineDataArray, tolerance);
  case DataType::uint64:
    return detail::CompareScalarTyped<uint64>(*outputDataArray, *baselineDataArray, tolerance);
  default:
    return MakeErrorResult(-100, "Unsupported data type for image comparison (boolean not supported).");
  }
}

//------------------------------------------------------------------------------
namespace detail
{
template <class T>
std::string Md5Typed(const IDataArray& array)
{
  const auto& typed = dynamic_cast<const DataArray<T>&>(array);
  const usize n = typed.getSize();
  MD5 md5;
  if(typed.getIDataStoreRef().getStoreType() != IDataStore::StoreType::OutOfCore)
  {
    const T* ptr = typed.template getIDataStoreRefAs<DataStore<T>>().data();
    md5.update(reinterpret_cast<const uint8*>(ptr), n * sizeof(T));
  }
  else
  {
    std::vector<T> buf(n);
    for(usize i = 0; i < n; ++i)
    {
      buf[i] = typed[i];
    }
    md5.update(reinterpret_cast<const uint8*>(buf.data()), n * sizeof(T));
  }
  md5.finalize();
  return md5.hexdigest();
}
} // namespace detail

// Mirror of ITKTestBase::ComputeMd5Hash. In-core hashes raw bytes; OOC materializes then hashes. boolean unsupported.
inline std::string ComputeMd5Hash(DataStructure& dataStructure, const DataPath& outputData)
{
  const auto& array = dataStructure.getDataRefAs<IDataArray>(outputData);
  switch(array.getDataType())
  {
  case DataType::float32:
    return detail::Md5Typed<float32>(array);
  case DataType::float64:
    return detail::Md5Typed<float64>(array);
  case DataType::int8:
    return detail::Md5Typed<int8>(array);
  case DataType::uint8:
    return detail::Md5Typed<uint8>(array);
  case DataType::int16:
    return detail::Md5Typed<int16>(array);
  case DataType::uint16:
    return detail::Md5Typed<uint16>(array);
  case DataType::int32:
    return detail::Md5Typed<int32>(array);
  case DataType::uint32:
    return detail::Md5Typed<uint32>(array);
  case DataType::int64:
    return detail::Md5Typed<int64>(array);
  case DataType::uint64:
    return detail::Md5Typed<uint64>(array);
  default:
    return {};
  }
}

// §2.C optional OOC direct check: materialize an OOC output into a contiguous buffer via copyIntoBuffer, then md5 it,
// so the OOC code path can be hashed on the real inputs too. Identical bytes to ComputeMd5Hash for an in-core store.
inline std::string MaterializeAndMd5(DataStructure& dataStructure, const DataPath& outputData)
{
  return ComputeMd5Hash(dataStructure, outputData); // Md5Typed already materializes an OOC store element-by-element
}

//------------------------------------------------------------------------------
// Creates the legacy ITK filter by UUID (resolved from the loaded ITKImageProcessing plugin) and runs preflight +
// execute with @p args on @p dataStructure. Caller MUST have an active ForceInCore PreferencesSentinel: the legacy
// ITK filters dynamic_cast buffers to the in-core DataStore<T> and throw bad_cast on an OOC store.
inline Result<> RunLegacyItkFilter(const Uuid& legacyUuid, DataStructure& dataStructure, const Arguments& args)
{
  IFilter::UniquePointer filter = Application::Instance()->getFilterList()->createFilter(legacyUuid);
  if(filter == nullptr)
  {
    return MakeErrorResult(-9002, fmt::format("Legacy ITK filter {} could not be created (is ITKImageProcessing loaded?)", legacyUuid.str()));
  }
  auto preflight = filter->preflight(dataStructure, args);
  if(preflight.outputActions.invalid())
  {
    return ConvertResult(std::move(preflight.outputActions));
  }
  return filter->execute(dataStructure, args).result;
}

// Reverse lookup: given OUR filter's UUID, return the legacy ITK UUID that redirects to it (from the ImageProcessing
// plugin's oldItkUuid -> newUuid replacement map). Lets a per-filter test avoid hardcoding the legacy UUID.
inline std::optional<Uuid> LegacyUuidFor(const Uuid& newFilterUuid)
{
  const auto plugins = Application::Instance()->getFilterList()->getLoadedPlugins();
  for(const auto* plugin : plugins)
  {
    if(plugin->getName() != "ImageProcessing")
    {
      continue;
    }
    for(const auto& [oldUuid, mappedNew] : plugin->getFilterReplacementMap())
    {
      if(mappedNew == newFilterUuid)
      {
        return oldUuid;
      }
    }
  }
  return {};
}
} // namespace ip_golden

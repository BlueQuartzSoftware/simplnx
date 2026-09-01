#pragma once

#include "simplnx/Utilities/OStreamUtilities.hpp"

#include <memory>

namespace nx::core
{

/**
 * @brief Limits an ASCII VTK formatting buffer to 1,000,000 bytes.
 */
static constexpr usize k_BufferDumpVal = 1000000;

/**
 * @brief Maps a primitive C++ type to a legacy VTK scalar type name.
 * @tparam T Specifies the primitive C++ type.
 * @param messageHandler Receives an unsupported-type diagnostic.
 * @return Legacy VTK scalar type name, or an empty string for an unsupported type.
 */
template <typename T>
std::string TypeForPrimitive(const IFilter::MessageHandler& messageHandler)
{
  if constexpr(std::is_same_v<T, float32>)
  {
    return "float";
  }
  if constexpr(std::is_same_v<T, float64>)
  {
    return "double";
  }

  if constexpr(std::is_same_v<T, int8>)
  {
    return "char";
  }
  if constexpr(std::is_same_v<T, uint8>)
  {
    return "unsigned_char";
  }
  if constexpr(std::is_same_v<T, char>)
  {
    return "char";
  }
  if constexpr(std::is_same_v<T, signed char>)
  {
    return "char";
  }
  if constexpr(std::is_same_v<T, unsigned char>)
  {
    return "char";
  }

  if constexpr(std::is_same_v<T, int16>)
  {
    return "short";
  }
  if constexpr(std::is_same_v<T, short>)
  {
    return "short";
  }
  if constexpr(std::is_same_v<T, signed short>)
  {
    return "short";
  }
  if constexpr(std::is_same_v<T, uint16>)
  {
    return "unsigned_short";
  }
  if constexpr(std::is_same_v<T, unsigned short>)
  {
    return "unsigned_short";
  }

  if constexpr(std::is_same_v<T, int32>)
  {
    return "int";
  }
  if constexpr(std::is_same_v<T, uint32>)
  {
    return "unsigned_int";
  }
  if constexpr(std::is_same_v<T, int>)
  {
    return "int";
  }
  if constexpr(std::is_same_v<T, signed int>)
  {
    return "int";
  }
  if constexpr(std::is_same_v<T, unsigned int>)
  {
    return "unsigned_int";
  }

  if constexpr(std::is_same_v<T, long int>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, signed long int>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, unsigned long int>)
  {
    return "unsigned_long";
  }

  if constexpr(std::is_same_v<T, long long int>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, signed long long int>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, unsigned long long int>)
  {
    return "unsigned_long";
  }
  if constexpr(std::is_same_v<T, int64>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, uint64>)
  {
    return "unsigned_long";
  }

  if constexpr(std::is_same_v<T, bool>)
  {
    return "char";
  }

  messageHandler(IFilter::Message::Type::Info, fmt::format("Error: TypeForPrimitive - Unknown Type: ", typeid(T).name()));
  if(const char* name = typeid(T).name(); nullptr != name && name[0] == 'l')
  {
    messageHandler(
        IFilter::Message::Type::Info,
        fmt::format("You are using 'long int' as a type which is not 32/64 bit safe. It is suggested you use one of the H5SupportTypes defined in <Common/H5SupportTypes.h> such as int32 or uint32.",
                    typeid(T).name()));
  }
  return "";
}

/**
 * @struct WriteVtkDataArrayFunctor
 * @brief Type-dispatched writer for one numeric DataArray in the legacy VTK
 * scalar-array representation.
 *
 * Both output modes read fixed-size contiguous pages. This design avoids one
 * disk-backed source read for each scalar. The writer converts binary pages to
 * VTK's required big-endian byte order in a temporary buffer. Boolean values
 * use an explicit byte buffer because packed or implementation-defined bool
 * storage cannot be written as raw VTK bytes.
 */
struct WriteVtkDataArrayFunctor
{
  /**
   * @brief Writes the array selected by @p arrayPath to an open VTK file.
   * @tparam T Primitive component type selected from the array's runtime type.
   * @param outputFile Destination owned and closed by the caller.
   * @param binary True to write big-endian binary values; false to write ASCII.
   * @param dataStructure DataStructure that owns the source array.
   * @param arrayPath Path of the numeric array to write.
   * @param messageHandler Receives the per-array progress message.
   * @return Error if a source page cannot be read or a binary page cannot be written.
   * @pre arrayPath identifies a DataArray<T>.
   *
   * ASCII FILE write status and header write status are not reported.
   */
  template <typename T>
  Result<> operator()(FILE* outputFile, bool binary, DataStructure& dataStructure, const DataPath& arrayPath, const IFilter::MessageHandler& messageHandler)
  {
    auto* dataArray = dataStructure.getDataAs<DataArray<T>>(arrayPath);
    auto& dataStore = dataArray->getDataStoreRef();

    messageHandler(IFilter::Message::Type::Info, fmt::format("Writing Cell Data {}", arrayPath.getTargetName()));

    const usize totalElements = dataStore.getSize();
    const int numComps = static_cast<int>(dataStore.getNumberOfComponents());
    std::string dName = arrayPath.getTargetName();
    dName = StringUtilities::replace(dName, " ", "_");

    const std::string vtkTypeString = TypeForPrimitive<T>(messageHandler);
    bool useIntCast = false;
    if(vtkTypeString == "unsigned_char" || vtkTypeString == "char")
    {
      useIntCast = true;
    }

    fprintf(outputFile, "SCALARS %s %s %d\n", dName.c_str(), vtkTypeString.c_str(), numComps);
    fprintf(outputFile, "LOOKUP_TABLE default\n");
    if(binary)
    {
      constexpr usize k_ChunkSize = 4096;
      const usize bufferSize = std::min(k_ChunkSize, std::max<usize>(1, totalElements));
      auto buf = std::make_unique<T[]>(bufferSize);
      std::unique_ptr<uint8[]> boolBytes;
      if constexpr(std::is_same_v<T, bool>)
      {
        boolBytes = std::make_unique<uint8[]>(bufferSize);
      }
      for(usize offset = 0; offset < totalElements; offset += k_ChunkSize)
      {
        const usize count = std::min(k_ChunkSize, totalElements - offset);
        auto copyResult = dataStore.copyIntoBuffer(offset, nonstd::span<T>(buf.get(), count));
        if(copyResult.invalid())
        {
          return MakeErrorResult(-2090, fmt::format("Failed to read chunk [{}, {}) from data array '{}' while writing VTK file: {}", offset, offset + count, arrayPath.toString(),
                                                    copyResult.errors().empty() ? "unknown error" : copyResult.errors()[0].message));
        }

        if constexpr(std::is_same_v<T, bool>)
        {
          for(usize i = 0; i < count; i++)
          {
            boolBytes[i] = buf[i] ? uint8{1} : uint8{0};
          }
          if(fwrite(boolBytes.get(), sizeof(uint8), count, outputFile) != count)
          {
            return MakeErrorResult(-2090, fmt::format("Failed to write chunk [{}, {}) from data array '{}' while writing VTK file.", offset, offset + count, arrayPath.toString()));
          }
        }
        else
        {
          if constexpr(endian::little == endian::native)
          {
            for(usize i = 0; i < count; i++)
            {
              buf[i] = nx::core::byteswap(buf[i]);
            }
          }
          if(fwrite(buf.get(), sizeof(T), count, outputFile) != count)
          {
            return MakeErrorResult(-2090, fmt::format("Failed to write chunk [{}, {}) from data array '{}' while writing VTK file.", offset, offset + count, arrayPath.toString()));
          }
        }
      }
      fprintf(outputFile, "\n");
    }
    else
    {
      std::string buffer;
      buffer.reserve(k_BufferDumpVal);
      constexpr usize k_ChunkSize = 4096;
      auto values = std::make_unique<T[]>(std::min(k_ChunkSize, std::max<usize>(1, totalElements)));
      for(usize offset = 0; offset < totalElements; offset += k_ChunkSize)
      {
        const usize count = std::min(k_ChunkSize, totalElements - offset);
        Result<> copyResult = dataStore.copyIntoBuffer(offset, nonstd::span<T>(values.get(), count));
        if(copyResult.invalid())
        {
          return copyResult;
        }
        for(usize localIndex = 0; localIndex < count; localIndex++)
        {
          const usize globalIndex = offset + localIndex;
          if(globalIndex % 20 == 0 && globalIndex > 0)
          {
            buffer.append("\n");
          }
          if(useIntCast)
          {
            buffer.append(fmt::format(" {:d}", static_cast<int>(values[localIndex])));
          }
          else if constexpr(std::is_floating_point_v<T>)
          {
            buffer.append(fmt::format(" {:f}", values[localIndex]));
          }
          else
          {
            buffer.append(fmt::format(" {}", values[localIndex]));
          }
          if(buffer.size() > (k_BufferDumpVal - 32))
          {
            fprintf(outputFile, "%s", buffer.c_str());
            buffer.clear();
            buffer.reserve(k_BufferDumpVal);
          }
        }
      }
      buffer.append("\n");
      fprintf(outputFile, "%s", buffer.c_str());
    }
    return {};
  }
};

/**
 * @brief Maps a primitive C++ type to a legacy VTK scalar type name.
 * @tparam T Specifies a supported primitive C++ type.
 * @return Legacy VTK scalar type name.
 */
template <class T>
inline std::string ConvertDataTypeToVtkDataType() noexcept
{
  if constexpr(std::is_same_v<T, int8> || std::is_same_v<T, bool>)
  {
    return "char";
  }
  else if constexpr(std::is_same_v<T, uint8>)
  {
    return "unsigned_char";
  }
  else if constexpr(std::is_same_v<T, int16>)
  {
    return "short";
  }
  else if constexpr(std::is_same_v<T, uint16>)
  {
    return "unsigned_short";
  }
  else if constexpr(std::is_same_v<T, int32>)
  {
    return "int";
  }
  else if constexpr(std::is_same_v<T, uint32>)
  {
    return "unsigned_int";
  }
  else if constexpr(std::is_same_v<T, int64>)
  {
    return "long";
  }
  else if constexpr(std::is_same_v<T, uint64>)
  {
    return "unsigned_long";
  }
  else if constexpr(std::is_same_v<T, float32>)
  {
    return "float";
  }
  else if constexpr(std::is_same_v<T, float64>)
  {
    return "double";
  }
  else
  {
    static_assert(dependent_false<T>, "ConvertDataTypeToVtkDataType: Unsupported type");
  }
}

/**
 * @struct WriteVtkDataFunctor
 * @brief Writes numeric arrays with the legacy VTK serializer.
 *
 * Binary output byte-swaps the source array before it writes the data. It
 * restores native byte order after the write. This serializer uses direct
 * DataArray access and does not establish generic DataArray thread safety.
 */
struct WriteVtkDataFunctor
{
  /**
   * @brief Writes one numeric DataArray in legacy VTK format.
   * @tparam T Specifies the numeric array type.
   * @param outStrm Receives VTK scalar data.
   * @param iDataArray Supplies a DataArray<T>.
   * @param binary True to write binary output; false to write ASCII.
   * @param messageHandler Receives periodic ASCII-write progress.
   * @param shouldCancel Signals ASCII-write cancellation.
   * @return Success after completion or cancellation.
   * @pre iDataArray has type DataArray<T>.
   *
   * Binary output does not inspect shouldCancel. Binary and ASCII stream errors
   * are not returned. ASCII cancellation can leave a written prefix.
   */
  template <typename T>
  Result<> operator()(std::ofstream& outStrm, IDataArray& iDataArray, bool binary, const nx::core::IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    using DataArrayType = DataArray<T>;

    auto& dataArrayRef = dynamic_cast<DataArrayType&>(iDataArray);
    const auto& dataStoreRef = dataArrayRef.getDataStoreRef();

    std::string name = StringUtilities::replace(dataArrayRef.getName(), " ", "_");

    outStrm << "SCALARS " << name << " " << ConvertDataTypeToVtkDataType<T>() << " " << dataArrayRef.getNumberOfComponents() << "\n";
    outStrm << "LOOKUP_TABLE default\n";

    if(binary)
    {
      // Legacy VTK binary data uses big-endian byte order.
      if constexpr(endian::little == endian::native)
      {
        dataArrayRef.byteSwapElements();
      }

      auto result = dataStoreRef.writeBinaryFile(outStrm);
      if(result.first != 0)
      {
      }

      // Restore the source array to native byte order.
      if constexpr(endian::little == endian::native)
      {
        dataArrayRef.byteSwapElements();
      }
    }
    else
    {
      const usize k_DefaultElementsPerLine = 10;
      auto start = std::chrono::steady_clock::now();
      auto numTuples = dataStoreRef.getSize();
      usize currentItemCount = 0;

      for(usize idx = 0; idx < numTuples; idx++)
      {
        auto now = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
        {
          auto string = fmt::format("Processing {}: {}% completed", dataArrayRef.getName(), static_cast<int32>(100 * static_cast<float32>(idx) / static_cast<float32>(numTuples)));
          messageHandler(IFilter::Message::Type::Info, string);
          start = now;
          if(shouldCancel)
          {
            return {};
          }
        }

        if constexpr(std::is_same_v<T, int8> || std::is_same_v<T, uint8>)
        {
          outStrm << static_cast<int32>(dataArrayRef[idx]);
        }
        else if constexpr(std::is_same_v<T, float32> || std::is_same_v<T, float64>)
        {
          outStrm << fmt::format("{}", dataArrayRef.getValue(idx));
        }
        else
        {
          outStrm << dataArrayRef[idx];
        }
        if(currentItemCount < k_DefaultElementsPerLine - 1)
        {
          outStrm << ' ';
          currentItemCount++;
        }
        else
        {
          outStrm << "\n";
          currentItemCount = 0;
        }
      }
    }
    outStrm << "\n";
    return {};
  }
};
} // namespace nx::core

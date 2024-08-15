#pragma once

#include "simplnx/Utilities/OStreamUtilities.hpp"

namespace nx::core
{

static constexpr usize k_BufferDumpVal = 1000000;

// -----------------------------------------------------------------------------
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
        fmt::format(
            "You are using 'long int' as a type which is not 32/64 bit safe. It is suggested you use one of the H5SupportTypes defined in <Common/H5SupportTypes.h> such as int32_t or uint32_t.",
            typeid(T).name()));
  }
  return "";
}

// -----------------------------------------------------------------------------
struct WriteVtkDataArrayFunctor
{
  template <typename T>
  void operator()(FILE* outputFile, bool binary, DataStructure& dataStructure, const DataPath& arrayPath, const IFilter::MessageHandler& messageHandler)
  {
    auto* dataArray = dataStructure.getDataAs<DataArray<T>>(arrayPath);
    auto& dataStore = dataArray->template getIDataStoreRefAs<DataStore<T>>();

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
      if constexpr(endian::little == endian::native)
      {
        dataArray->byteSwapElements();
      }
      fwrite(dataStore.data(), sizeof(T), totalElements, outputFile);
      fprintf(outputFile, "\n");
      if constexpr(endian::little == endian::native)
      {
        dataArray->byteSwapElements();
      }
    }
    else
    {
      std::string buffer;
      buffer.reserve(k_BufferDumpVal);
      for(size_t i = 0; i < totalElements; i++)
      {
        if(i % 20 == 0 && i > 0)
        {
          buffer.append("\n");
        }
        if(useIntCast)
        {
          buffer.append(fmt::format(" {:d}", static_cast<int>(dataStore[i])));
        }
        else if constexpr(std::is_floating_point_v<T>)
        {
          buffer.append(fmt::format(" {:f}", dataStore[i]));
        }
        else
        {
          buffer.append(fmt::format(" {}", dataStore[i]));
        }
        // If the buffer is within 32 bytes of the reserved size, then dump
        // the contents to the file.
        if(buffer.size() > (k_BufferDumpVal - 32))
        {
          fprintf(outputFile, "%s", buffer.c_str());
          buffer.clear();
          buffer.reserve(k_BufferDumpVal);
        }
      }
      buffer.append("\n");
      fprintf(outputFile, "%s", buffer.c_str());
    }
  }
};

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

struct WriteVtkDataFunctor
{
  template <typename T>
  Result<> operator()(std::ofstream& outStrm, IDataArray& iDataArray, bool binary, const nx::core::IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    using DataArrayType = DataArray<T>;
    using DataStoreType = typename DataArrayType::store_type;

    auto& dataArrayRef = dynamic_cast<DataArrayType&>(iDataArray);
    const auto& dataStoreRef = dataArrayRef.getDataStoreRef();

    std::string name = StringUtilities::replace(dataArrayRef.getName(), " ", "_");

    outStrm << "SCALARS " << name << " " << ConvertDataTypeToVtkDataType<T>() << " " << dataArrayRef.getNumberOfComponents() << "\n";
    outStrm << "LOOKUP_TABLE default\n";

    if(binary)
    {
      // Swap to big Endian... because
      if constexpr(endian::little == endian::native)
      {
        dataArrayRef.byteSwapElements();
      }

      auto result = dataStoreRef.writeBinaryFile(outStrm);
      if(result.first != 0)
      {
      }

      // Swap back to little endian
      if constexpr(endian::little == endian::native)
      {
        dataArrayRef.byteSwapElements();
      }
    }
    else
    {
      const size_t k_DefaultElementsPerLine = 10;
      auto start = std::chrono::steady_clock::now();
      auto numTuples = dataStoreRef.getSize();
      size_t currentItemCount = 0;

      for(size_t idx = 0; idx < numTuples; idx++)
      {
        auto now = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
        {
          auto string = fmt::format("Processing {}: {}% completed", dataArrayRef.getName(), static_cast<int32>(100 * static_cast<float>(idx) / static_cast<float>(numTuples)));
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
          outStrm << fmt::format("{}", dataArrayRef[idx]);
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
    outStrm << "\n"; // Always end with a new line for binary data
    return {};
  }
};
} // namespace nx::core

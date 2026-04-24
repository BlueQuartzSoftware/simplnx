#include "ImageIOUtilities.hpp"

namespace nx::core
{

usize BytesPerImageElement(DataType type)
{
  switch(type)
  {
  case DataType::uint8:
    return 1;
  case DataType::uint16:
    return 2;
  case DataType::uint32:
    return 4;
  case DataType::float32:
    return 4;
  default:
    return 0;
  }
}

DataType ChoiceToImageDataType(usize choice)
{
  switch(choice)
  {
  case 1:
    return DataType::uint16;
  case 2:
    return DataType::uint32;
  default:
    return DataType::uint8;
  }
}

usize ImageDataTypeToChoice(DataType type)
{
  switch(type)
  {
  case DataType::uint16:
    return 1;
  case DataType::uint32:
    return 2;
  default:
    return 0;
  }
}

} // namespace nx::core

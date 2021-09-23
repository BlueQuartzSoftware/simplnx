#pragma once

#include <cstdint>

#define H5_USE_110_API
#include <hdf5.h>
#include <H5Opublic.h>


#ifdef H5Support_USE_MUTEX
#include <mutex>
#define H5SUPPORT_MUTEX_LOCK()                                                                                                                                                                         \
  std::mutex mutex;                                                                                                                                                                                    \
  std::lock_guard<std::mutex> lock(mutex);
#else
#define H5SUPPORT_MUTEX_LOCK()
#endif


namespace complex
{
namespace H5
{
using IdType = int64_t;
using ErrorType = int32_t;
using SizeType = unsigned long long;

inline const std::string DataTypeTag = "DataType";
inline const std::string CompDims = "ComponentDimensions";
inline const std::string TupleDims = "TupleDimensions";
} // namespace H5
} // namespace complex

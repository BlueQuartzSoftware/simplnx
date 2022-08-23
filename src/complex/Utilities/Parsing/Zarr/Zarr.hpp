#pragma once

namespace complex::Zarr
{
using ErrorType = int64_t;
} // namespace complex::Zarr

namespace FileVec
{
class BaseCollection;
class IGroup;
class BaseGenericArray;

template <typename T>
class IArray;
} // namespace FileVec

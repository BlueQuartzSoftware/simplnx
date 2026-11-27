#include "EmptyDataStore.hpp"

namespace nx::core
{
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<int8>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<uint8>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<int16>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<uint16>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<int32>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<uint32>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<int64>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<uint64>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<float32>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<float64>;
template class SIMPLNX_TEMPLATE_EXPORT EmptyDataStore<bool>;
} // namespace nx::core

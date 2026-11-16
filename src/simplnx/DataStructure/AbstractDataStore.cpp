#include "AbstractDataStore.hpp"

namespace nx::core
{
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<int8>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<uint8>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<int16>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<uint16>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<int32>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<uint32>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<int64>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<uint64>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<float32>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<float64>;
template class SIMPLNX_TEMPLATE_EXPORT AbstractDataStore<bool>;
} // namespace nx::core

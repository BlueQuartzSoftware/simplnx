#include "DataStore.hpp"

namespace nx::core
{
template class SIMPLNX_TEMPLATE_EXPORT DataStore<int8>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<uint8>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<int16>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<uint16>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<int32>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<uint32>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<int64>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<uint64>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<float32>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<float64>;
template class SIMPLNX_TEMPLATE_EXPORT DataStore<bool>;
} // namespace nx::core

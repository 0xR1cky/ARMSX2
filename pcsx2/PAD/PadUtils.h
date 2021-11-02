
#pragma once

#include <limits>

template<typename T>
constexpr u8 Normalize(T value)
{
	float f = (static_cast<float>(value) - std::numeric_limits<T>::min()) / (std::numeric_limits<T>::max() - std::numeric_limits<T>::min());
	return 0xff * f;
}

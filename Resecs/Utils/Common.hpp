#pragma once

namespace Resecs {
	template<typename T>
	void EnlargeVectorToFit(T& vecVal,size_t index) {
		if (index >= vecVal.size())
		{
			vecVal.resize((index + 1) * 2.0f);
		}
	}
}
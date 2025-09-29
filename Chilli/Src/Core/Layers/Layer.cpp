#include "Ch_PCH.h"
#include "Layer.h"

namespace Chilli
{
	Layer::Layer(const char* Name)
		:_Name(Name)
	{
		CH_CORE_INFO("Layer Added: {0}", Name);
	}
	
	Layer::~Layer()
	{
		CH_CORE_INFO("Layer Deleted: {0}", _Name);
	}
}

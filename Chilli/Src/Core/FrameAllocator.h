#pragma once

#include "MemoryArena.h"

namespace Chilli
{
	class FrameAllocator
	{
	public:
		FrameAllocator() {}
		~FrameAllocator() { Free(); }

		inline void Ref(MemoryArena& Other, size_t NewCapacity)
		{
			if (_Arena != nullptr)
				Free();

			_Arena = Other.Alloc(NewCapacity);
			_Capacity = NewCapacity;
			_Size = 0;
			_Refrenced = true;
		}

		inline void Prepare(size_t Size)
		{
			if (_Arena != nullptr)
				Free();

			_Arena = malloc(Size);
			_Capacity = Size;
			_Size = 0;
			_Refrenced = false;
		}

		inline void* Alloc(size_t Size)
		{
			if (_Capacity == 0 || _Arena == nullptr)
				return nullptr;

			if (_Size + Size > _Capacity)
				return nullptr;

			_Size += Size;
			return (uint8_t*)_Arena + (_Size - Size);
		}

		inline uint32_t Capacity() const { return _Capacity; }
		inline size_t Size() const { return _Size; }

		inline void ResetSize()
		{
			_Size = 0;
		}

	private:
		inline void Free()
		{
			if (!_Refrenced)
				free(_Arena);
			_Arena = nullptr;
			_Capacity = 0;
			_Size = 0;
		}

	private:
		void* _Arena = nullptr;
		uint32_t _Capacity = 0;
		size_t _Size = 0;
		bool _Refrenced = false;
	};

}

#pragma once

namespace Chilli
{
	class MemoryArena
	{
	public:
		MemoryArena() {}
		~MemoryArena() { Free(); }

		inline void Prepare(size_t Size)
		{
			if (_Arena != nullptr)
				Free();

			_Arena = malloc(Size);
			_Capacity = Size;
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

		inline void Free()
		{
			free(_Arena);
			_Arena = nullptr;
			_Capacity = 0;
			_Size = 0;
		}

		inline uint32_t Capacity() const { return _Capacity; }
		inline size_t Size() const { return _Size; }

	private:
		void* _Arena = nullptr;
		uint32_t _Capacity = 0;
		size_t _Size = 0;
	};
}

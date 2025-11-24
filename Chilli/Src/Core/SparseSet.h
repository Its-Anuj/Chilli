#pragma once

namespace Chilli
{
	template<typename T>
	class SparseSet
	{
	public:
		static constexpr uint32_t npos = static_cast<uint32_t>(-1);

		SparseSet() = default;
		~SparseSet() = default;

		uint32_t Create(const T& val)
		{
			uint32_t id;
			if (!_FreeList.empty()) {
				id = _FreeList.back();
				_FreeList.pop_back();
			}
			else {
				id = NextId++;
				if (id >= _Sparse.size())
					_Sparse.resize(id + 1, npos);
			}

			_Sparse[id] = _Dense.size();
			_Dense.push_back(id);
			_Data.push_back(val);
			return id;
		}

		void Destroy(uint32_t id)
		{
			if (!HasVal(id)) return;

			uint32_t index = _Sparse[id];
			uint32_t lastIndex = _Dense.size() - 1;
			uint32_t lastVal = _Dense[lastIndex];

			if (index != lastIndex)
			{
				_Dense[index] = lastVal;
				_Data[index] = std::move(_Data[lastIndex]);
				_Sparse[lastVal] = index;
			}

			_Dense.pop_back();
			_Data.pop_back();
			_Sparse[id] = npos;
			_FreeList.push_back(id);
		}

		T* Get(uint32_t id)
		{
			return HasVal(id) ? &_Data[_Sparse[id]] : nullptr;
		}

		const T* Get(uint32_t id) const
		{
			return HasVal(id) ? &_Data[_Sparse[id]] : nullptr;
		}

		bool HasVal(uint32_t id) const {
			if (id >= _Sparse.size())
				return false;

			return _Sparse[id] != npos;
		}

		uint32_t size() const { return _Dense.size(); }
		bool empty() const { return _Dense.empty(); }
		
		bool Contains(uint32_t Id) const {
			if (Id >= _Sparse.size())         // >= because valid indices are 0..size-1
				return false;
			if (_Sparse[Id] >= _Dense.size()) // >= same reason
				return false;

			return _Dense[_Sparse[Id]] == Id;
		}

		auto begin() { return _Data.begin(); }
		auto end() { return _Data.end(); }

		auto begin() const { return _Data.begin(); }
		auto end() const { return _Data.end(); }

		const uint32_t GetActiveCount() const { return _Dense.size(); }
		const uint32_t GetSparseCount() const { return _Sparse.size(); }
	private:
		uint32_t NextId = 0;

		std::vector<uint32_t> _Sparse;
		std::vector<uint32_t> _Dense;
		std::vector<T> _Data;
		std::vector<uint32_t> _FreeList;
	};
}

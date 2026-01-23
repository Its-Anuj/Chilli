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

		// --- Original Functions ---

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

			_Sparse[id] = static_cast<uint32_t>(_Dense.size());
			_Dense.push_back(id);
			_Data.push_back(val);
			return id;
		}

		void Destroy(uint32_t id)
		{
			if (!HasVal(id)) return;

			uint32_t index = _Sparse[id];
			uint32_t lastIndex = static_cast<uint32_t>(_Dense.size() - 1);
			uint32_t lastId = _Dense[lastIndex];

			if (index != lastIndex)
			{
				_Dense[index] = lastId;
				_Data[index] = std::move(_Data[lastIndex]);
				_Sparse[lastId] = index;
			}

			_Dense.pop_back();
			_Data.pop_back();
			_Sparse[id] = npos;
			_FreeList.push_back(id);
		}

		// --- New: Manual Insertion (For AssetHandle IDs) ---

		void Insert(uint32_t id, const T& val)
		{
			if (id >= _Sparse.size())
				_Sparse.resize(id + 1, npos);

			// 1. If ID is already in the FreeList, remove it so Create() doesn't reuse it
			auto it = std::find(_FreeList.begin(), _FreeList.end(), id);
			if (it != _FreeList.end()) {
				_FreeList.erase(it);
			}

			// 2. Advance NextId to prevent future collisions
			if (id >= NextId) {
				NextId = id + 1;
			}

			// 3. Update existing or Add new
			if (_Sparse[id] != npos) {
				_Data[_Sparse[id]] = val;
			}
			else {
				_Sparse[id] = static_cast<uint32_t>(_Dense.size());
				_Dense.push_back(id);
				_Data.push_back(val);
			}
		}

		// --- Helpers & Accessors ---

		T* Get(uint32_t id) { return HasVal(id) ? &_Data[_Sparse[id]] : nullptr; }
		const T* Get(uint32_t id) const { return HasVal(id) ? &_Data[_Sparse[id]] : nullptr; }

		bool HasVal(uint32_t id) const {
			return (id < _Sparse.size() && _Sparse[id] != npos);
		}

		bool Contains(uint32_t id) const {
			if (id >= _Sparse.size()) return false;
			uint32_t denseIdx = _Sparse[id];
			return (denseIdx < _Dense.size() && _Dense[denseIdx] == id);
		}

		void Clear() {
			_Sparse.clear();
			_Dense.clear();
			_Data.clear();
			_FreeList.clear();
			NextId = 0;
		}

		// --- Iteration & Metadata ---

		uint32_t size() const { return static_cast<uint32_t>(_Dense.size()); }
		bool empty() const { return _Dense.empty(); }

		auto begin() { return _Data.begin(); }
		auto end() { return _Data.end(); }
		auto begin() const { return _Data.begin(); }
		auto end() const { return _Data.end(); }

		// Useful for getting the ID of an element while iterating _Data
		uint32_t GetIdAtDenseIndex(uint32_t index) const { return _Dense[index]; }

		T* GetDataBuffer() { return _Data.data(); }
		const uint32_t GetActiveCount() const { return static_cast<uint32_t>(_Dense.size()); }
		const uint32_t GetSparseCount() const { return static_cast<uint32_t>(_Sparse.size()); }

	private:
		uint32_t NextId = 0;
		std::vector<uint32_t> _Sparse;
		std::vector<uint32_t> _Dense;
		std::vector<T> _Data;
		std::vector<uint32_t> _FreeList;
	};
}

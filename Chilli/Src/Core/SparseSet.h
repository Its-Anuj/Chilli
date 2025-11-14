#pragma once

namespace Chilli
{
    template<typename T>
    class SparseSet
    {
    public:
        static constexpr size_t npos = static_cast<size_t>(-1);

        SparseSet() = default;
        ~SparseSet() = default;

        size_t Create(const T& val)
        {
            size_t id;
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

        void Destroy(size_t id)
        {
            if (!HasVal(id)) return;

            size_t index = _Sparse[id];
            size_t lastIndex = _Dense.size() - 1;
            size_t lastVal = _Dense[lastIndex];

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

        T* Get(size_t id)
        {
            return HasVal(id) ? &_Data[_Sparse[id]] : nullptr;
        }

        const T* Get(size_t id) const
        {
            return HasVal(id) ? &_Data[_Sparse[id]] : nullptr;
        }

        bool HasVal(size_t id) const
        {
            return id < _Sparse.size() && _Sparse[id] != npos;
        }

        size_t size() const { return _Dense.size(); }
        bool empty() const { return _Dense.empty(); }

        bool Contains(int SparseIndex, size_t Id) {
            return SparseIndex < _Dense.size() && _Dense[SparseIndex] == Id;
        };

        auto begin() { return _Data.begin(); }
        auto end() { return _Data.end(); }

        auto begin() const { return _Data.begin(); }
        auto end() const { return _Data.end(); }
    private:
        size_t NextId = 0;

        std::vector<size_t> _Sparse;
        std::vector<size_t> _Dense;
        std::vector<T> _Data;
        std::vector<size_t> _FreeList;
    };
}

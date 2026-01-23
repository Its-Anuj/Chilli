#pragma once

namespace Chilli
{
	template<typename _T>
	class Scope
	{
	public:
		explicit Scope(_T* ptr = nullptr) noexcept : _Ptr(ptr) {}

		Scope(const Scope&) = delete;

		Scope(Scope&& other) noexcept
			: _Ptr(other._Ptr)
		{
			other._Ptr = nullptr;
		}

		Scope& operator=(Scope&& other) noexcept
		{
			if (this != &other)
			{
				delete _Ptr;
				_Ptr = other._Ptr;
				other._Ptr = nullptr;
			}
			return *this;
		}

		~Scope()
		{
			delete _Ptr;
		}

		void Reset(_T* NewPtr = nullptr)
		{
			if (_Ptr != NewPtr)
			{
				delete _Ptr;
				_Ptr = NewPtr;
			}
		}

		_T* Release() noexcept
		{
			_T* tmp = _Ptr;
			_Ptr = nullptr;
			return tmp;
		}

		_T* Get() noexcept { return _Ptr; }
		const _T* Get() const noexcept { return _Ptr; }

		bool IsEmpty() const noexcept { return _Ptr == nullptr; }

		_T& operator*() { return *_Ptr; }
		const _T& operator*() const { return *_Ptr; }

		_T* operator->() noexcept { return _Ptr; }
		const _T* operator->() const noexcept { return _Ptr; }

	private:
		_T* _Ptr = nullptr;
	};

	template<typename _T, typename... Args>
	Scope<_T> Make_Scope(Args&&... args)
	{
		return Scope<_T>(new _T(std::forward<Args>(args)...));
	}

    template<typename _T>
    class Ref
    {
    public:
        Ref() noexcept = default;

        Ref(const Ref& other) noexcept
            : _Ptr(other._Ptr), _ReferenceCounter(other._ReferenceCounter)
        {
            IncrementCounter();
        }

        Ref(Ref&& other) noexcept
            : _Ptr(other._Ptr), _ReferenceCounter(other._ReferenceCounter)
        {
            other._Ptr = nullptr;
            other._ReferenceCounter = nullptr;
        }

        Ref& operator=(const Ref& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                _Ptr = other._Ptr;
                _ReferenceCounter = other._ReferenceCounter;
                IncrementCounter();
            }
            return *this;
        }

        Ref& operator=(Ref&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                _Ptr = other._Ptr;
                _ReferenceCounter = other._ReferenceCounter;
                other._Ptr = nullptr;
                other._ReferenceCounter = nullptr;
            }
            return *this;
        }

        ~Ref()
        {
            Reset();
        }

        void Reset()
        {
            if (!_ReferenceCounter)
                return;

            DecrementCounter();

            if (*_ReferenceCounter == 0)
            {
                delete _Ptr;
                delete _ReferenceCounter;
            }

            _Ptr = nullptr;
            _ReferenceCounter = nullptr;
        }

        uint32_t GetReferenceCount() const noexcept
        {
            return _ReferenceCounter ? *_ReferenceCounter : 0;
        }

        _T* Get() noexcept { return _Ptr; }
        const _T* Get() const noexcept { return _Ptr; }

        bool IsEmpty() const noexcept { return _Ptr == nullptr; }

        _T& operator*() { return *_Ptr; }
        const _T& operator*() const { return *_Ptr; }

        _T* operator->() noexcept { return _Ptr; }
        const _T* operator->() const noexcept { return _Ptr; }

    private:
        explicit Ref(_T* ptr)
            : _Ptr(ptr), _ReferenceCounter(new uint32_t(1)) {
        }

        void IncrementCounter()
        {
            if (_ReferenceCounter)
                ++(*_ReferenceCounter);
        }

        void DecrementCounter()
        {
            if (_ReferenceCounter)
                --(*_ReferenceCounter);
        }

    private:
        uint32_t* _ReferenceCounter = nullptr;
        _T* _Ptr = nullptr;

        template<typename U, typename... Args>
        friend Ref<U> Make_Ref(Args&&...);
    };

    template<typename _T, typename... Args>
    Ref<_T> Make_Ref(Args&&... args)
    {
        return Ref<_T>(new _T(std::forward<Args>(args)...));
    }

}

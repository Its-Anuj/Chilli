#pragma once

#include "Events/Events.h"
#include <cstdint>

namespace Chilli
{
	using EventID = std::uint32_t;

#pragma region Event Manager
	struct __IPerEventStorage__
	{
		virtual void Clear() = 0;
		virtual uint32_t GetActiveSize() const = 0;
	};

	template<typename _EventType>
	struct PerEventStorage : __IPerEventStorage__
	{
	private:
		std::vector< _EventType> Events;
		uint32_t ActiveSize = 0;

	public:

		void Push(const _EventType& e) {
			Events.push_back(e);
			ActiveSize++;
		}

		virtual uint32_t GetActiveSize() const override { return ActiveSize; }
		virtual void Clear() override { Events.clear(); ActiveSize = 0; }

		_EventType* Data() { return Events.data(); }
		const _EventType* Data() const { return Events.data(); }

		std::vector< _EventType>::iterator begin() { return Events.begin(); }
		std::vector< _EventType>::iterator end() { return Events.end(); }

		std::vector< _EventType>::const_iterator begin()  const { return Events.begin(); }
		std::vector< _EventType>::const_iterator end() const { return Events.end(); }
	};

	uint32_t GetNewEventID();

	template<typename _Type>
	uint32_t GetEventID()
	{
		static uint32_t ID = GetNewEventID();
		return ID;
	}

	struct EventHandler
	{
	public:
		EventHandler() {}
		~EventHandler()
		{
			// free all allocated storages
			for (auto& storage : _Storage)
				delete storage;

			_Storage.clear();
		}

		template<typename _EventType>
			requires std::derived_from<_EventType, Event>
		void Register()
		{
			EventID id = GetEventID<_EventType>();
			if (id < _Storage.size())
				return;

			_Storage.push_back(new PerEventStorage<_EventType>());
		}

		template<typename _EventType>
			requires std::derived_from<_EventType, Event>
		PerEventStorage<_EventType>* GetEventStorage()
		{
			EventID id = GetEventID<_EventType>();
			if (id >= _Storage.size())
				return nullptr;

			return static_cast<PerEventStorage<_EventType>*>(_Storage[id]);
		}

		template<typename _EventType>
			requires std::derived_from<_EventType, Event>
		void Add(const _EventType& e)
		{
			auto* storage = GetEventStorage<_EventType>();
			storage->Push(e);
		}

		template<typename _EventType>
			requires std::derived_from<_EventType, Event>
		void Clear()
		{
			EventID id = GetEventID<_EventType>();
			if (id >= _Storage.size())
				return;

			auto Storage = static_cast<PerEventStorage<_EventType>*>(_Storage[id]);
			Storage->Clear();
		}

		void ClearAll()
		{
			for (auto& storage : _Storage)
				storage->Clear();
		}
	private:
		std::vector<__IPerEventStorage__*> _Storage;
	};

	template<typename _EventType>
		requires std::derived_from<_EventType, Event>
	struct EventReader {
		PerEventStorage<_EventType>* storage = nullptr;

		EventReader() = default;
		EventReader(EventHandler* Handler) {
			storage = Handler->GetEventStorage<_EventType>();
		}

		// --- Range-for support using custom iterator ---
		struct Iterator {
			PerEventStorage<_EventType>* storage;
			uint32_t idx;

			using value_type = const _EventType;
			using reference = const _EventType&;
			using pointer = const _EventType*;

			Iterator(PerEventStorage<_EventType>* s, uint32_t i)
				: storage(s), idx(i) {
			}

			bool operator!=(const Iterator& other) const {
				return idx != other.idx;
			}

			reference operator*() const {
				return storage->Data()[idx];
			}

			Iterator& operator++() {
				idx++;
				return *this;
			}
		};

		// begin = first unread event
		Iterator begin() {
			return Iterator(storage, 0);
		}

		// end = storage->ActiveSize
		Iterator end() {
			return Iterator(storage, storage->GetActiveSize());
		}
	};

	template<typename _EventType>
		requires std::derived_from<_EventType, Event>
	struct EventWriter {
		PerEventStorage<_EventType>* storage = nullptr;
		EventWriter(const EventHandler& Handler) :storage(Handler.GetEventStorage<_EventType>()) {}

		void Write(const _EventType& e) {
			storage->Push(e);
		}
	};
#pragma endregion 

}

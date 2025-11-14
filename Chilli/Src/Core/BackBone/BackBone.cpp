#include "Ch_PCH.h"
#include "BackBone.h"
#include "DeafultExtensions.h"

namespace Chilli
{
	namespace BackBone
	{
		void Schedule::AddSystem(ScheduleTimer Stage, std::unique_ptr<System> S, App& App)
		{
			SystemContext Ctxt{ .Registry = App.Registry, .AssetRegistry = App.AssetRegistry,
			.ServiceRegistry = App.ServiceRegsitry };
			S->OnCreate(Ctxt);
			_Systems[Stage].push_back(std::move(S));
		}

		void Schedule::AddSystemFunction(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function)
		{
			_SystemFunctions[Stage].push_back(Function);
		}

		void Schedule::Run(App& App)
		{
			SystemContext Ctxt{ .Registry = App.Registry, .AssetRegistry = App.AssetRegistry,
			.ServiceRegistry = App.ServiceRegsitry };
			for (auto& [_, Systems] : _Systems)
			{
				for (auto& ScheduledSytem : Systems)
				{
					ScheduledSytem->OnBeforeRun(Ctxt);
					ScheduledSytem->Run(Ctxt);
					ScheduledSytem->OnAfterRun(Ctxt);
				}
			}
		}

		void Schedule::Run(ScheduleTimer Stage, App& App)
		{
			auto& Systems = _Systems[Stage];

			SystemContext Ctxt{ .Registry = App.Registry, .AssetRegistry = App.AssetRegistry,
			.ServiceRegistry = App.ServiceRegsitry };
			for (auto& ScheduledSytem : Systems)
			{
				ScheduledSytem->OnBeforeRun(Ctxt);
				ScheduledSytem->Run(Ctxt);
				ScheduledSytem->OnAfterRun(Ctxt);
			}

			auto& SystemFuncss = _SystemFunctions[Stage];
			for (auto& ScheduledSytem : SystemFuncss)
				ScheduledSytem(Ctxt);
		}

		void Schedule::Terminate(App& App)
		{
			SystemContext Ctxt{ .Registry = App.Registry, .AssetRegistry = App.AssetRegistry,
			.ServiceRegistry = App.ServiceRegsitry };

			for (auto& [_, Systems] : _Systems)
				for (auto& ScheduledSytem : Systems)
					ScheduledSytem->OnTerminate(Ctxt);
			_Systems.clear();
			_SystemFunctions.clear();
		}

		// Extensions
		void ExtensionRegistry::AddExtension(std::unique_ptr<Extension> Ext, bool BuildNow, App* app)
		{
			if (BuildNow)
				Ext->Build(*app);
			Extensions[Ext->ID()] = std::move(Ext);
		}

		void BackBone::ExtensionRegistry::BuildAll(App& app)
		{
		}

		void App::Run()
		{
			// Initialition
			SystemScheduler.Run(ScheduleTimer::START_UP, *this);
			auto WindowStore = AssetRegistry.GetStore<Window>();

			bool Is_Running = true;
			while (Is_Running)
			{
				SystemScheduler.Run(ScheduleTimer::UPDATE_BEGIN, *this);
				SystemScheduler.Run(ScheduleTimer::UPDATE, *this);
				SystemScheduler.Run(ScheduleTimer::UPDATE_END, *this);

				SystemScheduler.Run(ScheduleTimer::RENDER_BEGIN, *this);
				SystemScheduler.Run(ScheduleTimer::RENDER, *this);
				SystemScheduler.Run(ScheduleTimer::RENDER_END, *this);

				if (WindowStore != nullptr)
					for (auto& Win : WindowStore->Store)
					{
						Is_Running = false;
						if (Win.IsClose() == false)
							Is_Running = true;
						if (!Is_Running)
							CH_CORE_INFO("Window Close");
					}
			}

			// ShutDown
			SystemScheduler.Run(ScheduleTimer::SHUTDOWN, *this);
			SystemScheduler.Terminate(*this);
		}
	}
}

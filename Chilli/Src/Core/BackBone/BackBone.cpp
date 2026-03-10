#include "Ch_PCH.h"
#include "BackBone.h"
#include "DeafultExtensions.h"

namespace Chilli
{
	namespace BackBone
	{
		uint32_t GetNewResourceID() {
			static uint32_t NewID = 0;
			return NewID++;
		}

		uint32_t GetNewComponentID()
		{
			static uint32_t NewComponentID = 0;
			return NewComponentID++;
		}

		uint32_t GetNewAssetID()
		{
			static uint32_t NewAssetID = 0;
			return NewAssetID++;
		}

		uint32_t GetNewServiceID()
		{
			static uint32_t NewServiceID = 0;
			return NewServiceID++;
		}

		void Schedule::AddSystem(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function)
		{
			_SystemFunctions[int(Stage)].push_back(Function);
		}

		void Schedule::AddSystemOverLayBefore(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function)
		{
			_SystemOverLayBefore[int(Stage)].push_back(Function);
		}

		void Schedule::AddSystemOverLayAfter(ScheduleTimer Stage, const std::function<void(SystemContext&)>& Function)
		{
			_SystemOverLayAfter[int(Stage)].push_back(Function);
		}

		void Schedule::Run(ScheduleTimer Stage, SystemContext& Ctxt)
		{
			{
				auto& SystemFuncss = _SystemOverLayBefore[int(Stage)];
				for (auto& ScheduledSytem : SystemFuncss)
					ScheduledSytem(Ctxt);
			}
			{
				auto& SystemFuncss = _SystemFunctions[int(Stage)];
				for (auto& ScheduledSytem : SystemFuncss)
					ScheduledSytem(Ctxt);
			}
			{
				auto& SystemFuncss = _SystemOverLayAfter[int(Stage)];
				for (auto& ScheduledSytem : SystemFuncss)
					ScheduledSytem(Ctxt);
			}
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
			SystemScheduler.Run(ScheduleTimer::START_UP, Ctxt);

			// Ensure resource is present (though usually added in StartUp)
			if (!Registry.GetResource<GenericFrameData>())
				Registry.AddResource<GenericFrameData>();

			auto FrameData = Registry.GetResource<GenericFrameData>();
			FrameTimer Timer;
			Timer.Start();

			while (FrameData->IsRunning)
			{
				FrameData->Ts = Timer.Reset();
				float dt = FrameData->Ts.GetSecond();

				SystemScheduler.Run(ScheduleTimer::INPUT, Ctxt);

				// --- FIXED PIPELINE LOGIC ---
				// Helper to run a fixed stage
				auto ProcessFixedStage = [&](GenericFrameData::StageData& stage, ScheduleTimer timerEnum) {
					stage.Accumulator += dt;
					int safety = 0;
					while (stage.Accumulator >= stage.Ticks && safety < 5) {
						SystemScheduler.Run(timerEnum, Ctxt);
						stage.Accumulator -= stage.Ticks;
						safety++;
					}
					// Update Alpha for sub-frame interpolation
					stage.Alpha = stage.Accumulator / stage.Ticks;
					};

				// 1. Physics (Fixed Network/Physics)
				ProcessFixedStage(FrameData->FixedPhysicsData, ScheduleTimer::FIXED_NETWORK);

				// 2. Simulation (Collision/Verlet)
				ProcessFixedStage(FrameData->FixedSimulationData, ScheduleTimer::SIMULATION);

				// 3. AI (Decision making - usually lower frequency)
				ProcessFixedStage(FrameData->FixedAIData, ScheduleTimer::FIXED_AI);

				// 4. Triggers (Gameplay logic)
				ProcessFixedStage(FrameData->FixedTriggerData, ScheduleTimer::FIXED_TRIGGER);

				// --- VARIABLE PIPELINE LOGIC ---
				SystemScheduler.Run(ScheduleTimer::UPDATE, Ctxt);
				SystemScheduler.Run(ScheduleTimer::RENDER, Ctxt);
			}

			SystemScheduler.Run(ScheduleTimer::SHUTDOWN, Ctxt);
		}
	}
}

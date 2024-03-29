#include "core_undo.h"

namespace Undo
{
	using VoidVFPtr = void(**)();
	struct PolymorphicLayoutTest { i32 DummyField; virtual void VirtualFunc() = 0; };

	inline b8 CommandsHaveSameVFPtr(const Command& commandA, const Command& commandB)
	{
		// HACK: This ""technically"" may not work on different compilers as the vfptr might not be stored at the start of the object (or at all)
		//		 and even a static_assert() of an offsetof() isn't *technically* allowed (?) although it seems to be supported by all major compilers
		static_assert(offsetof(PolymorphicLayoutTest, DummyField) == sizeof(VoidVFPtr), "Expected virtual function table pointer at start of object");
		static_assert(std::is_polymorphic_v<Command> && sizeof(Command) >= sizeof(VoidVFPtr), "Only polymorphic objects have virtual function tables");
		return memcmp(&commandA, &commandB, sizeof(VoidVFPtr)) == 0;
	}

	static auto VectorPop(std::vector<std::unique_ptr<Command>>& vectorToPop)
	{
		assert(!vectorToPop.empty());

		auto popped = std::move(vectorToPop.back());
		vectorToPop.erase(vectorToPop.end() - 1);
		return popped;
	}

	void UndoHistory::FlushAndExecuteEndOfFrameCommands()
	{
		if (!CommandsToExecutedAtEndOfFrame.empty())
		{
			for (auto& commandToExecute : CommandsToExecutedAtEndOfFrame)
				TryMergeOrExecute(std::move(commandToExecute));

			CommandsToExecutedAtEndOfFrame.clear();
		}
	}

	void UndoHistory::TryMergeOrExecute(std::unique_ptr<Command> commandToExecute)
	{
		assert(commandToExecute != nullptr);
		commandToExecute->CreationTime = CPUTime::GetNow();
		commandToExecute->LastMergeTime = {};

		HasPendingChanges = true;
		NumberOfChangesMade++;

		if (!RedoStack.empty())
			RedoStack.clear();

		// HACK: This is definitely a bit hacky because it introduces a somewhat unpredictable outside variable of time
		//		 but if so required by the host application it can be disabled by setting the threshold to zero
		//		 and it automatically takes care of merging possibly unrelated commands easily without adding additional code to all call sites
		const Time timeSinceLastCommand = LastExecutedCommandStopwatch.Restart();
		Command* lastCommand = (!UndoStack.empty() ? UndoStack.back().get() : nullptr);

		const b8 mergeDisallowedByTypeMismatch = (lastCommand == nullptr || !CommandsHaveSameVFPtr(*commandToExecute, *lastCommand));
		const b8 mergeDisallowedByTimeOut = (CommandMergeTimeThreshold > Time::Zero()) && (timeSinceLastCommand > CommandMergeTimeThreshold);
		const b8 mergeDisallowedByCounter = (NumberOfCommandsToDisallowMergesFor > 0);
		if (NumberOfCommandsToDisallowMergesFor > 0) NumberOfCommandsToDisallowMergesFor--;

		if (!mergeDisallowedByTypeMismatch && !mergeDisallowedByTimeOut && !mergeDisallowedByCounter)
		{
			const auto result = lastCommand->TryMerge(*commandToExecute);
			if (result == MergeResult::Failed)
			{
				UndoStack.emplace_back(std::move(commandToExecute))->Redo();
			}
			else if (result == MergeResult::ValueUpdated)
			{
				lastCommand->Redo();
				lastCommand->LastMergeTime = CPUTime::GetNow();
			}
			else
			{
				assert(!"Invalid MergeResult enum value returned. Possibly garbage stack value due to missing return statement in implementation (?)");
			}
		}
		else
		{
			UndoStack.emplace_back(std::move(commandToExecute))->Redo();
		}
	}

	void UndoHistory::Undo(size_t count)
	{
		for (size_t i = 0; i < count; i++)
		{
			if (UndoStack.empty())
				break;

			HasPendingChanges = true;
			RedoStack.emplace_back(VectorPop(UndoStack))->Undo();
		}
	}

	void UndoHistory::Redo(size_t count)
	{
		for (size_t i = 0; i < count; i++)
		{
			if (RedoStack.empty())
				break;

			HasPendingChanges = true;
			UndoStack.emplace_back(VectorPop(RedoStack))->Redo();
		}
	}

	void UndoHistory::ClearAll()
	{
		ClearChangesWereMade();
		if (!CommandsToExecutedAtEndOfFrame.empty()) CommandsToExecutedAtEndOfFrame.clear();
		if (!UndoStack.empty()) UndoStack.clear();
		if (!RedoStack.empty()) RedoStack.clear();
	}
}

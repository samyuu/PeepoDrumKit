#include "core_undo.h"

namespace Hacks
{
#if !defined(_MSC_VER)
	static_assert(false, "This may not work on different compilers as they could either detect UB and delete the code outright or *technically* the vfptr might not be stored at the start of the object");
#endif

	template <typename T>
	static const void* GetVirtualFunctionTablePointer(const T& polymorphicObject)
	{
		static_assert(std::is_polymorphic_v<T> && sizeof(T) >= sizeof(void*), "Non-polymorphic objects do not have a virtual function table");

		// HACK: This is surely invoking undefined behavior but from my testing with _MSV_VER=1916 this is definitely reliable for both debug and release builds.
		//		 Alternatively it might be safer to memcpy() into a temp buffer or directly memcmp() with sizeof(void*) instead (?)
		const auto vfptr = *reinterpret_cast<const void* const*>(&polymorphicObject);
		return vfptr;
	}
}

namespace Undo
{
	// TODO: Maybe use templated static function pointer IDs (similar to std::any) instead of hacky vftable for type comparisons (?)
	static bool CommandsAreOfSameType(const Command& commandA, const Command& commandB)
	{
		return (Hacks::GetVirtualFunctionTablePointer(commandA) == Hacks::GetVirtualFunctionTablePointer(commandB));
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

		const bool mergeDisallowedByTypeMismatch = (lastCommand == nullptr || !CommandsAreOfSameType(*commandToExecute, *lastCommand));
		const bool mergeDisallowedByTimeOut = (CommandMergeTimeThreshold > Time::Zero()) && (timeSinceLastCommand > CommandMergeTimeThreshold);
		const bool mergeDisallowedByCounter = (NumberOfCommandsToDisallowMergesFor > 0);
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

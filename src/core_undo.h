#pragma once
#include "core_types.h"
#include <type_traits>
#include <string_view>
#include <vector>

namespace Undo
{
	enum class MergeResult : u8 { Failed, ValueUpdated };

	struct CommandInfo
	{
		std::string_view Description;
	};

	struct Command
	{
		// NOTE: The derived constructor should store new and old values as member fields and a reference to the data to be edited
		Command() = default;
		virtual ~Command() = default;

		// NOTE: Restore stored old value to the reference
		virtual void Undo() = 0;
		// NOTE: Apply stored new value to the reference
		virtual void Redo() = 0;

		// NOTE: The command parameter is garanteed to be safely castable to the type of the derived class.
		//		 Passed as non const reference to allow for move optimizations however it should not be mutated if the merge failed
		virtual MergeResult TryMerge(Command& commandToMerge) = 0;

		// NOTE: To be displayed to the user
		virtual CommandInfo GetInfo() const = 0;

		// NOTE: Automatically set by the parent UndoHistory, only meant to potentially be displayed to the user
		CPUTime CreationTime;
		CPUTime LastMergeTime;
	};

	// DEBUG: Swallows all arguments without functionality! Only intended to be used for quickly stubbing out commands that haven't been implemented yet
	struct UnimplementedDummyCommand : Command
	{
		template <typename... Args> UnimplementedDummyCommand(Args&&... args) {}
		void Undo() override {}
		void Redo() override {}
		MergeResult TryMerge(Command& commandToMerge) override { return MergeResult::Failed; }
		CommandInfo GetInfo() const override { return { "Unimplemented Command" }; }
	};

	struct UndoHistory
	{
		std::vector<std::unique_ptr<Command>> UndoStack, RedoStack;
		std::vector<std::unique_ptr<Command>> CommandsToExecutedAtEndOfFrame;
		b8 HasPendingChanges = false;
		i32 NumberOfChangesMade = 0;

		i32 NumberOfCommandsToDisallowMergesFor = 0;
		Time CommandMergeTimeThreshold = Time::FromSec(2.0);
		CPUStopwatch LastExecutedCommandStopwatch = CPUStopwatch::StartNew();

	public:
		template<typename CommandType, typename... Args>
		void Execute(Args&&... args)
		{
			static_assert(std::is_base_of_v<Command, CommandType>);
			TryMergeOrExecute(std::make_unique<CommandType>(std::forward<Args>(args)...));
		}

		template<typename CommandType, typename... Args>
		void ExecuteEndOfFrame(Args&&... args)
		{
			static_assert(std::is_base_of_v<Command, CommandType>);
			CommandsToExecutedAtEndOfFrame.emplace_back(std::make_unique<CommandType>(std::forward<Args>(args)...));
		}

		void TryMergeOrExecute(std::unique_ptr<Command> commandToExecute);
		void FlushAndExecuteEndOfFrameCommands();

	public:
		void Undo(size_t count = 1);
		void Redo(size_t count = 1);
		void ClearAll();

		inline b8 CanUndo() const { return !UndoStack.empty(); }
		inline b8 CanRedo() const { return !RedoStack.empty(); }
		inline void NotifyChangesWereMade() { HasPendingChanges = true; NumberOfChangesMade++; }
		inline void ClearChangesWereMade() { HasPendingChanges = false; NumberOfChangesMade = 0; }

		inline void DisallowMergeForLastCommand() { NumberOfCommandsToDisallowMergesFor = 1; }
		inline void ResetMergeTimeThresholdStopwatch() { LastExecutedCommandStopwatch.Restart(); }
	};
}

#pragma once
#include "core_types.h"
#include "core_undo.h"
#include "chart.h"

namespace PeepoDrumKit
{
	// NOTE: General chart commands
	namespace Commands
	{
		struct ChangeSongOffset : Undo::Command
		{
			ChangeSongOffset(ChartProject* chart, Time value) : Chart(chart), NewValue(value), OldValue(chart->SongOffset) {}

			void Undo() override { Chart->SongOffset = OldValue; }
			void Redo() override { Chart->SongOffset = NewValue; }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Chart != Chart)
					return Undo::MergeResult::Failed;

				NewValue = other->NewValue;
				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Change Song Offset" }; }

			ChartProject* Chart;
			Time NewValue, OldValue;
		};

		struct ChangeSongDemoStartTime : Undo::Command
		{
			ChangeSongDemoStartTime(ChartProject* chart, Time value) : Chart(chart), NewValue(value), OldValue(chart->SongDemoStartTime) {}

			void Undo() override { Chart->SongDemoStartTime = OldValue; }
			void Redo() override { Chart->SongDemoStartTime = NewValue; }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Chart != Chart)
					return Undo::MergeResult::Failed;

				NewValue = other->NewValue;
				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Change Song Demo Start" }; }

			ChartProject* Chart;
			Time NewValue, OldValue;
		};

		struct ChangeChartDuration : Undo::Command
		{
			ChangeChartDuration(ChartProject* chart, Time value) : Chart(chart), NewValue(value), OldValue(chart->ChartDuration) {}

			void Undo() override { Chart->ChartDuration = OldValue; }
			void Redo() override { Chart->ChartDuration = NewValue; }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Chart != Chart)
					return Undo::MergeResult::Failed;

				NewValue = other->NewValue;
				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Change Chart Duration" }; }

			ChartProject* Chart;
			Time NewValue, OldValue;
		};
	}

	// NOTE: Tempo map commands
	namespace Commands
	{
		struct AddTempoChange : Undo::Command
		{
			AddTempoChange(SortedTempoMap* tempoMap, TempoChange newValue) : TempoMap(tempoMap), NewValue(newValue) {}

			void Undo() override { TempoMap->TempoRemoveAtBeat(NewValue.Beat); TempoMap->RebuildAccelerationStructure(); }
			void Redo() override { TempoMap->TempoInsertOrUpdate(NewValue); TempoMap->RebuildAccelerationStructure(); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Tempo Change" }; }

			SortedTempoMap* TempoMap;
			TempoChange NewValue;
		};

		struct RemoveTempoChange : Undo::Command
		{
			RemoveTempoChange(SortedTempoMap* tempoMap, Beat beat) : TempoMap(tempoMap), OldValue(*TempoMap->TempoTryFindLastAtBeat(beat)) { assert(OldValue.Beat == beat); }

			void Undo() override { TempoMap->TempoInsertOrUpdate(OldValue); TempoMap->RebuildAccelerationStructure(); }
			void Redo() override { TempoMap->TempoRemoveAtBeat(OldValue.Beat); TempoMap->RebuildAccelerationStructure(); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Tempo Change" }; }

			SortedTempoMap* TempoMap;
			TempoChange OldValue;
		};

		struct UpdateTempoChange : Undo::Command
		{
			UpdateTempoChange(SortedTempoMap* tempoMap, TempoChange newValue) : TempoMap(tempoMap), NewValue(newValue), OldValue(*TempoMap->TempoTryFindLastAtBeat(newValue.Beat)) { assert(newValue.Beat == OldValue.Beat); }

			void Undo() override { TempoMap->TempoInsertOrUpdate(OldValue); TempoMap->RebuildAccelerationStructure(); }
			void Redo() override { TempoMap->TempoInsertOrUpdate(NewValue); TempoMap->RebuildAccelerationStructure(); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->TempoMap != TempoMap || other->NewValue.Beat != NewValue.Beat)
					return Undo::MergeResult::Failed;

				NewValue = other->NewValue;
				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Update Tempo Change" }; }

			SortedTempoMap* TempoMap;
			TempoChange NewValue, OldValue;
		};

		struct AddTimeSignatureChange : Undo::Command
		{
			AddTimeSignatureChange(SortedTempoMap* tempoMap, TimeSignatureChange newValue) : TempoMap(tempoMap), NewValue(newValue) {}

			void Undo() override { TempoMap->SignatureRemoveAtBeat(NewValue.Beat); }
			void Redo() override { TempoMap->SignatureInsertOrUpdate(NewValue); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Time Signature Change" }; }

			SortedTempoMap* TempoMap;
			TimeSignatureChange NewValue;
		};

		struct RemoveTimeSignatureChange : Undo::Command
		{
			RemoveTimeSignatureChange(SortedTempoMap* tempoMap, Beat beat) : TempoMap(tempoMap), OldValue(*TempoMap->SignatureTryFindLastAtBeat(beat)) { assert(OldValue.Beat == beat); }

			void Undo() override { TempoMap->SignatureInsertOrUpdate(OldValue); }
			void Redo() override { TempoMap->SignatureRemoveAtBeat(OldValue.Beat); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Time Signature Change" }; }

			SortedTempoMap* TempoMap;
			TimeSignatureChange OldValue;
		};

		struct UpdateTimeSignatureChange : Undo::Command
		{
			UpdateTimeSignatureChange(SortedTempoMap* tempoMap, TimeSignatureChange newValue) : TempoMap(tempoMap), NewValue(newValue), OldValue(*TempoMap->SignatureTryFindLastAtBeat(newValue.Beat)) { assert(newValue.Beat == OldValue.Beat); }

			void Undo() override { TempoMap->SignatureInsertOrUpdate(OldValue); }
			void Redo() override { TempoMap->SignatureInsertOrUpdate(NewValue); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->TempoMap != TempoMap || other->NewValue.Beat != NewValue.Beat)
					return Undo::MergeResult::Failed;

				NewValue = other->NewValue;
				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Update Time Signature Change" }; }

			SortedTempoMap* TempoMap;
			TimeSignatureChange NewValue, OldValue;
		};
	}

	// NOTE: Chart change commands
	namespace Commands
	{
		struct AddScrollChange : Undo::Command
		{
			AddScrollChange(SortedScrollChangesList* scrollChanges, ScrollChange newValue) : ScrollChanges(scrollChanges), NewValue(newValue) {}

			void Undo() override { ScrollChanges->RemoveAtBeat(NewValue.BeatTime); }
			void Redo() override { ScrollChanges->InsertOrUpdate(NewValue); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Scroll Change" }; }

			SortedScrollChangesList* ScrollChanges;
			ScrollChange NewValue;
		};

		struct RemoveScrollChange : Undo::Command
		{
			RemoveScrollChange(SortedScrollChangesList* scrollChanges, Beat beat) : ScrollChanges(scrollChanges), OldValue(*ScrollChanges->TryFindExactAtBeat(beat)) { assert(OldValue.BeatTime == beat); }

			void Undo() override { ScrollChanges->InsertOrUpdate(OldValue); }
			void Redo() override { ScrollChanges->RemoveAtBeat(OldValue.BeatTime); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Scroll Change" }; }

			SortedScrollChangesList* ScrollChanges;
			ScrollChange OldValue;
		};

		struct UpdateScrollChange : Undo::Command
		{
			UpdateScrollChange(SortedScrollChangesList* scrollChanges, ScrollChange newValue) : ScrollChanges(scrollChanges), NewValue(newValue), OldValue(*ScrollChanges->TryFindExactAtBeat(newValue.BeatTime)) { assert(newValue.BeatTime == OldValue.BeatTime); }

			void Undo() override { ScrollChanges->InsertOrUpdate(OldValue); }
			void Redo() override { ScrollChanges->InsertOrUpdate(NewValue); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->ScrollChanges != ScrollChanges || other->NewValue.BeatTime != NewValue.BeatTime)
					return Undo::MergeResult::Failed;

				NewValue = other->NewValue;
				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Update Scroll Change" }; }

			SortedScrollChangesList* ScrollChanges;
			ScrollChange NewValue, OldValue;
		};

		struct AddBarLineChange : Undo::Command
		{
			AddBarLineChange(SortedBarLineChangesList* barLineChanges, BarLineChange newValue) : BarLineChanges(barLineChanges), NewValue(newValue) {}

			void Undo() override { BarLineChanges->RemoveAtBeat(NewValue.BeatTime); }
			void Redo() override { BarLineChanges->InsertOrUpdate(NewValue); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Bar Line Change" }; }

			SortedBarLineChangesList* BarLineChanges;
			BarLineChange NewValue;
		};

		struct RemoveBarLineChange : Undo::Command
		{
			RemoveBarLineChange(SortedBarLineChangesList* barLineChanges, Beat beat) : BarLineChanges(barLineChanges), OldValue(*BarLineChanges->TryFindExactAtBeat(beat)) { assert(OldValue.BeatTime == beat); }

			void Undo() override { BarLineChanges->InsertOrUpdate(OldValue); }
			void Redo() override { BarLineChanges->RemoveAtBeat(OldValue.BeatTime); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Bar Line Change" }; }

			SortedBarLineChangesList* BarLineChanges;
			BarLineChange OldValue;
		};

		struct UpdateBarLineChange : Undo::Command
		{
			UpdateBarLineChange(SortedBarLineChangesList* barLineChanges, BarLineChange newValue) : BarLineChanges(barLineChanges), NewValue(newValue), OldValue(*BarLineChanges->TryFindExactAtBeat(newValue.BeatTime)) { assert(newValue.BeatTime == OldValue.BeatTime); }

			void Undo() override { BarLineChanges->InsertOrUpdate(OldValue); }
			void Redo() override { BarLineChanges->InsertOrUpdate(NewValue); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override
			{
#if 1 // NOTE: Merging here doesn't really make much sense..?
				return Undo::MergeResult::Failed;
#else
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->BarLineChanges != BarLineChanges || other->NewValue.BeatTime != NewValue.BeatTime)
					return Undo::MergeResult::Failed;

				NewValue = other->NewValue;
				return Undo::MergeResult::ValueUpdated;
#endif
			}

			Undo::CommandInfo GetInfo() const override { return { "Update Bar Line Change" }; }

			SortedBarLineChangesList* BarLineChanges;
			BarLineChange NewValue, OldValue;
		};

		struct AddGoGoRange : Undo::Command
		{
			// NOTE: Creating a full copy of the new/old state for now because it's easy and there typically are only a very few number of GoGoRanges
			AddGoGoRange(SortedGoGoRangesList* gogoRanges, SortedGoGoRangesList newValues) : GoGoRanges(gogoRanges), NewValues(std::move(newValues)), OldValues(*gogoRanges) {}

			void Undo() override { *GoGoRanges = OldValues; }
			void Redo() override { *GoGoRanges = NewValues; }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Go-Go Range" }; }

			SortedGoGoRangesList* GoGoRanges;
			SortedGoGoRangesList NewValues, OldValues;
		};

		struct RemoveGoGoRange : Undo::Command
		{
			RemoveGoGoRange(SortedGoGoRangesList* gogoRanges, Beat beat) : GoGoRanges(gogoRanges), OldValue(*gogoRanges->TryFindExactAtBeat(beat)) { assert(OldValue.BeatTime == beat); }

			void Undo() override { GoGoRanges->InsertOrUpdate(OldValue); }
			void Redo() override { GoGoRanges->RemoveAtBeat(OldValue.BeatTime); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Go-Go Range" }; }

			SortedGoGoRangesList* GoGoRanges;
			GoGoRange OldValue;
		};

		struct AddLyricChange : Undo::Command
		{
			AddLyricChange(SortedLyricsList* lyrics, LyricChange newValue) : Lyrics(lyrics), NewValue(newValue) {}

			void Undo() override { Lyrics->RemoveAtBeat(NewValue.BeatTime); }
			void Redo() override { Lyrics->InsertOrUpdate(NewValue); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Lyric Change" }; }

			SortedLyricsList* Lyrics;
			LyricChange NewValue;
		};

		struct RemoveLyricChange : Undo::Command
		{
			RemoveLyricChange(SortedLyricsList* lyrics, Beat beat) : Lyrics(lyrics), OldValue(*Lyrics->TryFindExactAtBeat(beat)) { assert(OldValue.BeatTime == beat); }

			void Undo() override { Lyrics->InsertOrUpdate(OldValue); }
			void Redo() override { Lyrics->RemoveAtBeat(OldValue.BeatTime); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Lyric Change" }; }

			SortedLyricsList* Lyrics;
			LyricChange OldValue;
		};

		struct UpdateLyricChange : Undo::Command
		{
			UpdateLyricChange(SortedLyricsList* lyrics, LyricChange newValue) : Lyrics(lyrics), NewValue(newValue), OldValue(*Lyrics->TryFindExactAtBeat(newValue.BeatTime)) { assert(newValue.BeatTime == OldValue.BeatTime); }

			void Undo() override { Lyrics->InsertOrUpdate(OldValue); }
			void Redo() override { Lyrics->InsertOrUpdate(NewValue); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Lyrics != Lyrics || other->NewValue.BeatTime != NewValue.BeatTime)
					return Undo::MergeResult::Failed;

				NewValue = other->NewValue;
				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Update Lyric Change" }; }

			SortedLyricsList* Lyrics;
			LyricChange NewValue, OldValue;
		};
	}

	// NOTE: Note commands
	namespace Commands
	{
		struct AddSingleNote : Undo::Command
		{
			AddSingleNote(SortedNotesList* notes, Note newNote) : Notes(notes), NewNote(newNote) {}

			void Undo() override { Notes->RemoveID(NewNote.StableID); }
			void Redo() override { NewNote.StableID = Notes->Add(NewNote); }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Note" }; }

			SortedNotesList* Notes;
			Note NewNote;
		};

		struct AddMultipleNotes : Undo::Command
		{
			AddMultipleNotes(SortedNotesList* notes, std::vector<Note> newNotes) : Notes(notes), NewNotes(std::move(newNotes)) {}

			void Undo() override
			{
				for (const Note& note : NewNotes)
					Notes->RemoveID(note.StableID);
			}

			void Redo() override
			{
				for (Note& note : NewNotes)
					note.StableID = Notes->Add(note);
			}

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Notes" }; }

			SortedNotesList* Notes;
			std::vector<Note> NewNotes;
		};

		struct RemoveSingleNote : Undo::Command
		{
			RemoveSingleNote(SortedNotesList* notes, Note oldNote) : Notes(notes), OldNote(oldNote) {}

			void Undo() override { OldNote.StableID = Notes->Add(OldNote); }
			void Redo() override { Notes->RemoveID(OldNote.StableID); }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Note" }; }

			SortedNotesList* Notes;
			Note OldNote;
		};

		struct RemoveMultipleNotes : Undo::Command
		{
			RemoveMultipleNotes(SortedNotesList* notes, std::vector<Note> oldNotes) : Notes(notes), OldNotes(std::move(oldNotes)) {}

			void Undo() override
			{
				for (Note& note : OldNotes)
					note.StableID = Notes->Add(note);
			}

			void Redo() override
			{
				for (Note& note : OldNotes)
					Notes->RemoveID(note.StableID);
			}

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Notes" }; }

			SortedNotesList* Notes;
			std::vector<Note> OldNotes;
		};

		struct AddSingleLongNote : Undo::Command
		{
			AddSingleLongNote(SortedNotesList* notes, Note newNote, std::vector<Note> notesToRemove) : Notes(notes), NewNote(newNote), NotesToRemove(notes, std::move(notesToRemove)) {}

			void Undo() override { Notes->RemoveID(NewNote.StableID); NotesToRemove.Undo(); }
			void Redo() override { NotesToRemove.Redo(); NewNote.StableID = Notes->Add(NewNote); }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Long Note" }; }

			SortedNotesList* Notes;
			Note NewNote;
			RemoveMultipleNotes NotesToRemove;
		};

		struct AddMultipleNotes_Paste : AddMultipleNotes
		{
			using AddMultipleNotes::AddMultipleNotes;
			Undo::CommandInfo GetInfo() const override { return { "Paste Notes" }; }
		};

		struct RemoveMultipleNotes_Cut : RemoveMultipleNotes
		{
			using RemoveMultipleNotes::RemoveMultipleNotes;
			Undo::CommandInfo GetInfo() const override { return { "Cut Notes" }; }
		};

		struct ChangeSingleNoteType : Undo::Command
		{
			struct Data { StableID StableID; NoteType NewType, OldType; };

			ChangeSingleNoteType(SortedNotesList* notes, Data newData)
				: Notes(notes), NewData(std::move(newData))
			{
				NewData.OldType = Notes->TryFindID(NewData.StableID)->Type;
			}

			void Undo() override
			{
				Notes->TryFindID(NewData.StableID)->Type = NewData.OldType;
			}

			void Redo() override
			{
				Notes->TryFindID(NewData.StableID)->Type = NewData.NewType;
			}

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Notes != Notes)
					return Undo::MergeResult::Failed;

				if (NewData.StableID != other->NewData.StableID)
					return Undo::MergeResult::Failed;

				NewData.NewType = other->NewData.NewType;

				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Change Note Type" }; }

			SortedNotesList* Notes;
			Data NewData;
		};

		struct ChangeMultipleNoteTypes : Undo::Command
		{
			struct Data { StableID StableID; NoteType NewType, OldType; };

			ChangeMultipleNoteTypes(SortedNotesList* notes, std::vector<Data> newData)
				: Notes(notes), NewData(std::move(newData))
			{
				for (auto& newData : NewData)
					newData.OldType = Notes->TryFindID(newData.StableID)->Type;
			}

			void Undo() override
			{
				for (const auto& newData : NewData)
					Notes->TryFindID(newData.StableID)->Type = newData.OldType;
			}

			void Redo() override
			{
				for (const auto& newData : NewData)
					Notes->TryFindID(newData.StableID)->Type = newData.NewType;
			}

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Notes != Notes)
					return Undo::MergeResult::Failed;

				if (other->NewData.size() != NewData.size())
					return Undo::MergeResult::Failed;

				for (size_t i = 0; i < NewData.size(); i++)
				{
					if (NewData[i].StableID != other->NewData[i].StableID)
						return Undo::MergeResult::Failed;
				}

				for (size_t i = 0; i < NewData.size(); i++)
					NewData[i].NewType = other->NewData[i].NewType;

				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Change Note Types" }; }

			SortedNotesList* Notes;
			std::vector<Data> NewData;
		};

		struct ChangeMultipleNoteBeats : Undo::Command
		{
			struct Data { StableID StableID; Beat NewBeat, OldBeat; };

			ChangeMultipleNoteBeats(SortedNotesList* notes, std::vector<Data> data) : Notes(notes), NewData(std::move(data))
			{
				for (auto& newData : NewData)
					newData.OldBeat = Notes->TryFindID(newData.StableID)->BeatTime;
			}

			void Undo() override
			{
				for (const auto& newData : NewData)
					Notes->TryFindID(newData.StableID)->BeatTime = newData.OldBeat;
				// TODO: Assert sorted (?)
			}

			void Redo() override
			{
				for (const auto& newData : NewData)
					Notes->TryFindID(newData.StableID)->BeatTime = newData.NewBeat;
				// TODO: Assert sorted (?)
			}

			Undo::MergeResult TryMerge(Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Notes != Notes)
					return Undo::MergeResult::Failed;

				if (other->NewData.size() != NewData.size())
					return Undo::MergeResult::Failed;

				for (size_t i = 0; i < NewData.size(); i++)
				{
					if (NewData[i].StableID != other->NewData[i].StableID)
						return Undo::MergeResult::Failed;
				}

				for (size_t i = 0; i < NewData.size(); i++)
					NewData[i].NewBeat = other->NewData[i].NewBeat;

				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Change Note Beats" }; }

			SortedNotesList* Notes;
			std::vector<Data> NewData;
		};

		struct ChangeMultipleNoteBeats_MoveNotes : ChangeMultipleNoteBeats
		{
			using ChangeMultipleNoteBeats::ChangeMultipleNoteBeats;
			Undo::CommandInfo GetInfo() const override { return { "Move Notes" }; }
		};
	}
}

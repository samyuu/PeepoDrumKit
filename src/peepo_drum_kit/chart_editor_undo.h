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

			void Undo() override { TempoMap->Tempo.RemoveAtBeat(NewValue.Beat); TempoMap->RebuildAccelerationStructure(); }
			void Redo() override { TempoMap->Tempo.InsertOrUpdate(NewValue); TempoMap->RebuildAccelerationStructure(); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Tempo Change" }; }

			SortedTempoMap* TempoMap;
			TempoChange NewValue;
		};

		struct RemoveTempoChange : Undo::Command
		{
			RemoveTempoChange(SortedTempoMap* tempoMap, Beat beat) : TempoMap(tempoMap), OldValue(*TempoMap->Tempo.TryFindLastAtBeat(beat)) { assert(OldValue.Beat == beat); }

			void Undo() override { TempoMap->Tempo.InsertOrUpdate(OldValue); TempoMap->RebuildAccelerationStructure(); }
			void Redo() override { TempoMap->Tempo.RemoveAtBeat(OldValue.Beat); TempoMap->RebuildAccelerationStructure(); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Tempo Change" }; }

			SortedTempoMap* TempoMap;
			TempoChange OldValue;
		};

		struct UpdateTempoChange : Undo::Command
		{
			UpdateTempoChange(SortedTempoMap* tempoMap, TempoChange newValue) : TempoMap(tempoMap), NewValue(newValue), OldValue(*TempoMap->Tempo.TryFindLastAtBeat(newValue.Beat)) { assert(newValue.Beat == OldValue.Beat); }

			void Undo() override { TempoMap->Tempo.InsertOrUpdate(OldValue); TempoMap->RebuildAccelerationStructure(); }
			void Redo() override { TempoMap->Tempo.InsertOrUpdate(NewValue); TempoMap->RebuildAccelerationStructure(); }

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

			void Undo() override { TempoMap->Signature.RemoveAtBeat(NewValue.Beat); }
			void Redo() override { TempoMap->Signature.InsertOrUpdate(NewValue); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Time Signature Change" }; }

			SortedTempoMap* TempoMap;
			TimeSignatureChange NewValue;
		};

		struct RemoveTimeSignatureChange : Undo::Command
		{
			RemoveTimeSignatureChange(SortedTempoMap* tempoMap, Beat beat) : TempoMap(tempoMap), OldValue(*TempoMap->Signature.TryFindLastAtBeat(beat)) { assert(OldValue.Beat == beat); }

			void Undo() override { TempoMap->Signature.InsertOrUpdate(OldValue); }
			void Redo() override { TempoMap->Signature.RemoveAtBeat(OldValue.Beat); }

			Undo::MergeResult TryMerge(Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Time Signature Change" }; }

			SortedTempoMap* TempoMap;
			TimeSignatureChange OldValue;
		};

		struct UpdateTimeSignatureChange : Undo::Command
		{
			UpdateTimeSignatureChange(SortedTempoMap* tempoMap, TimeSignatureChange newValue) : TempoMap(tempoMap), NewValue(newValue), OldValue(*TempoMap->Signature.TryFindLastAtBeat(newValue.Beat)) { assert(newValue.Beat == OldValue.Beat); }

			void Undo() override { TempoMap->Signature.InsertOrUpdate(OldValue); }
			void Redo() override { TempoMap->Signature.InsertOrUpdate(NewValue); }

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

		struct AddMultipleScrollChanges : Undo::Command
		{
			AddMultipleScrollChanges(SortedScrollChangesList* scrollChanges, std::vector<ScrollChange> newValues) : ScrollChanges(scrollChanges), NewScrollChanges(std::move(newValues)) {}

			void Undo() override { for (const auto& scroll : NewScrollChanges) ScrollChanges->RemoveAtBeat(scroll.BeatTime); }
			void Redo() override { for (const auto& scroll : NewScrollChanges) ScrollChanges->InsertOrUpdate(scroll); }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Scroll Changes" }; }

			SortedScrollChangesList* ScrollChanges;
			std::vector<ScrollChange> NewScrollChanges;
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

		struct ReplaceAllLyricChanges : Undo::Command
		{
			ReplaceAllLyricChanges(SortedLyricsList* lyrics, SortedLyricsList newValue) : Lyrics(lyrics), NewValue(std::move(newValue)), OldValue(*lyrics) {}

			void Undo() override { *Lyrics = OldValue; }
			void Redo() override { *Lyrics = NewValue; }

			Undo::MergeResult TryMerge(Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Lyrics != Lyrics)
					return Undo::MergeResult::Failed;

				NewValue = other->NewValue;
				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Update All Lyrics" }; }

			SortedLyricsList* Lyrics;
			SortedLyricsList NewValue, OldValue;
		};
	}

	// NOTE: Note commands
	namespace Commands
	{
		struct AddSingleNote : Undo::Command
		{
			AddSingleNote(SortedNotesList* notes, Note newNote) : Notes(notes), NewNote(newNote) {}

			void Undo() override { Notes->RemoveAtBeat(NewNote.BeatTime); }
			void Redo() override { Notes->InsertOrUpdate(NewNote); }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Note" }; }

			SortedNotesList* Notes;
			Note NewNote;
		};

		struct AddMultipleNotes : Undo::Command
		{
			AddMultipleNotes(SortedNotesList* notes, std::vector<Note> newNotes) : Notes(notes), NewNotes(std::move(newNotes)) {}

			void Undo() override { for (const Note& note : NewNotes) Notes->RemoveAtBeat(note.BeatTime); }
			void Redo() override { for (const Note& note : NewNotes) Notes->InsertOrUpdate(note); }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Notes" }; }

			SortedNotesList* Notes;
			std::vector<Note> NewNotes;
		};

		struct RemoveSingleNote : Undo::Command
		{
			RemoveSingleNote(SortedNotesList* notes, Note oldNote) : Notes(notes), OldNote(oldNote) {}

			void Undo() override { Notes->InsertOrUpdate(OldNote); }
			void Redo() override { Notes->RemoveAtBeat(OldNote.BeatTime); }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Note" }; }

			SortedNotesList* Notes;
			Note OldNote;
		};

		struct RemoveMultipleNotes : Undo::Command
		{
			RemoveMultipleNotes(SortedNotesList* notes, std::vector<Note> oldNotes) : Notes(notes), OldNotes(std::move(oldNotes)) {}

			void Undo() override { for (Note& note : OldNotes) Notes->InsertOrUpdate(note); }
			void Redo() override { for (const Note& note : OldNotes) Notes->RemoveAtBeat(note.BeatTime); }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Notes" }; }

			SortedNotesList* Notes;
			std::vector<Note> OldNotes;
		};

		struct AddSingleLongNote : Undo::Command
		{
			AddSingleLongNote(SortedNotesList* notes, Note newNote, std::vector<Note> notesToRemove) : Notes(notes), NewNote(newNote), NotesToRemove(notes, std::move(notesToRemove)) {}

			void Undo() override { Notes->RemoveAtBeat(NewNote.BeatTime); NotesToRemove.Undo(); }
			void Redo() override { NotesToRemove.Redo(); Notes->InsertOrUpdate(NewNote); }

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
			struct Data { size_t Index; NoteType NewType, OldType; };

			ChangeSingleNoteType(SortedNotesList* notes, Data newData) : Notes(notes), NewData(std::move(newData)) { NewData.OldType = (*Notes)[NewData.Index].Type; }

			void Undo() override { (*Notes)[NewData.Index].Type = NewData.OldType; }
			void Redo() override { (*Notes)[NewData.Index].Type = NewData.NewType; }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Notes != Notes)
					return Undo::MergeResult::Failed;

				if (NewData.Index != other->NewData.Index)
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
			struct Data { size_t Index; NoteType NewType, OldType; };

			ChangeMultipleNoteTypes(SortedNotesList* notes, std::vector<Data> newData)
				: Notes(notes), NewData(std::move(newData))
			{
				for (auto& newData : NewData)
					newData.OldType = (*Notes)[newData.Index].Type;
			}

			void Undo() override
			{
				for (const auto& newData : NewData)
					(*Notes)[newData.Index].Type = newData.OldType;
			}

			void Redo() override
			{
				for (const auto& newData : NewData)
					(*Notes)[newData.Index].Type = newData.NewType;
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
					if (NewData[i].Index != other->NewData[i].Index)
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

		struct ChangeMultipleNoteTypes_FlipTypes : ChangeMultipleNoteTypes
		{
			using ChangeMultipleNoteTypes::ChangeMultipleNoteTypes;
			Undo::CommandInfo GetInfo() const override { return { "Flip Notes" }; }
		};

		struct ChangeMultipleNoteTypes_ToggleSizes : ChangeMultipleNoteTypes
		{
			using ChangeMultipleNoteTypes::ChangeMultipleNoteTypes;
			Undo::CommandInfo GetInfo() const override { return { "Toggle Note Sizes" }; }
		};

		struct ChangeMultipleNoteBeats : Undo::Command
		{
			struct Data { size_t Index; Beat NewBeat, OldBeat; };

			ChangeMultipleNoteBeats(SortedNotesList* notes, std::vector<Data> data) : Notes(notes), NewData(std::move(data))
			{
				for (auto& newData : NewData)
					newData.OldBeat = (*Notes)[newData.Index].BeatTime;
			}

			void Undo() override
			{
				for (const auto& newData : NewData)
					(*Notes)[newData.Index].BeatTime = newData.OldBeat;
				// TODO: Assert sorted (?)
			}

			void Redo() override
			{
				for (const auto& newData : NewData)
					(*Notes)[newData.Index].BeatTime = newData.NewBeat;
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
					if (NewData[i].Index != other->NewData[i].Index)
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

	// NOTE: Generic chart commands
	namespace Commands
	{
		struct AddMultipleGenericItems : Undo::Command
		{
			AddMultipleGenericItems(ChartCourse* course, std::vector<GenericListStructWithType> newData) : Course(course), NewData(std::move(newData)), UpdateTempoMap(false)
			{
				for (const auto& data : NewData)
				{
					if (data.List == GenericList::TempoChanges)
						UpdateTempoMap = true;
				}
			}

			void Undo() override
			{
				for (const auto& data : NewData)
					TryRemoveGenericStruct(*Course, data.List, data.Value);
				if (UpdateTempoMap)
					Course->TempoMap.RebuildAccelerationStructure();
			}

			void Redo() override
			{
				for (const auto& data : NewData)
					TryAddGenericStruct(*Course, data.List, data.Value);
				if (UpdateTempoMap)
					Course->TempoMap.RebuildAccelerationStructure();
			}

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Add Items" }; }

			ChartCourse* Course;
			std::vector<GenericListStructWithType> NewData;
			b8 UpdateTempoMap;
		};

		struct RemoveMultipleGenericItems : Undo::Command
		{
			RemoveMultipleGenericItems(ChartCourse* course, std::vector<GenericListStructWithType> oldData) : Course(course), OldData(std::move(oldData)), UpdateTempoMap(false)
			{
				for (const auto& data : OldData)
				{
					if (data.List == GenericList::TempoChanges)
						UpdateTempoMap = true;
				}
			}

			void Undo() override
			{
				for (const auto& data : OldData)
					TryAddGenericStruct(*Course, data.List, data.Value);
				if (UpdateTempoMap)
					Course->TempoMap.RebuildAccelerationStructure();
			}

			void Redo() override
			{
				for (const auto& data : OldData)
					TryRemoveGenericStruct(*Course, data.List, data.Value);
				if (UpdateTempoMap)
					Course->TempoMap.RebuildAccelerationStructure();
			}

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove Items" }; }

			ChartCourse* Course;
			std::vector<GenericListStructWithType> OldData;
			b8 UpdateTempoMap;
		};

		struct AddMultipleGenericItems_Paste : AddMultipleGenericItems
		{
			using AddMultipleGenericItems::AddMultipleGenericItems;
			Undo::CommandInfo GetInfo() const override { return { "Paste Items" }; }
		};

		struct RemoveMultipleGenericItems_Cut : RemoveMultipleGenericItems
		{
			using RemoveMultipleGenericItems::RemoveMultipleGenericItems;
			Undo::CommandInfo GetInfo() const override { return { "Cut Items" }; }
		};

		struct ChangeMultipleGenericProperties : Undo::Command
		{
			// TODO: Separate string vector for cstr ownership stuff
			struct Data
			{
				size_t Index;
				GenericList List;
				GenericMember Member;
				GenericMemberUnion NewValue, OldValue;
			};

			ChangeMultipleGenericProperties(ChartCourse* course, std::vector<Data> newData)
				: Course(course), NewData(std::move(newData)), UpdateTempoMap(false)
			{
				for (auto& data : NewData)
				{
					const b8 success = TryGetGeneric(*Course, data.List, data.Index, data.Member, data.OldValue);
					assert(success);
					if (data.List == GenericList::TempoChanges)
						UpdateTempoMap = true;
				}
			}

			void Undo() override
			{
				for (const auto& newData : NewData)
					TrySetGeneric(*Course, newData.List, newData.Index, newData.Member, newData.OldValue);
				if (UpdateTempoMap)
					Course->TempoMap.RebuildAccelerationStructure();
			}

			void Redo() override
			{
				for (const auto& newData : NewData)
					TrySetGeneric(*Course, newData.List, newData.Index, newData.Member, newData.NewValue);
				if (UpdateTempoMap)
					Course->TempoMap.RebuildAccelerationStructure();
			}

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override
			{
				auto* other = static_cast<decltype(this)>(&commandToMerge);
				if (other->Course != Course)
					return Undo::MergeResult::Failed;

				if (other->NewData.size() != NewData.size())
					return Undo::MergeResult::Failed;

				for (size_t i = 0; i < NewData.size(); i++)
				{
					const auto& data = NewData[i];
					const auto& otherData = other->NewData[i];
					if (data.Index != otherData.Index || data.List != otherData.List || data.Member != otherData.Member)
						return Undo::MergeResult::Failed;
				}

				for (size_t i = 0; i < NewData.size(); i++)
					NewData[i].NewValue = other->NewData[i].NewValue;

				return Undo::MergeResult::ValueUpdated;
			}

			Undo::CommandInfo GetInfo() const override { return { "Change Properties" }; }

			ChartCourse* Course;
			std::vector<Data> NewData;
			b8 UpdateTempoMap;
		};

		struct ChangeMultipleGenericProperties_MoveItems : ChangeMultipleGenericProperties
		{
			using ChangeMultipleGenericProperties::ChangeMultipleGenericProperties;
			Undo::CommandInfo GetInfo() const override { return { "Move Items" }; }
		};

		struct RemoveThenAddMultipleGenericItems : Undo::Command
		{
			RemoveThenAddMultipleGenericItems(ChartCourse* course, std::vector<GenericListStructWithType> removeData, std::vector<GenericListStructWithType> addData)
				: RemoveCommand(course, std::move(removeData)), AddCommand(course, std::move(addData))
			{
			}

			void Undo() override { AddCommand.Undo(); RemoveCommand.Undo(); }
			void Redo() override { RemoveCommand.Redo(); AddCommand.Redo(); }

			Undo::MergeResult TryMerge(Undo::Command& commandToMerge) override { return Undo::MergeResult::Failed; }
			Undo::CommandInfo GetInfo() const override { return { "Remove and Add Items" }; }

			RemoveMultipleGenericItems RemoveCommand;
			AddMultipleGenericItems AddCommand;
		};

		struct RemoveThenAddMultipleGenericItems_ExpandItems : RemoveThenAddMultipleGenericItems
		{
			using RemoveThenAddMultipleGenericItems::RemoveThenAddMultipleGenericItems;
			Undo::CommandInfo GetInfo() const override { return { "Expand Items" }; }
		};

		struct RemoveThenAddMultipleGenericItems_CompressItems : RemoveThenAddMultipleGenericItems
		{
			using RemoveThenAddMultipleGenericItems::RemoveThenAddMultipleGenericItems;
			Undo::CommandInfo GetInfo() const override { return { "Compress Items" }; }
		};
	}
}

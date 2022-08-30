#include "test_gui_audio.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	void AudioTestWindow::DrawGui(b8* isOpen)
	{
		if (Gui::Begin("Audio Test", isOpen, ImGuiWindowFlags_None))
		{
			const ImVec2 originalFramePadding = Gui::GetStyle().FramePadding;
			Gui::PushStyleVar(ImGuiStyleVar_FramePadding, GuiScale(vec2(10.0f, 5.0f)));
			Gui::PushStyleColor(ImGuiCol_TabHovered, Gui::GetStyleColorVec4(ImGuiCol_HeaderActive));
			Gui::PushStyleColor(ImGuiCol_TabActive, Gui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
			if (Gui::BeginTabBar("AudioTestWindowTabBar", ImGuiTabBarFlags_None))
			{
				auto beginEndTabItem = [&](cstr label, auto func)
				{
					if (Gui::BeginTabItem(label)) { Gui::PushStyleVar(ImGuiStyleVar_FramePadding, originalFramePadding); func(); Gui::PopStyleVar(); Gui::EndTabItem(); }
				};
				beginEndTabItem("Audio Engine", [this] { AudioEngineTabContent(); });
				beginEndTabItem("Active Voices", [this] { ActiveVoicesTabContent(); });
				beginEndTabItem("Loaded Sources", [this] { LoadedSourcesTabContent(); });
				Gui::EndTabBar();
			}
			Gui::PopStyleColor(2);
			Gui::PopStyleVar();
		}
		Gui::End();
	}

	void AudioTestWindow::AudioEngineTabContent()
	{
		Gui::Property::Table(ImGuiTableFlags_BordersInner | ImGuiTableFlags_ScrollY, [&]
		{
			static constexpr ImVec4 greenColor = ImVec4(0.470f, 0.948f, 0.243f, 1.0f), redColor = ImVec4(0.964f, 0.298f, 0.229f, 1.0f);

			Gui::Property::PropertyTextValueFunc("Master Volume", [&]
			{
				Gui::SetNextItemWidth(-1.0f);
				f32 v = ToPercent(Audio::Engine.GetMasterVolume());
				if (Gui::SliderFloat("##MasterVolumeSlider", &v, ToPercent(Audio::Engine.MinVolume), ToPercent(Audio::Engine.MaxVolume), "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
					Audio::Engine.SetMasterVolume(FromPercent(v));
			});

			Gui::Property::PropertyTextValueFunc("Device Control", [&]
			{
				if (Gui::Button("Open Start Stream", vec2(Gui::GetContentRegionAvail().x, 0.0f)))
					Audio::Engine.OpenStartStream();
				if (Gui::Button("Stop Close Stream", vec2(Gui::GetContentRegionAvail().x, 0.0f)))
					Audio::Engine.StopCloseStream();
				if (Gui::Button("Ensure Stream Running", vec2(Gui::GetContentRegionAvail().x, 0.0f)))
					Audio::Engine.EnsureStreamRunning();
			});

			Gui::Property::PropertyTextValueFunc("Device State", [&]
			{
				if (Audio::Engine.GetIsStreamOpenRunning())
					Gui::TextColored(greenColor, "Open");
				else
					Gui::TextColored(redColor, "Closed");
			});

			Gui::Property::PropertyTextValueFunc("Audio Backend", [&]
			{
				Gui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (auto v = Audio::Engine.GetBackend(); Gui::ComboEnum("##Backend", &v, Audio::BackendNames))
					Audio::Engine.SetBackend(v);
			});

			Gui::Property::PropertyTextValueFunc("Channel Mixing Behavior", [&]
			{
				// HACK: Not actually atomic... although on x86 this totally works just fine regardless (TM) ((fuck UB))
				Gui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				Gui::ComboEnum("##MixingBehavior", &Audio::Engine.GetChannelMixer().MixingBehavior, Audio::ChannelMixingBehaviorNames);
			});

			Gui::Property::PropertyTreeNodeValueFunc("Engine Output", ImGuiTreeNodeFlags_DefaultOpen, [&]
			{
				Gui::Property::PropertyTextValueFunc("Channel Count", [&]
				{
					Gui::Text("%u Channel(s)", Audio::Engine.OutputChannelCount);
				});

				Gui::Property::PropertyTextValueFunc("Sample Rate", [&]
				{
					Gui::Text("%u Hz", Audio::Engine.OutputSampleRate);
				});

				Gui::Property::PropertyTextValueFunc("Callback Frequency", [&]
				{
					Gui::TextColored(Audio::Engine.GetIsStreamOpenRunning() ? greenColor : Gui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%.3f ms", Audio::Engine.GetCallbackFrequency().ToMS());
				});

				Gui::Property::PropertyTextValueFunc("Buffer Size", [&]
				{
					Gui::BeginDisabled();
					u32 currentBufferFrameCount = Audio::Engine.GetBufferFrameSize();
					Gui::SetNextItemWidth(-1.0f);
					Gui::InputScalar("##CurrentBufferSize", ImGuiDataType_U32, &currentBufferFrameCount, PtrArg<u32>(8), PtrArg<u32>(64), "%u Frames (Current)");
					Gui::EndDisabled();

					Gui::SetNextItemWidth(-1.0f);
					Gui::InputScalar("##NewBufferSize", ImGuiDataType_U32, &newBufferFrameCount, PtrArg<u32>(8), PtrArg<u32>(64), "%u Frames (Request)");
					newBufferFrameCount = Clamp(newBufferFrameCount, Audio::Engine.MinBufferFrameCount, Audio::Engine.MaxBufferFrameCount);

					if (Gui::Button("Request Resize", vec2(Gui::GetContentRegionAvail().x, 0.0f)))
						Audio::Engine.SetBufferFrameSize(newBufferFrameCount);
				});

				Gui::Property::PropertyTextValueFunc("Render Performance", [&]
				{
					const auto durations = Audio::Engine.DebugGetRenderPerformanceHistory();
					f32 durationsMS[durations.size()];
					for (size_t i = 0; i < durations.size(); i++) durationsMS[i] = durations[i].ToMS_F32();

					f32 totalBufferProcessTimeMS = 0.0f;
					for (const f32 v : durationsMS)totalBufferProcessTimeMS += v;
					f32 averageProcessTimeMS = (totalBufferProcessTimeMS / static_cast<f32>(ArrayCountI32(durationsMS)));

					char overlayTextBuffer[32];
					sprintf_s(overlayTextBuffer, "Average: %.6f ms", averageProcessTimeMS);

					Gui::PlotLines("##CallbackProcessDuration", durationsMS, ArrayCountI32(durationsMS), 0, overlayTextBuffer, FLT_MAX, FLT_MAX, vec2(Gui::GetContentRegionAvail().x, 32.0f));
				});

				Gui::Property::PropertyTextValueFunc("Rendered Samples", [&]
				{
					Gui::PushStyleColor(ImGuiCol_PlotLines, Gui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
					Gui::PushStyleColor(ImGuiCol_PlotLinesHovered, Gui::GetStyleColorVec4(ImGuiCol_PlotHistogramHovered));

					const auto lastPlayedSamples = Audio::Engine.DebugGetLastPlayedSamples();
					for (size_t channel = 0; channel < lastPlayedSamples.size(); channel++)
					{
						f32 normalizedSamples[lastPlayedSamples[0].size()];
						for (size_t sample = 0; sample < ArrayCount(normalizedSamples); sample++)
							normalizedSamples[sample] = Audio::ConvertSampleI16ToF32(lastPlayedSamples[channel][sample]);

						char plotName[32]; sprintf_s(plotName, "Channel [%zu]", channel);
						Gui::PushID(static_cast<int>(channel));
						Gui::PlotLines("##ChannelOutputPlot", normalizedSamples, ArrayCountI32(normalizedSamples), 0, plotName, -1.0f, 1.0f, vec2(Gui::GetContentRegionAvail().x, 32.0f));
						Gui::PopID();
					}

					Gui::PopStyleColor(2);
				});
			});
		});
	}

	void AudioTestWindow::ActiveVoicesTabContent()
	{
		static constexpr cstr voiceTableFields[] = { "Name", "Position", "Smooth", "Duration", "Volume", "Speed", "Voice", "Source", "Flags", };
		if (Gui::BeginTable("VoicesTable", ArrayCountI32(voiceTableFields), ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail()))
		{
			Gui::TableSetupScrollFreeze(0, 1);
			for (cstr name : voiceTableFields)
				Gui::TableSetupColumn(name, ImGuiTableColumnFlags_None);
			Gui::TableHeadersRow();

			const Audio::AudioEngine::DebugVoicesArray allActiveVoices = Audio::Engine.DebugGetAllActiveVoices();
			for (size_t i = 0; i < allActiveVoices.Count; i++)
			{
				const Audio::Voice voiceIt = allActiveVoices.Slots[i];
				const b8 voiceIsPlaying = voiceIt.GetIsPlaying();
				if (!voiceIsPlaying) Gui::PushStyleColor(ImGuiCol_Text, Gui::GetStyleColorVec4(ImGuiCol_TextDisabled));

				Gui::TableNextRow();
				Gui::TableNextColumn(); Gui::TextUnformatted(voiceIt.GetName());
				Gui::TableNextColumn(); Gui::TextUnformatted(voiceIt.GetPosition().ToString().Data);
				Gui::TableNextColumn(); Gui::TextUnformatted(voiceIt.GetPositionSmooth().ToString().Data);
				Gui::TableNextColumn(); Gui::TextUnformatted(voiceIt.GetSourceDuration().ToString().Data);
				Gui::TableNextColumn(); Gui::Text("%.0f%%", ToPercent(voiceIt.GetVolume()));
				Gui::TableNextColumn(); Gui::Text("%.0f%%", ToPercent(voiceIt.GetPlaybackSpeed()));
				Gui::TableNextColumn(); Gui::Text("0x%04X", static_cast<Audio::HandleBaseType>(voiceIt.Handle));
				Gui::TableNextColumn(); Gui::Text("0x%04X", static_cast<Audio::HandleBaseType>(voiceIt.GetSource()));
				static_assert(sizeof(Audio::HandleBaseType) == 2, "TODO: Update format strings");

				Gui::TableNextColumn();
				if (voiceIt.GetIsLooping()) voiceFlagsBuffer += "Loop | ";
				if (voiceIt.GetPlayPastEnd()) voiceFlagsBuffer += "PlayPastEnd | ";
				if (voiceIt.GetRemoveOnEnd()) voiceFlagsBuffer += "RemoveOnEnd | ";
				if (voiceIt.GetPauseOnEnd()) voiceFlagsBuffer += "PauseOnEnd | ";
				if (voiceFlagsBuffer.empty())
					Gui::TextUnformatted("None");
				else
					Gui::TextUnformatted(Gui::StringViewStart(voiceFlagsBuffer), Gui::StringViewEnd(voiceFlagsBuffer) - (sizeof(' ') + sizeof('|') + sizeof(' ')));
				voiceFlagsBuffer.clear();

				if (!voiceIsPlaying) Gui::PopStyleColor();
			}
			Gui::EndTable();
		}
	}

	void AudioTestWindow::LoadedSourcesTabContent()
	{
		static constexpr cstr sourcesTableFields[] = { "Name", "Handle", "Base Volume", "Instances", "Channels", "Sample Rate", "Duration" };
		if (Gui::BeginTable("SourcesTable", ArrayCountI32(sourcesTableFields), ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail()))
		{
			Gui::TableSetupScrollFreeze(0, 1);
			for (cstr name : sourcesTableFields)
				Gui::TableSetupColumn(name, ImGuiTableColumnFlags_None);
			Gui::TableHeadersRow();

			const Audio::AudioEngine::DebugSourcesArray allLoadedSources = Audio::Engine.DebugGetAllLoadedSources();
			for (size_t i = 0; i < allLoadedSources.Count; i++)
			{
				const Audio::SourceHandle sourceIt = allLoadedSources.Slots[i];
				const Audio::PCMSampleBuffer* sourceItSampleBuffer = Audio::Engine.GetSourceSampleBufferView(sourceIt);
				const std::string_view sourceItName = Audio::Engine.GetSourceName(sourceIt);
				const f32 sourceItBaseVolume = Audio::Engine.GetSourceBaseVolume(sourceIt);
				const Time sourceItDuration = ((sourceItSampleBuffer != nullptr) ? Audio::FramesToTime(sourceItSampleBuffer->FrameCount, sourceItSampleBuffer->SampleRate) : Time::Zero());
				i32 sourceItInstanceCount = Audio::Engine.DebugGetSourceVoiceInstanceCount(sourceIt);
				if (sourcePreviewVoice.GetSource() == sourceIt)
					sourceItInstanceCount--;

				Gui::PushID(allLoadedSources.Slots + i);
				b8 sourceIsPreviewing = (sourcePreviewVoice.GetIsPlaying() && sourcePreviewVoice.GetSource() == sourceIt);
				const b8 pushTextColor = !sourceIsPreviewing;
				if (pushTextColor) Gui::PushStyleColor(ImGuiCol_Text, Gui::GetStyleColorVec4(ImGuiCol_TextDisabled));

				Gui::TableNextRow();
				Gui::TableNextColumn();
				{
					const b8 selectableClicked = Gui::Selectable("##SourcePreview", sourceIsPreviewing, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
					const b8 selectableHovered = Gui::IsItemHovered();
					const b8 selectableRightClicked = (selectableHovered && Gui::IsMouseClicked(ImGuiMouseButton_Right));
					const Rect selectableRect = Gui::GetItemRect();
					const f32 nameColumnWidth = ClampBot(1.0f, Gui::GetContentRegionAvail().x + (Gui::GetStyle().CellPadding.x * 2.0f));

					if (selectableClicked) { sourceIsPreviewing ? StopSourcePreview() : StartSourcePreview(sourceIt); sourceIsPreviewing ^= true; }
					if (selectableRightClicked) { StartSourcePreview(sourceIt); sourceIsPreviewing = true; }

					if (sourceIsPreviewing)
					{
						if (selectableRightClicked)
						{
							const f32 hoverProgress = (Gui::GetMousePos().x - selectableRect.TL.x) / nameColumnWidth;
							if (hoverProgress >= 0.0f && hoverProgress < 1.0f)
								sourcePreviewVoice.SetPosition((sourcePreviewVoice.GetSourceDuration() * hoverProgress));
						}

						const f32 previewProgress = Clamp(static_cast<f32>(sourcePreviewVoice.GetPosition() / sourcePreviewVoice.GetSourceDuration()), 0.0f, 1.0f);
						Gui::GetWindowDrawList()->AddRectFilled(
							selectableRect.TL,
							vec2(selectableRect.TL.x + (nameColumnWidth * previewProgress), selectableRect.BR.y),
							Gui::GetColorU32(selectableHovered ? ImGuiCol_PlotHistogramHovered : ImGuiCol_PlotHistogram));
					}
					Gui::SameLine(0.0f, 0.0f); Gui::TextUnformatted(sourceItName);
				}
				static_assert(sizeof(Audio::HandleBaseType) == 2, "TODO: Update format strings");
				Gui::TableNextColumn(); Gui::Text("0x%04X", static_cast<Audio::HandleBaseType>(sourceIt));
				Gui::TableNextColumn(); Gui::Text("%.2f%%", ToPercent(sourceItBaseVolume));
				Gui::TableNextColumn(); Gui::Text("%d", sourceItInstanceCount);
				Gui::TableNextColumn(); (sourceItSampleBuffer != nullptr) ? Gui::Text("%d", sourceItSampleBuffer->ChannelCount) : Gui::TextDisabled("n/a");
				Gui::TableNextColumn(); (sourceItSampleBuffer != nullptr) ? Gui::Text("%d", sourceItSampleBuffer->SampleRate) : Gui::TextDisabled("n/a");
				Gui::TableNextColumn(); Gui::TextUnformatted(sourceItDuration.ToString().Data);

				if (pushTextColor) Gui::PopStyleColor();
				Gui::PopID();
			}
			Gui::EndTable();
		}
	}

	void AudioTestWindow::StartSourcePreview(Audio::SourceHandle source, Time startTime)
	{
		if (source == Audio::SourceHandle::Invalid)
			return;

		Audio::Engine.EnsureStreamRunning();

		if (!sourcePreviewVoiceHasBeenAdded)
		{
			sourcePreviewVoiceHasBeenAdded = true;
			sourcePreviewVoice = Audio::Engine.AddVoice(Audio::SourceHandle::Invalid, "AudioTestWindow SourcePreview", false, 1.0f, false);
			sourcePreviewVoice.SetPauseOnEnd(true);
		}

		sourcePreviewVoice.SetSource(source);
		sourcePreviewVoice.SetPosition(startTime);
		sourcePreviewVoice.SetIsPlaying(true);
	}

	void AudioTestWindow::StopSourcePreview()
	{
		if (sourcePreviewVoiceHasBeenAdded)
			sourcePreviewVoice.SetIsPlaying(false);
	}

	void AudioTestWindow::RemoveSourcePreviewVoice()
	{
		if (sourcePreviewVoiceHasBeenAdded)
			Audio::Engine.RemoveVoice(sourcePreviewVoice);
	}
}

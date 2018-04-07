cmake_minimum_required(VERSION 3.0)

project(ft0cc)

if(MSVC)
	add_compile_options(/std:c++17 /permissive- /EHsc /W4 /WX- /MT
		/Zc:wchar_t /Zc:inline /Zc:rvalueCast /Zc:forScope /fp:precise
		/wd4100 /wd4244 /wd4245 /wd4389
		/DFT0CC_EXT_BUILD /DWIN32)
else()
	add_compile_options(-std=c++17 -O3 -Wall -Wextra -Wpedantic -Werror
		-Wno-switch -Wno-sign-compare -Wno-narrowing -Wno-unused-parameter
		-DFT0CC_EXT_BUILD)
endif()

set(FT0CC_ROOT ../Source)
set(LIBFT0CC_ROOT ../libft0cc)

#all objects known to build on linux (will produce unresolved external symbols)
set(SRCS
	${LIBFT0CC_ROOT}/src/ft0cc/doc/dpcm_sample.cpp
	${LIBFT0CC_ROOT}/src/ft0cc/doc/groove.cpp
	${LIBFT0CC_ROOT}/src/ft0cc/doc/inst_sequence.cpp
#	${FT0CC_ROOT}/AboutDlg.cpp
#	${FT0CC_ROOT}/Accelerator.cpp
	${FT0CC_ROOT}/Action.cpp
	${FT0CC_ROOT}/ActionHandler.cpp
	${FT0CC_ROOT}/APU/2A03.cpp
	${FT0CC_ROOT}/APU/2A03Chan.cpp
	${FT0CC_ROOT}/APU/APU.cpp
	${FT0CC_ROOT}/APU/Channel.cpp
	${FT0CC_ROOT}/APU/DPCM.cpp
	${FT0CC_ROOT}/APU/ext/FDSSound_new.cpp
	${FT0CC_ROOT}/APU/FDS.cpp
	${FT0CC_ROOT}/APU/Mixer.cpp
	${FT0CC_ROOT}/APU/MixerChannel.cpp
	${FT0CC_ROOT}/APU/MixerLevels.cpp
	${FT0CC_ROOT}/APU/MMC5.cpp
	${FT0CC_ROOT}/APU/N163.cpp
	${FT0CC_ROOT}/APU/Noise.cpp
	${FT0CC_ROOT}/APU/S5B.cpp
	${FT0CC_ROOT}/APU/SampleMem.cpp
	${FT0CC_ROOT}/APU/SoundChip.cpp
	${FT0CC_ROOT}/APU/Square.cpp
	${FT0CC_ROOT}/APU/Triangle.cpp
	${FT0CC_ROOT}/APU/VRC6.cpp
	${FT0CC_ROOT}/APU/VRC7.cpp
	${FT0CC_ROOT}/Arpeggiator.cpp
#	${FT0CC_ROOT}/AudioDriver.cpp
	${FT0CC_ROOT}/Blip_Buffer/Blip_Buffer.cpp
	${FT0CC_ROOT}/Bookmark.cpp
	${FT0CC_ROOT}/BookmarkCollection.cpp
#	${FT0CC_ROOT}/BookmarkDlg.cpp
	${FT0CC_ROOT}/ChannelHandler.cpp
	${FT0CC_ROOT}/ChannelMap.cpp
	${FT0CC_ROOT}/ChannelOrder.cpp
#	${FT0CC_ROOT}/Channels2A03.cpp
#	${FT0CC_ROOT}/ChannelsDlg.cpp
	${FT0CC_ROOT}/ChannelsFDS.cpp
	${FT0CC_ROOT}/ChannelsMMC5.cpp
	${FT0CC_ROOT}/ChannelsN163.cpp
	${FT0CC_ROOT}/ChannelsS5B.cpp
	${FT0CC_ROOT}/ChannelsVRC6.cpp
	${FT0CC_ROOT}/ChannelsVRC7.cpp
	${FT0CC_ROOT}/ChipHandler.cpp
	${FT0CC_ROOT}/ChipHandlerS5B.cpp
	${FT0CC_ROOT}/ChipHandlerVRC7.cpp
	${FT0CC_ROOT}/Chunk.cpp
	${FT0CC_ROOT}/ChunkRenderBinary.cpp
	${FT0CC_ROOT}/ChunkRenderText.cpp
#	${FT0CC_ROOT}/Clipboard.cpp
#	${FT0CC_ROOT}/ClipboardResource.cpp
#	${FT0CC_ROOT}/CommandLineExport.cpp
#	${FT0CC_ROOT}/CommentsDlg.cpp
	${FT0CC_ROOT}/Compiler.cpp
	${FT0CC_ROOT}/CompoundAction.cpp
#	${FT0CC_ROOT}/ConfigAppearance.cpp
#	${FT0CC_ROOT}/ConfigGeneral.cpp
#	${FT0CC_ROOT}/ConfigMIDI.cpp
#	${FT0CC_ROOT}/ConfigMixer.cpp
#	${FT0CC_ROOT}/ConfigShortcuts.cpp
#	${FT0CC_ROOT}/ConfigSound.cpp
#	${FT0CC_ROOT}/ConfigVersion.cpp
#	${FT0CC_ROOT}/ControlPanelDlg.cpp
#	${FT0CC_ROOT}/CreateWaveDlg.cpp
#	${FT0CC_ROOT}/CustomControls.cpp
#	${FT0CC_ROOT}/DetuneDlg.cpp
	${FT0CC_ROOT}/DetuneTable.cpp
#	${FT0CC_ROOT}/DialogReBar.cpp
#	${FT0CC_ROOT}/DirectSound.cpp
	${FT0CC_ROOT}/DocumentFile.cpp
#	${FT0CC_ROOT}/DPI.cpp
	${FT0CC_ROOT}/DSampleManager.cpp
#	${FT0CC_ROOT}/Exception.cpp
#	${FT0CC_ROOT}/ExportDialog.cpp
#	${FT0CC_ROOT}/FamiTracker.cpp
#	${FT0CC_ROOT}/FamiTrackerDoc.cpp
#	${FT0CC_ROOT}/FamiTrackerDocIO.cpp
	${FT0CC_ROOT}/FamiTrackerDocIOJson.cpp
#	${FT0CC_ROOT}/FamiTrackerDocOldIO.cpp
	${FT0CC_ROOT}/FamiTrackerEnv.cpp
	${FT0CC_ROOT}/FamiTrackerModule.cpp
#	${FT0CC_ROOT}/FamiTrackerView.cpp
#	${FT0CC_ROOT}/FileDialogs.cpp
#	${FT0CC_ROOT}/FindDlg.cpp
#	${FT0CC_ROOT}/FrameAction.cpp
#	${FT0CC_ROOT}/FrameClipData.cpp
#	${FT0CC_ROOT}/FrameEditor.cpp
#	${FT0CC_ROOT}/FrameEditorModel.cpp
	${FT0CC_ROOT}/FrameEditorTypes.cpp
#	${FT0CC_ROOT}/GotoDlg.cpp
#	${FT0CC_ROOT}/GraphEditor.cpp
#	${FT0CC_ROOT}/GraphEditorComponent.cpp
#	${FT0CC_ROOT}/GraphEditorComponentImpl.cpp
#	${FT0CC_ROOT}/GraphEditorFactory.cpp
#	${FT0CC_ROOT}/Graphics.cpp
#	${FT0CC_ROOT}/GrooveDlg.cpp
	${FT0CC_ROOT}/InstCompiler.cpp
	${FT0CC_ROOT}/InstHandlerDPCM.cpp
	${FT0CC_ROOT}/InstHandlerVRC7.cpp
	${FT0CC_ROOT}/Instrument.cpp
	${FT0CC_ROOT}/Instrument2A03.cpp
#	${FT0CC_ROOT}/InstrumentEditDlg.cpp
#	${FT0CC_ROOT}/InstrumentEditorDPCM.cpp
#	${FT0CC_ROOT}/InstrumentEditorFDS.cpp
#	${FT0CC_ROOT}/InstrumentEditorFDSEnvelope.cpp
#	${FT0CC_ROOT}/InstrumentEditorN163Wave.cpp
#	${FT0CC_ROOT}/InstrumentEditorSeq.cpp
#	${FT0CC_ROOT}/InstrumentEditorVRC7.cpp
#	${FT0CC_ROOT}/InstrumentEditPanel.cpp
	${FT0CC_ROOT}/InstrumentFDS.cpp
#	${FT0CC_ROOT}/InstrumentFileTree.cpp
	${FT0CC_ROOT}/InstrumentIO.cpp
#	${FT0CC_ROOT}/InstrumentListCtrl.cpp
	${FT0CC_ROOT}/InstrumentManager.cpp
	${FT0CC_ROOT}/InstrumentN163.cpp
#	${FT0CC_ROOT}/InstrumentRecorder.cpp
	${FT0CC_ROOT}/InstrumentS5B.cpp
	${FT0CC_ROOT}/InstrumentService.cpp
	${FT0CC_ROOT}/InstrumentTypeImpl.cpp
	${FT0CC_ROOT}/InstrumentVRC6.cpp
	${FT0CC_ROOT}/InstrumentVRC7.cpp
	${FT0CC_ROOT}/Kraid.cpp
#	${FT0CC_ROOT}/MainFrm.cpp
#	${FT0CC_ROOT}/MIDI.cpp
#	${FT0CC_ROOT}/ModSequenceEditor.cpp
#	${FT0CC_ROOT}/ModuleAction.cpp
	${FT0CC_ROOT}/ModuleException.cpp
#	${FT0CC_ROOT}/ModuleImportDlg.cpp
#	${FT0CC_ROOT}/ModuleImporter.cpp
#	${FT0CC_ROOT}/ModulePropertiesDlg.cpp
	${FT0CC_ROOT}/NoteName.cpp
	${FT0CC_ROOT}/NoteQueue.cpp
	${FT0CC_ROOT}/OldSequence.cpp
#	${FT0CC_ROOT}/PatternAction.cpp
#	${FT0CC_ROOT}/PatternClipData.cpp
	${FT0CC_ROOT}/PatternCompiler.cpp
#	${FT0CC_ROOT}/PatternComponent.cpp
	${FT0CC_ROOT}/PatternData.cpp
#	${FT0CC_ROOT}/PatternEditor.cpp
	${FT0CC_ROOT}/PatternEditorTypes.cpp
#	${FT0CC_ROOT}/PCMImport.cpp
#	${FT0CC_ROOT}/PerformanceDlg.cpp
	${FT0CC_ROOT}/PeriodTables.cpp
	${FT0CC_ROOT}/PlayerCursor.cpp
#	${FT0CC_ROOT}/RecordSettingsDlg.cpp
#	${FT0CC_ROOT}/RegisterDisplay.cpp
	${FT0CC_ROOT}/RegisterState.cpp
	${FT0CC_ROOT}/resampler/resample.cpp
	${FT0CC_ROOT}/resampler/sinc.cpp
#	${FT0CC_ROOT}/SampleEditorDlg.cpp
#	${FT0CC_ROOT}/SampleEditorView.cpp
#	${FT0CC_ROOT}/SeqInstHandler.cpp
	${FT0CC_ROOT}/SeqInstHandlerFDS.cpp
	${FT0CC_ROOT}/SeqInstHandlerN163.cpp
	${FT0CC_ROOT}/SeqInstHandlerS5B.cpp
	${FT0CC_ROOT}/SeqInstHandlerSawtooth.cpp
	${FT0CC_ROOT}/SeqInstrument.cpp
	${FT0CC_ROOT}/Sequence.cpp
	${FT0CC_ROOT}/SequenceCollection.cpp
#	${FT0CC_ROOT}/SequenceEditor.cpp
	${FT0CC_ROOT}/SequenceManager.cpp
	${FT0CC_ROOT}/SequenceParser.cpp
#	${FT0CC_ROOT}/SequenceSetting.cpp
	${FT0CC_ROOT}/Settings.cpp
#	${FT0CC_ROOT}/SettingsService.cpp
	${FT0CC_ROOT}/SimpleFile.cpp
#	${FT0CC_ROOT}/SizeEditor.cpp
	${FT0CC_ROOT}/SongData.cpp
	${FT0CC_ROOT}/SongLengthScanner.cpp
	${FT0CC_ROOT}/SongState.cpp
	${FT0CC_ROOT}/SongView.cpp
	${FT0CC_ROOT}/SoundChipService.cpp
	${FT0CC_ROOT}/SoundChipSet.cpp
	${FT0CC_ROOT}/SoundChipTypeImpl.cpp
	${FT0CC_ROOT}/SoundDriver.cpp
#	${FT0CC_ROOT}/SoundGen.cpp
#	${FT0CC_ROOT}/SpeedDlg.cpp
#	${FT0CC_ROOT}/SplitKeyboardDlg.cpp
#	${FT0CC_ROOT}/stdafx.cpp
#	${FT0CC_ROOT}/StretchDlg.cpp
#	${FT0CC_ROOT}/SwapDlg.cpp
	${FT0CC_ROOT}/TempoCounter.cpp
	${FT0CC_ROOT}/TempoDisplay.cpp
#	${FT0CC_ROOT}/TextExporter.cpp
	${FT0CC_ROOT}/TrackData.cpp
	${FT0CC_ROOT}/TrackerChannel.cpp
#	${FT0CC_ROOT}/TransposeDlg.cpp
	${FT0CC_ROOT}/version.cpp
#	${FT0CC_ROOT}/VersionChecker.cpp
#	${FT0CC_ROOT}/VisualizerBase.cpp
#	${FT0CC_ROOT}/VisualizerScope.cpp
#	${FT0CC_ROOT}/VisualizerSpectrum.cpp
#	${FT0CC_ROOT}/VisualizerStatic.cpp
#	${FT0CC_ROOT}/VisualizerWnd.cpp
#	${FT0CC_ROOT}/WaveEditor.cpp
#	${FT0CC_ROOT}/WaveFile.cpp
	${FT0CC_ROOT}/WaveformGenerator.cpp
	${FT0CC_ROOT}/WavegenBuiltin.cpp
#	${FT0CC_ROOT}/WaveRenderer.cpp
	${FT0CC_ROOT}/WaveRendererFactory.cpp
#	${FT0CC_ROOT}/WavProgressDlg.cpp
)

add_library(ft0cc STATIC ${SRCS})
target_include_directories(ft0cc PUBLIC ${FT0CC_ROOT} ${LIBFT0CC_ROOT}/include)
# make -i -j4

#only the files we need
set(TEST_SRCS
	${LIBFT0CC_ROOT}/src/ft0cc/doc/dpcm_sample.cpp
	${LIBFT0CC_ROOT}/src/ft0cc/doc/groove.cpp
	${FT0CC_ROOT}/Bookmark.cpp
	${FT0CC_ROOT}/BookmarkCollection.cpp
	${FT0CC_ROOT}/ChannelMap.cpp
	${FT0CC_ROOT}/ChannelOrder.cpp
	${FT0CC_ROOT}/Chunk.cpp
	${FT0CC_ROOT}/ChunkRenderBinary.cpp
	${FT0CC_ROOT}/ChunkRenderText.cpp
	${FT0CC_ROOT}/Compiler.cpp
	${FT0CC_ROOT}/DetuneTable.cpp
	${FT0CC_ROOT}/DocumentFile.cpp
	${FT0CC_ROOT}/DSampleManager.cpp
#	${FT0CC_ROOT}/FamiTrackerDocIO.cpp
	${FT0CC_ROOT}/FamiTrackerDocIOJson.cpp
#	${FT0CC_ROOT}/FamiTrackerDocOldIO.cpp
	${FT0CC_ROOT}/FamiTrackerEnv.cpp
	${FT0CC_ROOT}/FamiTrackerModule.cpp
	${FT0CC_ROOT}/InstCompiler.cpp
	${FT0CC_ROOT}/Instrument.cpp
	${FT0CC_ROOT}/Instrument2A03.cpp
	${FT0CC_ROOT}/InstrumentFDS.cpp
	${FT0CC_ROOT}/InstrumentManager.cpp
	${FT0CC_ROOT}/InstrumentN163.cpp
	${FT0CC_ROOT}/InstrumentS5B.cpp
	${FT0CC_ROOT}/InstrumentService.cpp
	${FT0CC_ROOT}/InstrumentTypeImpl.cpp
	${FT0CC_ROOT}/InstrumentVRC6.cpp
	${FT0CC_ROOT}/InstrumentVRC7.cpp
	${FT0CC_ROOT}/Kraid.cpp
	${FT0CC_ROOT}/ModuleException.cpp
	${FT0CC_ROOT}/NoteName.cpp
	${FT0CC_ROOT}/OldSequence.cpp
	${FT0CC_ROOT}/PatternCompiler.cpp
	${FT0CC_ROOT}/PatternData.cpp
	${FT0CC_ROOT}/PeriodTables.cpp
	${FT0CC_ROOT}/SeqInstrument.cpp
	${FT0CC_ROOT}/Sequence.cpp
	${FT0CC_ROOT}/SequenceCollection.cpp
	${FT0CC_ROOT}/SequenceManager.cpp
	${FT0CC_ROOT}/SequenceParser.cpp
	${FT0CC_ROOT}/SimpleFile.cpp
	${FT0CC_ROOT}/SongData.cpp
	${FT0CC_ROOT}/SongLengthScanner.cpp
	${FT0CC_ROOT}/SongView.cpp
	${FT0CC_ROOT}/SoundChipService.cpp
	${FT0CC_ROOT}/SoundChipSet.cpp
	${FT0CC_ROOT}/SoundChipTypeImpl.cpp
	${FT0CC_ROOT}/TrackData.cpp
	${FT0CC_ROOT}/TrackerChannel.cpp
	${FT0CC_ROOT}/version.cpp
)

add_executable(ft0cc-test testMain.cpp ${TEST_SRCS})
target_include_directories(ft0cc-test PRIVATE ${FT0CC_ROOT} ${LIBFT0CC_ROOT}/include)
#target_link_libraries(ft0cc-test PRIVATE ft0cc)

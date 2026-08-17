#pragma once
#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringHealth.h"
#include "dozi/core/MasteringProjectStore.h"
#include "dozi/core/MasteringReference.h"
#include "dozi/core/MasteringVersionStore.h"
#include "MasteringModuleInspector.h"
#include "MasteringWaveformView.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <thread>

class PlaybackMeter final : public juce::Component {
public:
    void setLevels(float leftPeak, float rightPeak, float leftRms, float rightRms);
    void reset();
    void paint(juce::Graphics&) override;
private:
    std::atomic<float> leftPeak_ {-100.0f}, rightPeak_ {-100.0f};
    std::atomic<float> leftRms_ {-100.0f}, rightRms_ {-100.0f};
};

class CompactComboLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    juce::Font getComboBoxFont(juce::ComboBox&) override { return juce::Font(juce::FontOptions(12.0f)); }
    juce::Font getPopupMenuFont() override { return juce::Font(juce::FontOptions(12.0f)); }
};

class MasteringComponent final : public juce::AudioAppComponent, public juce::FileDragAndDropTarget, private juce::Timer, private juce::KeyListener {
public:
    MasteringComponent(); ~MasteringComponent() override;
    bool handleTransportShortcut(const juce::KeyPress&);
    void paint(juce::Graphics&) override; void resized() override;
    void prepareToPlay(int,double) override; void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override; void releaseResources() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override; void filesDropped(const juce::StringArray&,int,int) override;
private:
    using juce::Component::keyPressed;
    void timerCallback() override; void choose(); void chooseReference(); void clearReference(); void load(const juce::File&); void analyse(); void renderPreview(); void exportMaster(); void exportMasterTo(bool mp3,int bitRateKbps,bool createNew=false); void renderExportTo(const juce::File&,bool mp3,int bitRateKbps); void cancel();
    void saveVersion(); void restoreVersion(); void compareVersion();
    void saveProject(); void openProject(); void openProjectFile(const juce::File&); void applyProject(dozi::core::MasteringProject);
    void saveRecoverySession(); void restoreRecoverySession(); void showRecentSessions(); void addRecentSession(const juce::File&); void loadRecentSessions();
    void submitDoziRequest(); void applyLowEndAdjustment(double amountDb); void undoDoziRequest();
    [[nodiscard]] dozi::core::MasteringProject currentProject(const juce::File& previewOverride = {}) const;
    [[nodiscard]] juce::File sessionSupportDirectory() const;
    void refreshText(); void setBusy(bool); void loadTransport(const juce::File&,bool); void switchAB(); void updatePlaybackGain(); void updateMonitorGain(); void togglePlayback(); void showAudioDeviceSettings();
    void seekBoth(double seconds); void nudgePlayback(double seconds); void updateLoopControls(double startSeconds,double endSeconds); void updateTransportTime(double seconds);
    void installTransportKeyListeners(juce::Component& parent);
    bool keyPressed(const juce::KeyPress&, juce::Component*) override;
    [[nodiscard]] dozi::core::MasteringPlan planFromControls() const;
    void applyPlanToControls(const dozi::core::MasteringPlan&);
    juce::TextButton chooseButton{"Choose Mix"},referenceButton{"Choose Reference"},clearReferenceButton{"Clear Reference"},analyseButton{"Analyze + Plan"},previewButton{"Render Verified Master"},exportButton{"Export Master"},playButton{"Play"},deviceButton{"Audio Device"},cancelButton{"Cancel"};
    juce::TextButton saveVersionButton{"Save Version"},restoreVersionButton{"Restore Version"},compareVersionButton{"Compare Version"};
    juce::TextButton saveProjectButton{"Save Project"},openProjectButton{"Open Project"},recentProjectsButton{"Recent Sessions"};
    juce::ToggleButton abButton{"Playback: Mastered"},loudnessMatchButton{"Loudness Match"},loopTrackButton{"Loop Track"},loopSelectionButton{"Loop Selection"},monitorMuteButton{"Mute"},eq{"Corrective EQ"},dynamicEq{"Dynamic EQ"},deEsser{"De-Esser"},comp{"Glue Compressor"},multiband{"Multiband"},saturation{"Saturation"},stereo{"Width / Mono Bass"},limiter{"True Peak Limiter"};
    CompactComboLookAndFeel compactComboLookAndFeel;
    juce::ComboBox stylePreset,targetPreset; juce::Slider seek,target,ceiling,width,saturationDrive,saturationMix,monitorLevel; juce::Label title,status,referenceStatus,workflowLabel,formatBadge,transportTime; juce::TextEditor results,beforeSummary,afterSummary; juce::ProgressBar progress;
    juce::TextButton detailsButton {"Show Technical Details"};
    juce::TextButton doziApplyButton {"Apply Request"},doziUndoButton {"Undo Dozi"};
    juce::TextEditor doziPrompt;
    MasteringModuleInspector moduleInspector;
    MasteringWaveformView waveformView;
    double progressValue=0; std::vector<float>waveform; juce::File source,reference,mastered,previewFile,exportedFile;
    std::optional<dozi::core::MasteringAnalysisResult>before,referenceAnalysis,after; std::optional<dozi::core::MasteringPlan>plan; std::optional<dozi::core::MasteringHealthScore>beforeHealth,afterHealth;
    juce::AudioFormatManager formats; std::unique_ptr<juce::AudioFormatReaderSource>beforeReader,afterReader; juce::AudioTransportSource beforeTransport,afterTransport;
    juce::AudioBuffer<float> beforePlaybackBuffer, afterPlaybackBuffer;
    std::atomic<float> targetMasteredBlend {0.0f}, masteredPlaybackGain {1.0f}, monitorGain {1.0f};
    float currentMasteredBlend = 0.0f, currentMonitorGain = 1.0f;
    double playbackSampleRate = 48000.0;
    std::unique_ptr<juce::FileChooser>chooser; std::unique_ptr<std::thread>worker; std::shared_ptr<dozi::core::CancellationToken>token;
    std::vector<juce::File> recentSessions;
    std::vector<dozi::core::MasteringPlan> doziPlanHistory;
    PlaybackMeter playbackMeter;
    bool previewIsTemporary = false;
    bool playbackWasRunning = false;
    bool presetAuditionPending = false;
    juce::int64 presetAuditionRequestedMs = 0;
    double limiterMaximumGainReductionDb = 0.0, limiterSafetyTrimDb = 0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasteringComponent)
};

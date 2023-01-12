/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2023  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"

#include <QtWidgets/QMessageBox>
#include <algorithm>

#include "pcsx2/SPU2/Global.h"
#include "pcsx2/SPU2/spu2.h"
#include "pcsx2/VMManager.h"

#include "AudioSettingsWidget.h"
#include "QtHost.h"
#include "QtUtils.h"
#include "SettingWidgetBinder.h"
#include "SettingsDialog.h"

static constexpr s32 DEFAULT_SYNCHRONIZATION_MODE = 0;
static constexpr s32 DEFAULT_EXPANSION_MODE = 0;
static constexpr s32 DEFAULT_DPL_DECODING_LEVEL = 0;
static const char* DEFAULT_OUTPUT_MODULE = "cubeb";
static constexpr s32 DEFAULT_TARGET_LATENCY = 60;
static constexpr s32 DEFAULT_OUTPUT_LATENCY = 20;
static constexpr s32 DEFAULT_VOLUME = 100;
static constexpr s32 DEFAULT_SOUNDTOUCH_SEQUENCE_LENGTH = 30;
static constexpr s32 DEFAULT_SOUNDTOUCH_SEEK_WINDOW = 20;
static constexpr s32 DEFAULT_SOUNDTOUCH_OVERLAP = 10;

static const char* s_output_module_entries[] = {QT_TRANSLATE_NOOP("AudioSettingsWidget", "No Sound (Emulate SPU2 only)"),
	QT_TRANSLATE_NOOP("AudioSettingsWidget", "Cubeb (Cross-platform)"),
#ifdef _WIN32
	QT_TRANSLATE_NOOP("AudioSettingsWidget", "XAudio2"),
#endif
	nullptr};
static const char* s_output_module_values[] = {"nullout", "cubeb",
#ifdef _WIN32
	"xaudio2",
#endif
	nullptr};

AudioSettingsWidget::AudioSettingsWidget(SettingsDialog* dialog, QWidget* parent)
	: QWidget(parent)
	, m_dialog(dialog)
{
	SettingsInterface* sif = dialog->getSettingsInterface();

	m_ui.setupUi(this);

	SettingWidgetBinder::BindWidgetToIntSetting(sif, m_ui.syncMode, "SPU2/Output", "SynchMode", DEFAULT_SYNCHRONIZATION_MODE);
	SettingWidgetBinder::BindWidgetToIntSetting(sif, m_ui.expansionMode, "SPU2/Output", "SpeakerConfiguration", DEFAULT_EXPANSION_MODE);
	SettingWidgetBinder::BindWidgetToIntSetting(sif, m_ui.dplLevel, "SPU2/Output", "DplDecodingLevel", DEFAULT_DPL_DECODING_LEVEL);
	connect(m_ui.syncMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AudioSettingsWidget::updateTargetLatencyRange);
	connect(m_ui.expansionMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AudioSettingsWidget::expansionModeChanged);
	updateTargetLatencyRange();
	expansionModeChanged();

	SettingWidgetBinder::BindWidgetToEnumSetting(
		sif, m_ui.outputModule, "SPU2/Output", "OutputModule", s_output_module_entries, s_output_module_values, DEFAULT_OUTPUT_MODULE);
	SettingWidgetBinder::BindSliderToIntSetting(
		sif, m_ui.targetLatency, m_ui.targetLatencyLabel, tr(" ms"), "SPU2/Output", "Latency", DEFAULT_TARGET_LATENCY);
	SettingWidgetBinder::BindSliderToIntSetting(
		sif, m_ui.outputLatency, m_ui.outputLatencyLabel, tr(" ms"), "SPU2/Output", "OutputLatency", DEFAULT_OUTPUT_LATENCY);
	SettingWidgetBinder::BindWidgetToBoolSetting(sif, m_ui.outputLatencyMinimal, "SPU2/Output", "OutputLatencyMinimal", false);
	connect(m_ui.outputModule, &QComboBox::currentIndexChanged, this, &AudioSettingsWidget::outputModuleChanged);
	connect(m_ui.backend, &QComboBox::currentIndexChanged, this, &AudioSettingsWidget::outputBackendChanged);
	connect(m_ui.targetLatency, &QSlider::valueChanged, this, &AudioSettingsWidget::updateLatencyLabels);
	connect(m_ui.outputLatency, &QSlider::valueChanged, this, &AudioSettingsWidget::updateLatencyLabels);
	connect(m_ui.outputLatencyMinimal, &QCheckBox::stateChanged, this, &AudioSettingsWidget::updateLatencyLabels);
	connect(m_ui.outputLatencyMinimal, &QCheckBox::stateChanged, this, &AudioSettingsWidget::onMinimalOutputLatencyStateChanged);
	outputModuleChanged();

	m_ui.volume->setValue(m_dialog->getEffectiveIntValue("SPU2/Mixing", "FinalVolume", DEFAULT_VOLUME));
	m_ui.volume->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_ui.volume, &QSlider::valueChanged, this, &AudioSettingsWidget::volumeChanged);
	connect(m_ui.volume, &QSlider::customContextMenuRequested, this, &AudioSettingsWidget::volumeContextMenuRequested);
	updateVolumeLabel();
	if (sif && sif->ContainsValue("SPU2/Mixing", "FinalVolume"))
	{
		QFont bold_font(m_ui.volume->font());
		bold_font.setBold(true);
		m_ui.volumeLabel->setFont(bold_font);
	}

	SettingWidgetBinder::BindSliderToIntSetting(sif, m_ui.sequenceLength, m_ui.sequenceLengthLabel, tr(" ms"), "Soundtouch",
		"SequenceLengthMS", DEFAULT_SOUNDTOUCH_SEQUENCE_LENGTH);
	SettingWidgetBinder::BindSliderToIntSetting(
		sif, m_ui.seekWindowSize, m_ui.seekWindowSizeLabel, tr(" ms"), "Soundtouch", "SeekWindowMS", DEFAULT_SOUNDTOUCH_SEEK_WINDOW);
	SettingWidgetBinder::BindSliderToIntSetting(
		sif, m_ui.overlap, m_ui.overlapLabel, tr(" ms"), "Soundtouch", "OverlapMS", DEFAULT_SOUNDTOUCH_OVERLAP);
	connect(m_ui.resetTimestretchDefaults, &QPushButton::clicked, this, &AudioSettingsWidget::resetTimestretchDefaults);

	m_ui.label_3b->setVisible(false);
	m_ui.dplLevel->setVisible(false);

	onMinimalOutputLatencyStateChanged();
	updateLatencyLabels();

	dialog->registerWidgetHelp(m_ui.syncMode, tr("Synchronization"), tr("TimeStretch (Recommended)"), tr(""));

	dialog->registerWidgetHelp(m_ui.expansionMode, tr("Expansion"), tr("Stereo (None, Default)"), tr(""));

	dialog->registerWidgetHelp(m_ui.outputModule, tr("Output Module"), tr("Cubeb (Cross-platform)"), tr(""));

	dialog->registerWidgetHelp(m_ui.backend, tr("Output Backend"), tr("Default"), tr(""));

	dialog->registerWidgetHelp(m_ui.targetLatency, tr("Target Latency"), tr("60 ms"),
		tr("Determines the buffer size which the time stretcher will try to keep filled. It effectively selects the average latency, as "
		   "audio will be stretched/shrunk to keep the buffer size within check."));
	dialog->registerWidgetHelp(m_ui.outputLatency, tr("Output Latency"), tr("20 ms"),
		tr("Determines the latency from the buffer to the host audio output. This can be set lower than the target latency to reduce audio "
		   "delay."));

	dialog->registerWidgetHelp(m_ui.sequenceLength, tr("Sequence Length"), tr("30 ms"), tr(""));

	dialog->registerWidgetHelp(m_ui.seekWindowSize, tr("Seekwindow Size"), tr("20 ms"), tr(""));

	dialog->registerWidgetHelp(m_ui.overlap, tr("Overlap"), tr("10 ms"), tr(""));

	dialog->registerWidgetHelp(m_ui.volume, tr("Volume"), tr("100%"),
		tr("Pre-applies a volume modifier to the game's audio output before forwarding it to your computer."));
}

AudioSettingsWidget::~AudioSettingsWidget() = default;

void AudioSettingsWidget::expansionModeChanged()
{
	const bool expansion51 = m_dialog->getEffectiveIntValue("SPU2/Output", "SpeakerConfiguration", 0) == 2;
	m_ui.dplLevel->setDisabled(!expansion51);
}

void AudioSettingsWidget::outputModuleChanged()
{
	const std::string module_name(m_dialog->getEffectiveStringValue("SPU2/Output", "OutputModule", DEFAULT_OUTPUT_MODULE));
	const char* const* backend_names = GetOutputModuleBackends(module_name.c_str());

	const std::string backend_name(m_dialog->getEffectiveStringValue("SPU2/Output", "BackendName", ""));

	QSignalBlocker sb(m_ui.backend);
	m_ui.backend->clear();

	if (m_dialog->isPerGameSettings())
	{
		const QString global_backend(QString::fromStdString(Host::GetStringSettingValue("SPU2/Output", "BackendName", "")));
		m_ui.backend->addItem(tr("Use Global Setting [%1]").arg(global_backend.isEmpty() ? tr("Default") : global_backend));
	}

	m_ui.backend->setEnabled(backend_names != nullptr);
	m_ui.backend->addItem(tr("Default"));
	if (!backend_names || backend_name.empty())
		m_ui.backend->setCurrentIndex(0);

	if (backend_names)
	{
		for (u32 i = 0; backend_names[i] != nullptr; i++)
		{
			const int index = m_ui.backend->count();
			m_ui.backend->addItem(QString::fromUtf8(backend_names[i]));
			if (backend_name == backend_names[i])
				m_ui.backend->setCurrentIndex(index);
		}
	}

	updateDevices();
}

void AudioSettingsWidget::outputBackendChanged()
{
	int index = m_ui.backend->currentIndex();
	if (m_dialog->isPerGameSettings())
	{
		if (index == 0)
		{
			m_dialog->setStringSettingValue("SPU2/Output", "BackendName", std::nullopt);
			return;
		}

		index--;
	}

	if (index == 0)
		m_dialog->setStringSettingValue("SPU2/Output", "BackendName", "");
	else
		m_dialog->setStringSettingValue("SPU2/Output", "BackendName", m_ui.backend->currentText().toUtf8().constData());

	updateDevices();
}

void AudioSettingsWidget::updateDevices()
{
	const std::string module_name(m_dialog->getEffectiveStringValue("SPU2/Output", "OutputModule", DEFAULT_OUTPUT_MODULE));
	const std::string backend_name(m_dialog->getEffectiveStringValue("SPU2/Output", "BackendName", ""));

	m_ui.outputDevice->disconnect();
	m_ui.outputDevice->clear();
	m_output_device_latency = 0;

	std::vector<SndOutDeviceInfo> devices(GetOutputDeviceList(module_name.c_str(), backend_name.c_str()));
	if (devices.empty())
	{
		m_ui.outputDevice->addItem(tr("Default"));
		m_ui.outputDevice->setEnabled(false);
	}
	else
	{
		const std::string current_device(m_dialog->getEffectiveStringValue("SPU2/Output", "DeviceName", ""));

		m_ui.outputDevice->setEnabled(true);
		for (const SndOutDeviceInfo& devi : devices)
		{
			m_ui.outputDevice->addItem(QString::fromStdString(devi.display_name), QString::fromStdString(devi.name));
			if (devi.name == current_device)
				m_output_device_latency = devi.minimum_latency_frames;
		}

		SettingWidgetBinder::BindWidgetToStringSetting(
			m_dialog->getSettingsInterface(), m_ui.outputDevice, "SPU2/Output", "DeviceName", std::move(devices.front().name));
	}
}

void AudioSettingsWidget::volumeChanged(int value)
{
	// Nasty, but needed so we don't do a full settings apply and lag while dragging.
	if (SettingsInterface* sif = m_dialog->getSettingsInterface())
	{
		if (!m_ui.volumeLabel->font().bold())
		{
			QFont bold_font(m_ui.volumeLabel->font());
			bold_font.setBold(true);
			m_ui.volumeLabel->setFont(bold_font);
		}

		sif->SetIntValue("SPU2/Mixing", "FinalVolume", value);
		sif->Save();
	}
	else
	{
		Host::SetBaseIntSettingValue("SPU2/Mixing", "FinalVolume", value);
		Host::CommitBaseSettingChanges();
	}

	// Push through to emu thread since we're not applying.
	if (QtHost::IsVMValid())
	{
		Host::RunOnCPUThread([value]() {
			if (!VMManager::HasValidVM())
				return;

			EmuConfig.SPU2.FinalVolume = value;
			SPU2::SetOutputVolume(value);
		});
	}

	updateVolumeLabel();
}

void AudioSettingsWidget::volumeContextMenuRequested(const QPoint& pt)
{
	QMenu menu(m_ui.volume);
	m_ui.volume->connect(menu.addAction(qApp->translate("SettingWidgetBinder", "Reset")), &QAction::triggered, this, [this]() {
		const s32 global_value = Host::GetBaseIntSettingValue("SPU2/Mixing", "FinalVolume", DEFAULT_VOLUME);
		{
			QSignalBlocker sb(m_ui.volumeLabel);
			m_ui.volume->setValue(global_value);
			updateVolumeLabel();
		}

		if (m_ui.volumeLabel->font().bold())
		{
			QFont orig_font(m_ui.volumeLabel->font());
			orig_font.setBold(false);
			m_ui.volumeLabel->setFont(orig_font);
		}

		SettingsInterface* sif = m_dialog->getSettingsInterface();
		if (sif->ContainsValue("SPU2/Mixing", "FinalVolume"))
		{
			sif->DeleteValue("SPU2/Mixing", "FinalVolume");
			sif->Save();
			g_emu_thread->reloadGameSettings();
		}
	});
	menu.exec(m_ui.volume->mapToGlobal(pt));
}

void AudioSettingsWidget::updateVolumeLabel()
{
	m_ui.volumeLabel->setText(tr("%1%").arg(m_ui.volume->value()));
}

void AudioSettingsWidget::updateTargetLatencyRange()
{
	const Pcsx2Config::SPU2Options::SynchronizationMode sync_mode = static_cast<Pcsx2Config::SPU2Options::SynchronizationMode>(
		m_dialog->getIntValue("SPU2/Output", "SynchMode", DEFAULT_SYNCHRONIZATION_MODE).value_or(DEFAULT_SYNCHRONIZATION_MODE));

	m_ui.targetLatency->setMinimum((sync_mode == Pcsx2Config::SPU2Options::SynchronizationMode::TimeStretch) ?
									   Pcsx2Config::SPU2Options::MIN_LATENCY_TIMESTRETCH :
									   Pcsx2Config::SPU2Options::MIN_LATENCY);
	m_ui.targetLatency->setMaximum(Pcsx2Config::SPU2Options::MAX_LATENCY);
}

void AudioSettingsWidget::updateLatencyLabels()
{
	const bool minimal_output = m_dialog->getEffectiveBoolValue("SPU2/Output", "OutputLatencyMinimal", false);

	m_ui.outputLatencyLabel->setText(minimal_output ? tr("N/A") : tr("%1 ms").arg(m_ui.outputLatency->value()));

	const u32 output_latency_ms =
		minimal_output ? (((m_output_device_latency * 1000u) + 47999u) / 48000u) : static_cast<u32>(m_ui.outputLatency->value());
	const u32 buffer_ms = static_cast<u32>(m_ui.targetLatency->value());
	if (output_latency_ms > 0)
	{
		m_ui.latencySummary->setText(tr("Average Latency: %1 ms (%2 ms buffer + %3 ms output)")
										 .arg(buffer_ms + output_latency_ms)
										 .arg(buffer_ms)
										 .arg(output_latency_ms));
	}
	else
	{
		m_ui.latencySummary->setText(tr("Average Latency: %1 ms (minimum output latency unknown)").arg(buffer_ms));
	}
}

void AudioSettingsWidget::onMinimalOutputLatencyStateChanged()
{
	m_ui.outputLatency->setEnabled(!m_dialog->getEffectiveBoolValue("SPU2/Output", "OutputLatencyMinimal", false));
}

void AudioSettingsWidget::resetTimestretchDefaults()
{
	m_ui.sequenceLength->setValue(DEFAULT_SOUNDTOUCH_SEQUENCE_LENGTH);
	m_ui.seekWindowSize->setValue(DEFAULT_SOUNDTOUCH_SEEK_WINDOW);
	m_ui.overlap->setValue(DEFAULT_SOUNDTOUCH_OVERLAP);
}

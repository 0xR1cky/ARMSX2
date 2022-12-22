/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2022  PCSX2 Dev Team
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

#pragma once

#include <atomic>
#include <memory>
#include <functional>
#include <optional>

#include "pcsx2/Host.h"
#include "pcsx2/HostDisplay.h"
#include "pcsx2/HostSettings.h"
#include "pcsx2/Frontend/InputManager.h"
#include "pcsx2/VMManager.h"

#include <QtCore/QList>
#include <QtCore/QEventLoop>
#include <QtCore/QMetaType>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QSemaphore>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QThread>

class SettingsInterface;

class DisplayWidget;
struct VMBootParameters;

enum class CDVD_SourceType : uint8_t;

Q_DECLARE_METATYPE(std::shared_ptr<VMBootParameters>);
Q_DECLARE_METATYPE(std::optional<bool>);
Q_DECLARE_METATYPE(GSRendererType);
Q_DECLARE_METATYPE(InputBindingKey);
Q_DECLARE_METATYPE(CDVD_SourceType);

class EmuThread : public QThread
{
	Q_OBJECT

public:
	explicit EmuThread(QThread* ui_thread);
	~EmuThread();

	static void start();
	static void stop();

	__fi QEventLoop* getEventLoop() const { return m_event_loop; }
	__fi bool isFullscreen() const { return m_is_fullscreen; }
	__fi bool isRenderingToMain() const { return m_is_rendering_to_main; }
	__fi bool isSurfaceless() const { return m_is_surfaceless; }
	__fi bool isRunningFullscreenUI() const { return m_run_fullscreen_ui; }

	bool isOnEmuThread() const;
	bool shouldRenderToMain() const;

	/// Called back from the GS thread when the display state changes (e.g. fullscreen, render to main).
	bool acquireHostDisplay(RenderAPI api, bool clear_state_on_fail);
	void connectDisplaySignals(DisplayWidget* widget);
	void releaseHostDisplay(bool clear_state);
	void updateDisplay();

	void startBackgroundControllerPollTimer();
	void stopBackgroundControllerPollTimer();
	void updatePerformanceMetrics(bool force);

public Q_SLOTS:
	bool confirmMessage(const QString& title, const QString& message);
	void loadSettings(SettingsInterface& si, std::unique_lock<std::mutex>& lock);
	void checkForSettingChanges(const Pcsx2Config& old_config);
	void startFullscreenUI(bool fullscreen);
	void stopFullscreenUI();
	void startVM(std::shared_ptr<VMBootParameters> boot_params);
	void resetVM();
	void setVMPaused(bool paused);
	void shutdownVM(bool save_state = true);
	void loadState(const QString& filename);
	void loadStateFromSlot(qint32 slot);
	void saveState(const QString& filename);
	void saveStateToSlot(qint32 slot);
	void toggleFullscreen();
	void setFullscreen(bool fullscreen);
	void setSurfaceless(bool surfaceless);
	void applySettings();
	void reloadGameSettings();
	void updateEmuFolders();
	void toggleSoftwareRendering();
	void switchRenderer(GSRendererType renderer);
	void changeDisc(CDVD_SourceType source, const QString& path);
	void reloadPatches();
	void reloadInputSources();
	void reloadInputBindings();
	void reloadInputDevices();
	void closeInputSources();
	void requestDisplaySize(float scale);
	void enumerateInputDevices();
	void enumerateVibrationMotors();
	void runOnCPUThread(const std::function<void()>& func);
	void queueSnapshot(quint32 gsdump_frames);
	void beginCapture(const QString& path);
	void endCapture();

Q_SIGNALS:
	bool messageConfirmed(const QString& title, const QString& message);

	DisplayWidget* onCreateDisplayRequested(bool fullscreen, bool render_to_main);
	DisplayWidget* onUpdateDisplayRequested(bool fullscreen, bool render_to_main, bool surfaceless);
	void onResizeDisplayRequested(qint32 width, qint32 height);
	void onDestroyDisplayRequested();
	void onRelativeMouseModeRequested(bool enabled);

	/// Called when the VM is starting initialization, but has not been completed yet.
	void onVMStarting();

	/// Called when the VM is created.
	void onVMStarted();

	/// Called when the VM is paused.
	void onVMPaused();

	/// Called when the VM is resumed after being paused.
	void onVMResumed();

	/// Called when the VM is shut down or destroyed.
	void onVMStopped();

	/// Provided by the host; called when the running executable changes.
	void onGameChanged(const QString& path, const QString& elf_override, const QString& serial, const QString& name, quint32 crc);

	void onInputDevicesEnumerated(const QList<QPair<QString, QString>>& devices);
	void onInputDeviceConnected(const QString& identifier, const QString& device_name);
	void onInputDeviceDisconnected(const QString& identifier);
	void onVibrationMotorsEnumerated(const QList<InputBindingKey>& motors);

	/// Called when a save state is loading, before the file is processed.
	void onSaveStateLoading(const QString& path);

	/// Called after a save state is successfully loaded. If the save state was invalid, was_successful will be false.
	void onSaveStateLoaded(const QString& path, bool was_successful);

	/// Called when a save state is being created/saved. The compression/write to disk is asynchronous, so this callback
	/// just signifies that the save has started, not necessarily completed.
	void onSaveStateSaved(const QString& path);

	/// Called when achievements are reloaded/refreshed (e.g. game change, login, option change).
	void onAchievementsRefreshed(quint32 id, const QString& game_info_string, quint32 total, quint32 points);

protected:
	void run();

private:
	/// Interval at which the controllers are polled when the system is not active.
	static constexpr u32 BACKGROUND_CONTROLLER_POLLING_INTERVAL = 100;

	/// Poll at half the vsync rate for FSUI to reduce the chance of getting a press+release in the same frame.
	static constexpr u32 FULLSCREEN_UI_CONTROLLER_POLLING_INTERVAL = 8;

	void destroyVM();
	void executeVM();

	void createBackgroundControllerPollTimer();
	void destroyBackgroundControllerPollTimer();
	void connectSignals();

private Q_SLOTS:
	void stopInThread();
	void doBackgroundControllerPoll();
	void onDisplayWindowResized(int width, int height, float scale);
	void onApplicationStateChanged(Qt::ApplicationState state);
	void redrawDisplayWindow();

private:
	QThread* m_ui_thread;
	QSemaphore m_started_semaphore;
	QEventLoop* m_event_loop = nullptr;
	QTimer* m_background_controller_polling_timer = nullptr;

	std::atomic_bool m_shutdown_flag{false};

	bool m_verbose_status = false;
	bool m_run_fullscreen_ui = false;
	bool m_is_rendering_to_main = false;
	bool m_is_fullscreen = false;
	bool m_is_surfaceless = false;
	bool m_save_state_on_shutdown = false;
	bool m_pause_on_focus_loss = false;

	bool m_was_paused_by_focus_loss = false;

	float m_last_speed = 0.0f;
	float m_last_game_fps = 0.0f;
	float m_last_video_fps = 0.0f;
	int m_last_internal_width = 0;
	int m_last_internal_height = 0;
	GSRendererType m_last_renderer = GSRendererType::Null;
};

extern EmuThread* g_emu_thread;

namespace QtHost
{
	/// Sets batch mode (exit after game shutdown).
	bool InBatchMode();

	/// Sets NoGUI mode (implys batch mode, does not display main window, exits on shutdown).
	bool InNoGUIMode();

	/// Returns true if the calling thread is the UI thread.
	bool IsOnUIThread();

	/// Returns true if advanced settings should be shown.
	bool ShouldShowAdvancedSettings();

	/// Executes a function on the UI thread.
	void RunOnUIThread(const std::function<void()>& func, bool block = false);

	/// Returns the application name and version, optionally including debug/devel config indicator.
	QString GetAppNameAndVersion();

	/// Returns the debug/devel config indicator.
	QString GetAppConfigSuffix();

	/// Returns the base path for resources. This may be : prefixed, if we're using embedded resources.
	QString GetResourcesBasePath();

	/// VM state, safe to access on UI thread.
	bool IsVMValid();
	bool IsVMPaused();
} // namespace QtHost

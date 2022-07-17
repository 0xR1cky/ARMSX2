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
#include "pcsx2/Host.h"
#include "pcsx2/HostDisplay.h"
#include "pcsx2/Frontend/InputManager.h"
#include <QtCore/QList>
#include <QtCore/QEventLoop>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QSemaphore>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <atomic>
#include <memory>

class DisplayWidget;
struct VMBootParameters;

enum class CDVD_SourceType : uint8_t;

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

	bool isOnEmuThread() const;

	/// Called back from the GS thread when the display state changes (e.g. fullscreen, render to main).
	HostDisplay* acquireHostDisplay(HostDisplay::RenderAPI api);
	void connectDisplaySignals(DisplayWidget* widget);
	void releaseHostDisplay();
	void updateDisplay();

	void startBackgroundControllerPollTimer();
	void stopBackgroundControllerPollTimer();
	void updatePerformanceMetrics(bool force);

public Q_SLOTS:
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
	void requestDisplaySize(float scale);
	void enumerateInputDevices();
	void enumerateVibrationMotors();
	void runOnCPUThread(const std::function<void()>& func);
	void queueSnapshot(quint32 gsdump_frames);

Q_SIGNALS:
	DisplayWidget* onCreateDisplayRequested(bool fullscreen, bool render_to_main);
	DisplayWidget* onUpdateDisplayRequested(bool fullscreen, bool render_to_main, bool surfaceless);
	void onResizeDisplayRequested(qint32 width, qint32 height);
	void onDestroyDisplayRequested();

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
	void onGameChanged(const QString& path, const QString& serial, const QString& name, quint32 crc);

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

protected:
	void run();

private:
	static constexpr u32 BACKGROUND_CONTROLLER_POLLING_INTERVAL =
		100; /// Interval at which the controllers are polled when the system is not active.

	void destroyVM();
	void executeVM();
	void checkForSettingChanges();
	bool shouldRenderToMain() const;

	void createBackgroundControllerPollTimer();
	void destroyBackgroundControllerPollTimer();
	void loadOurSettings();
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

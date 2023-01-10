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

#include "PrecompiledHeader.h"

#include <QtCore/QDateTime>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>

#include "common/Assertions.h"
#include "common/CocoaTools.h"
#include "common/FileSystem.h"

#include "pcsx2/CDVD/CDVDcommon.h"
#include "pcsx2/CDVD/CDVDdiscReader.h"
#include "pcsx2/Frontend/GameList.h"
#include "pcsx2/Frontend/LogSink.h"
#include "pcsx2/GS/GS.h"
#include "pcsx2/GSDumpReplayer.h"
#include "pcsx2/HostDisplay.h"
#include "pcsx2/HostSettings.h"
#include "pcsx2/PerformanceMetrics.h"
#include "pcsx2/Recording/InputRecording.h"
#include "pcsx2/Recording/InputRecordingControls.h"

#include "AboutDialog.h"
#include "AutoUpdaterDialog.h"
#include "CoverDownloadDialog.h"
#include "DisplayWidget.h"
#include "GameList/GameListRefreshThread.h"
#include "GameList/GameListWidget.h"
#include "MainWindow.h"
#include "QtHost.h"
#include "QtUtils.h"
#include "Settings/ControllerSettingsDialog.h"
#include "Settings/GameListSettingsWidget.h"
#include "Settings/InterfaceSettingsWidget.h"
#include "SettingWidgetBinder.h"
#include "svnrev.h"
#include "Tools/InputRecording/InputRecordingViewer.h"
#include "Tools/InputRecording/NewInputRecordingDlg.h"

#ifdef _WIN32
#include "common/RedtapeWindows.h"
#include <Dbt.h>
#endif

#ifdef ENABLE_RAINTEGRATION
#include "pcsx2/Frontend/Achievements.h"
#endif


const char* MainWindow::OPEN_FILE_FILTER =
	QT_TRANSLATE_NOOP("MainWindow", "All File Types (*.bin *.iso *.cue *.chd *.cso *.gz *.elf *.irx *.gs *.gs.xz *.gs.zst *.dump);;"
									"Single-Track Raw Images (*.bin *.iso);;"
									"Cue Sheets (*.cue);;"
									"MAME CHD Images (*.chd);;"
									"CSO Images (*.cso);;"
									"GZ Images (*.gz);;"
									"ELF Executables (*.elf);;"
									"IRX Executables (*.irx);;"
									"GS Dumps (*.gs *.gs.xz *.gs.zst);;"
									"Block Dumps (*.dump)");

const char* MainWindow::DISC_IMAGE_FILTER = QT_TRANSLATE_NOOP("MainWindow", "All File Types (*.bin *.iso *.cue *.chd *.cso *.gz *.dump);;"
																			"Single-Track Raw Images (*.bin *.iso);;"
																			"Cue Sheets (*.cue);;"
																			"MAME CHD Images (*.chd);;"
																			"CSO Images (*.cso);;"
																			"GZ Images (*.gz);;"
																			"Block Dumps (*.dump)");

#ifdef __APPLE__
const char* MainWindow::DEFAULT_THEME_NAME = "";
#else
const char* MainWindow::DEFAULT_THEME_NAME = "darkfusion";
#endif

MainWindow* g_main_window = nullptr;
static QString s_unthemed_style_name;
static QPalette s_unthemed_palette;
static bool s_unthemed_style_name_set;

#if defined(_WIN32) || defined(__APPLE__)
static const bool s_use_central_widget = false;
#else
// Qt Wayland is broken. Any sort of stacked widget usage fails to update,
// leading to broken window resizes, no display rendering, etc. So, we mess
// with the central widget instead. Which we can't do on xorg, because it
// breaks window resizing there...
static bool s_use_central_widget = false;
#endif

// UI thread VM validity.
static bool s_vm_valid = false;
static bool s_vm_paused = false;

MainWindow::MainWindow()
{
	pxAssert(!g_main_window);
	g_main_window = this;

#if !defined(_WIN32) && !defined(__APPLE__)
	s_use_central_widget = DisplayContainer::isRunningOnWayland();
#endif
}

MainWindow::~MainWindow()
{
	// make sure the game list isn't refreshing, because it's on a separate thread
	cancelGameListRefresh();

	// we compare here, since recreate destroys the window later
	if (g_main_window == this)
		g_main_window = nullptr;
#ifdef _WIN32
	unregisterForDeviceNotifications();
#endif
#ifdef __APPLE__
	CocoaTools::RemoveThemeChangeHandler(this);
#endif
}

void MainWindow::initialize()
{
#ifdef __APPLE__
	CocoaTools::AddThemeChangeHandler(this, [](void* ctx) {
		// This handler is called *before* the style change has propagated far enough for Qt to see it
		// Use RunOnUIThread to delay until it has
		QtHost::RunOnUIThread([ctx = static_cast<MainWindow*>(ctx)] {
			ctx->updateTheme(); // Qt won't notice the style change without us touching the palette in some way
		});
	});
#endif
	m_ui.setupUi(this);
	setupAdditionalUi();
	connectSignals();
	connectVMThreadSignals(g_emu_thread);

	restoreStateFromConfig();
	switchToGameListView();
	updateWindowTitle();
	updateSaveStateMenus(QString(), QString(), 0);

#ifdef _WIN32
	registerForDeviceNotifications();
#endif
}

// TODO: Figure out how to set this in the .ui file
/// Marks the icons for all actions in the given menu as mask icons
/// This means macOS's menubar renderer will ignore color values and use only the alpha in the image.
/// The color value will instead be taken from the system theme.
/// Since the menubar follows the OS's dark/light mode and not our current theme's, this prevents problems where a theme mismatch puts white icons in light mode or dark icons in dark mode.
static void makeIconsMasks(QWidget* menu)
{
	for (QAction* action : menu->actions())
	{
		if (!action->icon().isNull())
		{
			QIcon icon = action->icon();
			icon.setIsMask(true);
			action->setIcon(icon);
		}
		if (action->menu())
			makeIconsMasks(action->menu());
	}
}

QWidget* MainWindow::getContentParent()
{
	return s_use_central_widget ? static_cast<QWidget*>(this) : static_cast<QWidget*>(m_ui.mainContainer);
}

void MainWindow::setupAdditionalUi()
{
	const bool show_advanced_settings = QtHost::ShouldShowAdvancedSettings();

	setWindowIcon(QIcon(QStringLiteral("%1/icons/AppIconLarge.png").arg(QtHost::GetResourcesBasePath())));
	makeIconsMasks(menuBar());

	m_ui.menuDebug->menuAction()->setVisible(show_advanced_settings);

	const bool toolbar_visible = Host::GetBaseBoolSettingValue("UI", "ShowToolbar", false);
	m_ui.actionViewToolbar->setChecked(toolbar_visible);
	m_ui.toolBar->setVisible(toolbar_visible);

	const bool toolbars_locked = Host::GetBaseBoolSettingValue("UI", "LockToolbar", false);
	m_ui.actionViewLockToolbar->setChecked(toolbars_locked);
	m_ui.toolBar->setMovable(!toolbars_locked);
	m_ui.toolBar->setContextMenuPolicy(Qt::PreventContextMenu);

	const bool status_bar_visible = Host::GetBaseBoolSettingValue("UI", "ShowStatusBar", true);
	m_ui.actionViewStatusBar->setChecked(status_bar_visible);
	m_ui.statusBar->setVisible(status_bar_visible);

	m_game_list_widget = new GameListWidget(getContentParent());
	m_game_list_widget->initialize();
	m_ui.actionGridViewShowTitles->setChecked(m_game_list_widget->getShowGridCoverTitles());
	if (s_use_central_widget)
	{
		m_ui.mainContainer = nullptr; // setCentralWidget() will delete this
		setCentralWidget(m_game_list_widget);
	}
	else
	{
		m_ui.mainContainer->addWidget(m_game_list_widget);
	}

	m_status_progress_widget = new QProgressBar(m_ui.statusBar);
	m_status_progress_widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	m_status_progress_widget->setFixedSize(140, 16);
	m_status_progress_widget->setMinimum(0);
	m_status_progress_widget->setMaximum(100);
	m_status_progress_widget->hide();

	m_status_verbose_widget = new QLabel(m_ui.statusBar);
	m_status_verbose_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_status_verbose_widget->setFixedHeight(16);
	m_status_verbose_widget->hide();

	m_status_renderer_widget = new QLabel(m_ui.statusBar);
	m_status_renderer_widget->setFixedHeight(16);
	m_status_renderer_widget->setFixedSize(65, 16);
	m_status_renderer_widget->hide();

	m_status_resolution_widget = new QLabel(m_ui.statusBar);
	m_status_resolution_widget->setFixedHeight(16);
	m_status_resolution_widget->setFixedSize(70, 16);
	m_status_resolution_widget->hide();

	m_status_fps_widget = new QLabel(m_ui.statusBar);
	m_status_fps_widget->setFixedSize(85, 16);
	m_status_fps_widget->hide();

	m_status_vps_widget = new QLabel(m_ui.statusBar);
	m_status_vps_widget->setFixedSize(125, 16);
	m_status_vps_widget->hide();

	for (u32 scale = 0; scale <= 10; scale++)
	{
		QAction* action = m_ui.menuWindowSize->addAction((scale == 0) ? tr("Internal Resolution") : tr("%1x Scale").arg(scale));
		connect(action, &QAction::triggered, [scale]() { g_emu_thread->requestDisplaySize(static_cast<float>(scale)); });
	}

	updateEmulationActions(false, false);
	updateDisplayRelatedActions(false, false, false);

#ifdef ENABLE_RAINTEGRATION
	if (Achievements::IsUsingRAIntegration())
	{
		QMenu* raMenu = new QMenu(QStringLiteral("RAIntegration"), m_ui.menu_Tools);
		connect(raMenu, &QMenu::aboutToShow, this, [this, raMenu]() {
			raMenu->clear();

			const auto items = Achievements::RAIntegration::GetMenuItems();
			for (const auto& [id, title, checked] : items)
			{
				if (id == 0)
				{
					raMenu->addSeparator();
					continue;
				}

				QAction* raAction = raMenu->addAction(QString::fromUtf8(title));
				if (checked)
				{
					raAction->setCheckable(true);
					raAction->setChecked(checked);
				}

				connect(raAction, &QAction::triggered, this,
					[id = id]() { Host::RunOnCPUThread([id]() { Achievements::RAIntegration::ActivateMenuItem(id); }, false); });
			}
		});
		m_ui.menu_Tools->insertMenu(m_ui.menuInput_Recording->menuAction(), raMenu);
	}
#endif
}

void MainWindow::connectSignals()
{
	connect(m_ui.actionStartFile, &QAction::triggered, this, &MainWindow::onStartFileActionTriggered);
	connect(m_ui.actionStartDisc, &QAction::triggered, this, &MainWindow::onStartDiscActionTriggered);
	connect(m_ui.actionStartBios, &QAction::triggered, this, &MainWindow::onStartBIOSActionTriggered);
	connect(m_ui.actionChangeDisc, &QAction::triggered, [this] { m_ui.menuChangeDisc->exec(QCursor::pos()); });
	connect(m_ui.actionChangeDiscFromFile, &QAction::triggered, this, &MainWindow::onChangeDiscFromFileActionTriggered);
	connect(m_ui.actionChangeDiscFromDevice, &QAction::triggered, this, &MainWindow::onChangeDiscFromDeviceActionTriggered);
	connect(m_ui.actionChangeDiscFromGameList, &QAction::triggered, this, &MainWindow::onChangeDiscFromGameListActionTriggered);
	connect(m_ui.actionRemoveDisc, &QAction::triggered, this, &MainWindow::onRemoveDiscActionTriggered);
	connect(m_ui.menuChangeDisc, &QMenu::aboutToShow, this, &MainWindow::onChangeDiscMenuAboutToShow);
	connect(m_ui.menuChangeDisc, &QMenu::aboutToHide, this, &MainWindow::onChangeDiscMenuAboutToHide);
	connect(m_ui.actionPowerOff, &QAction::triggered, this, [this]() { requestShutdown(true, true, EmuConfig.SaveStateOnShutdown); });
	connect(m_ui.actionPowerOffWithoutSaving, &QAction::triggered, this, [this]() { requestShutdown(false, false, false); });
	connect(m_ui.actionLoadState, &QAction::triggered, this, [this]() { m_ui.menuLoadState->exec(QCursor::pos()); });
	connect(m_ui.actionSaveState, &QAction::triggered, this, [this]() { m_ui.menuSaveState->exec(QCursor::pos()); });
	connect(m_ui.actionExit, &QAction::triggered, this, &MainWindow::close);
	connect(m_ui.actionScreenshot, &QAction::triggered, this, &MainWindow::onScreenshotActionTriggered);
	connect(m_ui.menuLoadState, &QMenu::aboutToShow, this, &MainWindow::onLoadStateMenuAboutToShow);
	connect(m_ui.menuSaveState, &QMenu::aboutToShow, this, &MainWindow::onSaveStateMenuAboutToShow);
	connect(m_ui.actionSettings, &QAction::triggered, [this]() { doSettings(); });
	connect(m_ui.actionInterfaceSettings, &QAction::triggered, [this]() { doSettings("Interface"); });
	connect(m_ui.actionGameListSettings, &QAction::triggered, [this]() { doSettings("Game List"); });
	connect(m_ui.actionEmulationSettings, &QAction::triggered, [this]() { doSettings("Emulation"); });
	connect(m_ui.actionBIOSSettings, &QAction::triggered, [this]() { doSettings("BIOS"); });
	connect(m_ui.actionGraphicsSettings, &QAction::triggered, [this]() { doSettings("Graphics"); });
	connect(m_ui.actionAudioSettings, &QAction::triggered, [this]() { doSettings("Audio"); });
	connect(m_ui.actionMemoryCardSettings, &QAction::triggered, [this]() { doSettings("Memory Cards"); });
	connect(m_ui.actionDEV9Settings, &QAction::triggered, [this]() { doSettings("Network & HDD"); });
	connect(m_ui.actionFolderSettings, &QAction::triggered, [this]() { doSettings("Folders"); });
	connect(m_ui.actionAchievementSettings, &QAction::triggered, [this]() { doSettings("Achievements"); });
	connect(m_ui.actionControllerSettings, &QAction::triggered,
		[this]() { doControllerSettings(ControllerSettingsDialog::Category::GlobalSettings); });
	connect(m_ui.actionHotkeySettings, &QAction::triggered,
		[this]() { doControllerSettings(ControllerSettingsDialog::Category::HotkeySettings); });
	connect(m_ui.actionAddGameDirectory, &QAction::triggered,
		[this]() { getSettingsDialog()->getGameListSettingsWidget()->addSearchDirectory(this); });
	connect(m_ui.actionScanForNewGames, &QAction::triggered, [this]() { refreshGameList(false); });
	connect(m_ui.actionRescanAllGames, &QAction::triggered, [this]() { refreshGameList(true); });
	connect(m_ui.actionViewToolbar, &QAction::toggled, this, &MainWindow::onViewToolbarActionToggled);
	connect(m_ui.actionViewLockToolbar, &QAction::toggled, this, &MainWindow::onViewLockToolbarActionToggled);
	connect(m_ui.actionViewStatusBar, &QAction::toggled, this, &MainWindow::onViewStatusBarActionToggled);
	connect(m_ui.actionViewGameList, &QAction::triggered, this, &MainWindow::onViewGameListActionTriggered);
	connect(m_ui.actionViewGameGrid, &QAction::triggered, this, &MainWindow::onViewGameGridActionTriggered);
	connect(m_ui.actionViewSystemDisplay, &QAction::triggered, this, &MainWindow::onViewSystemDisplayTriggered);
	connect(m_ui.actionViewGameProperties, &QAction::triggered, this, &MainWindow::onViewGamePropertiesActionTriggered);
	connect(m_ui.actionGitHubRepository, &QAction::triggered, this, &MainWindow::onGitHubRepositoryActionTriggered);
	connect(m_ui.actionSupportForums, &QAction::triggered, this, &MainWindow::onSupportForumsActionTriggered);
	connect(m_ui.actionDiscordServer, &QAction::triggered, this, &MainWindow::onDiscordServerActionTriggered);
	connect(m_ui.actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
	connect(m_ui.actionAbout, &QAction::triggered, this, &MainWindow::onAboutActionTriggered);
	connect(m_ui.actionCheckForUpdates, &QAction::triggered, this, &MainWindow::onCheckForUpdatesActionTriggered);
	connect(m_ui.actionOpenDataDirectory, &QAction::triggered, this, &MainWindow::onToolsOpenDataDirectoryTriggered);
	connect(m_ui.actionCoverDownloader, &QAction::triggered, this, &MainWindow::onToolsCoverDownloaderTriggered);
	connect(m_ui.actionGridViewShowTitles, &QAction::triggered, m_game_list_widget, &GameListWidget::setShowCoverTitles);
	connect(m_ui.actionGridViewZoomIn, &QAction::triggered, m_game_list_widget, [this]() {
		if (isShowingGameList())
			m_game_list_widget->gridZoomIn();
	});
	connect(m_ui.actionGridViewZoomOut, &QAction::triggered, m_game_list_widget, [this]() {
		if (isShowingGameList())
			m_game_list_widget->gridZoomOut();
	});
	connect(m_ui.actionGridViewRefreshCovers, &QAction::triggered, m_game_list_widget, &GameListWidget::refreshGridCovers);
	connect(m_game_list_widget, &GameListWidget::layoutChange, this, [this]() {
		QSignalBlocker sb(m_ui.actionGridViewShowTitles);
		m_ui.actionGridViewShowTitles->setChecked(m_game_list_widget->getShowGridCoverTitles());
	});

	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionViewStatusBarVerbose, "UI", "VerboseStatusBar", false);

	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionEnableSystemConsole, "Logging", "EnableSystemConsole", false);
#ifndef PCSX2_DEVBUILD
	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionEnableVerboseLogging, "Logging", "EnableVerbose", false);
#else
	// Dev builds always have verbose logging.
	m_ui.actionEnableVerboseLogging->setChecked(true);
	m_ui.actionEnableVerboseLogging->setEnabled(false);
#endif
	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionEnableEEConsoleLogging, "Logging", "EnableEEConsole", true);
	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionEnableIOPConsoleLogging, "Logging", "EnableIOPConsole", true);
	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionEnableFileLogging, "Logging", "EnableFileLogging", false);
	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionEnableLogTimestamps, "Logging", "EnableTimestamps", true);
	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionEnableCDVDVerboseReads, "EmuCore", "CdvdVerboseReads", false);
	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionSaveBlockDump, "EmuCore", "CdvdDumpBlocks", false);
	m_ui.actionShowAdvancedSettings->setChecked(QtHost::ShouldShowAdvancedSettings());
	connect(m_ui.actionSaveBlockDump, &QAction::toggled, this, &MainWindow::onBlockDumpActionToggled);
	connect(m_ui.actionShowAdvancedSettings, &QAction::toggled, this, &MainWindow::onShowAdvancedSettingsToggled);
	connect(m_ui.actionSaveGSDump, &QAction::triggered, this, &MainWindow::onSaveGSDumpActionTriggered);
	connect(m_ui.actionToolsVideoCapture, &QAction::toggled, this, &MainWindow::onToolsVideoCaptureToggled);

	// Input Recording
	connect(m_ui.actionInputRecNew, &QAction::triggered, this, &MainWindow::onInputRecNewActionTriggered);
	connect(m_ui.actionInputRecPlay, &QAction::triggered, this, &MainWindow::onInputRecPlayActionTriggered);
	connect(m_ui.actionInputRecStop, &QAction::triggered, this, &MainWindow::onInputRecStopActionTriggered);
	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionInputRecConsoleLogs, "Logging", "EnableInputRecordingLogs", false);
	SettingWidgetBinder::BindWidgetToBoolSetting(nullptr, m_ui.actionInputRecControllerLogs, "Logging", "EnableControllerLogs", false);
	connect(m_ui.actionInputRecOpenViewer, &QAction::triggered, this, &MainWindow::onInputRecOpenViewer);

	// These need to be queued connections to stop crashing due to menus opening/closing and switching focus.
	connect(m_game_list_widget, &GameListWidget::refreshProgress, this, &MainWindow::onGameListRefreshProgress);
	connect(m_game_list_widget, &GameListWidget::refreshComplete, this, &MainWindow::onGameListRefreshComplete);
	connect(m_game_list_widget, &GameListWidget::selectionChanged, this, &MainWindow::onGameListSelectionChanged, Qt::QueuedConnection);
	connect(m_game_list_widget, &GameListWidget::entryActivated, this, &MainWindow::onGameListEntryActivated, Qt::QueuedConnection);
	connect(m_game_list_widget, &GameListWidget::entryContextMenuRequested, this, &MainWindow::onGameListEntryContextMenuRequested,
		Qt::QueuedConnection);
	connect(m_game_list_widget, &GameListWidget::addGameDirectoryRequested, this,
		[this]() { getSettingsDialog()->getGameListSettingsWidget()->addSearchDirectory(this); });
}

void MainWindow::connectVMThreadSignals(EmuThread* thread)
{
	connect(m_ui.actionStartFullscreenUI, &QAction::triggered, thread, &EmuThread::startFullscreenUI);
	connect(m_ui.actionStartFullscreenUI2, &QAction::triggered, thread, &EmuThread::startFullscreenUI);
	connect(thread, &EmuThread::messageConfirmed, this, &MainWindow::confirmMessage, Qt::BlockingQueuedConnection);
	connect(thread, &EmuThread::onCreateDisplayRequested, this, &MainWindow::createDisplay, Qt::BlockingQueuedConnection);
	connect(thread, &EmuThread::onUpdateDisplayRequested, this, &MainWindow::updateDisplay, Qt::BlockingQueuedConnection);
	connect(thread, &EmuThread::onDestroyDisplayRequested, this, &MainWindow::destroyDisplay, Qt::BlockingQueuedConnection);
	connect(thread, &EmuThread::onResizeDisplayRequested, this, &MainWindow::displayResizeRequested);
	connect(thread, &EmuThread::onRelativeMouseModeRequested, this, &MainWindow::relativeMouseModeRequested);
	connect(thread, &EmuThread::onVMStarting, this, &MainWindow::onVMStarting);
	connect(thread, &EmuThread::onVMStarted, this, &MainWindow::onVMStarted);
	connect(thread, &EmuThread::onVMPaused, this, &MainWindow::onVMPaused);
	connect(thread, &EmuThread::onVMResumed, this, &MainWindow::onVMResumed);
	connect(thread, &EmuThread::onVMStopped, this, &MainWindow::onVMStopped);
	connect(thread, &EmuThread::onGameChanged, this, &MainWindow::onGameChanged);

	connect(m_ui.actionReset, &QAction::triggered, thread, &EmuThread::resetVM);
	connect(m_ui.actionPause, &QAction::toggled, thread, &EmuThread::setVMPaused);
	connect(m_ui.actionFullscreen, &QAction::triggered, thread, &EmuThread::toggleFullscreen);
	connect(m_ui.actionToggleSoftwareRendering, &QAction::triggered, thread, &EmuThread::toggleSoftwareRendering);
	connect(m_ui.actionDebugger, &QAction::triggered, this, &MainWindow::openDebugger);
	connect(m_ui.actionReloadPatches, &QAction::triggered, thread, &EmuThread::reloadPatches);

	static constexpr GSRendererType renderers[] = {
#ifdef _WIN32
		GSRendererType::DX11, GSRendererType::DX12,
#endif
		GSRendererType::OGL, GSRendererType::VK, GSRendererType::SW, GSRendererType::Null};
	for (GSRendererType renderer : renderers)
	{
		connect(m_ui.menuDebugSwitchRenderer->addAction(QString::fromUtf8(Pcsx2Config::GSOptions::GetRendererName(renderer))),
			&QAction::triggered, [renderer] { g_emu_thread->switchRenderer(renderer); });
	}
}

void MainWindow::recreate()
{
	if (s_vm_valid)
		requestShutdown(false, true, EmuConfig.SaveStateOnShutdown);

	// We need to close input sources, because e.g. DInput uses our window handle.
	g_emu_thread->closeInputSources();

	close();
	g_main_window = nullptr;

	MainWindow* new_main_window = new MainWindow();
	new_main_window->initialize();
	new_main_window->refreshGameList(false);
	new_main_window->show();
	deleteLater();

	// Reload the sources we just closed.
	g_emu_thread->reloadInputSources();
}

void MainWindow::recreateSettings()
{
	QString current_category;
	if (m_settings_dialog)
	{
		const bool was_visible = m_settings_dialog->isVisible();

		current_category = m_settings_dialog->getCategory();
		m_settings_dialog->hide();
		m_settings_dialog->deleteLater();
		m_settings_dialog = nullptr;

		if (!was_visible)
			return;
	}

	doSettings(current_category.toUtf8().constData());
}

void MainWindow::resetSettings(bool ui)
{
	Host::RequestResetSettings(false, true, false, false, ui);

	if (ui)
	{
		// UI reset includes theme (and eventually language).
		// Just updating the theme here, when there's no change, causes Qt to get very confused..
		// So, we'll just tear down everything and recreate. We'll need to do that for language
		// resets eventaully anyway.
		recreate();
	}

	// g_main_window here for recreate() case above.
	g_main_window->recreateSettings();
}

void MainWindow::updateApplicationTheme()
{
	if (!s_unthemed_style_name_set)
	{
		s_unthemed_style_name_set = true;
		s_unthemed_style_name = QApplication::style()->objectName();
		s_unthemed_palette = QApplication::style()->standardPalette();
	}

	setStyleFromSettings();
	setIconThemeFromStyle();
}

void MainWindow::setStyleFromSettings()
{
	const std::string theme(Host::GetBaseStringSettingValue("UI", "Theme", DEFAULT_THEME_NAME));

	if (theme == "fusion")
	{
		qApp->setPalette(s_unthemed_palette);
		qApp->setStyleSheet(QString());
		qApp->setStyle(QStyleFactory::create("Fusion"));
	}
	else if (theme == "darkfusion")
	{
		// adapted from https://gist.github.com/QuantumCD/6245215
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor lighterGray(75, 75, 75);
		const QColor darkGray(53, 53, 53);
		const QColor gray(128, 128, 128);
		const QColor black(25, 25, 25);
		const QColor blue(198, 238, 255);

		QPalette darkPalette;
		darkPalette.setColor(QPalette::Window, darkGray);
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, black);
		darkPalette.setColor(QPalette::AlternateBase, darkGray);
		darkPalette.setColor(QPalette::ToolTipBase, darkGray);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, darkGray);
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::Link, blue);
		darkPalette.setColor(QPalette::Highlight, lighterGray);
		darkPalette.setColor(QPalette::HighlightedText, Qt::white);
		darkPalette.setColor(QPalette::PlaceholderText, QColor(Qt::white).darker());

		darkPalette.setColor(QPalette::Active, QPalette::Button, darkGray);
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

		qApp->setPalette(darkPalette);

		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	else if (theme == "darkfusionblue")
	{
		// adapted from https://gist.github.com/QuantumCD/6245215
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor darkGray(53, 53, 53);
		const QColor gray(128, 128, 128);
		const QColor black(25, 25, 25);
		const QColor blue(198, 238, 255);
		const QColor blue2(0, 88, 208);

		QPalette darkPalette;
		darkPalette.setColor(QPalette::Window, darkGray);
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, black);
		darkPalette.setColor(QPalette::AlternateBase, darkGray);
		darkPalette.setColor(QPalette::ToolTipBase, blue2);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, darkGray);
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::Link, blue);
		darkPalette.setColor(QPalette::Highlight, blue2);
		darkPalette.setColor(QPalette::HighlightedText, Qt::white);
		darkPalette.setColor(QPalette::PlaceholderText, QColor(Qt::white).darker());

		darkPalette.setColor(QPalette::Active, QPalette::Button, darkGray);
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

		qApp->setPalette(darkPalette);

		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	else if (theme == "UntouchedLagoon")
	{
		// Custom palette by RedDevilus, Tame (Light/Washed out) Green as main color and Grayish Blue as complimentary.
		// Alternative white theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor black(25, 25, 25);
		const QColor darkteal(0, 77, 77);
		const QColor teal(0, 128, 128);
		const QColor tameTeal(160, 190, 185);
		const QColor grayBlue(160, 180, 190);

		QPalette standardPalette;
		standardPalette.setColor(QPalette::Window, tameTeal);
		standardPalette.setColor(QPalette::WindowText, black.lighter());
		standardPalette.setColor(QPalette::Base, grayBlue);
		standardPalette.setColor(QPalette::AlternateBase, tameTeal);
		standardPalette.setColor(QPalette::ToolTipBase, tameTeal);
		standardPalette.setColor(QPalette::ToolTipText, grayBlue);
		standardPalette.setColor(QPalette::Text, black);
		standardPalette.setColor(QPalette::Button, tameTeal);
		standardPalette.setColor(QPalette::ButtonText, black);
		standardPalette.setColor(QPalette::Link, black.lighter());
		standardPalette.setColor(QPalette::Highlight, teal);
		standardPalette.setColor(QPalette::HighlightedText, grayBlue.lighter());

		standardPalette.setColor(QPalette::Active, QPalette::Button, tameTeal);
		standardPalette.setColor(QPalette::Disabled, QPalette::ButtonText, darkteal);
		standardPalette.setColor(QPalette::Disabled, QPalette::WindowText, darkteal.lighter());
		standardPalette.setColor(QPalette::Disabled, QPalette::Text, darkteal.lighter());
		standardPalette.setColor(QPalette::Disabled, QPalette::Light, tameTeal);

		qApp->setPalette(standardPalette);

		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	else if (theme == "BabyPastel")
	{
		// Custom palette by RedDevilus, Blue as main color and blue as complimentary.
		// Alternative light theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor gray(150, 150, 150);
		const QColor black(25, 25, 25);
		const QColor redpinkish(200, 75, 132);
		const QColor pink(255, 174, 201);
		const QColor brightPink(255, 230, 255);
		const QColor congoPink(255, 127, 121);
		const QColor blue(221, 225, 239);

		QPalette standardPalette;
		standardPalette.setColor(QPalette::Window, pink);
		standardPalette.setColor(QPalette::WindowText, black);
		standardPalette.setColor(QPalette::Base, brightPink);
		standardPalette.setColor(QPalette::AlternateBase, blue);
		standardPalette.setColor(QPalette::ToolTipBase, pink);
		standardPalette.setColor(QPalette::ToolTipText, brightPink);
		standardPalette.setColor(QPalette::Text, black);
		standardPalette.setColor(QPalette::Button, pink);
		standardPalette.setColor(QPalette::ButtonText, black);
		standardPalette.setColor(QPalette::Link, black);
		standardPalette.setColor(QPalette::Highlight, congoPink);
		standardPalette.setColor(QPalette::HighlightedText, black);

		standardPalette.setColor(QPalette::Active, QPalette::Button, pink);
		standardPalette.setColor(QPalette::Disabled, QPalette::ButtonText, redpinkish);
		standardPalette.setColor(QPalette::Disabled, QPalette::WindowText, redpinkish);
		standardPalette.setColor(QPalette::Disabled, QPalette::Text, redpinkish);
		standardPalette.setColor(QPalette::Disabled, QPalette::Light, gray);

		qApp->setPalette(standardPalette);

		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	else if (theme == "PCSX2Blue")
	{
		// Custom palette by RedDevilus, White as main color and Blue as complimentary.
		// Alternative light theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor blackish(35, 35, 35);
		const QColor darkBlue(73, 97, 177);
		const QColor blue2(80, 120, 200);
		const QColor blue(106, 156, 255);
		const QColor lightBlue(130, 155, 241);

		QPalette standardPalette;
		standardPalette.setColor(QPalette::Window, blue2.lighter());
		standardPalette.setColor(QPalette::WindowText, blackish);
		standardPalette.setColor(QPalette::Base, lightBlue);
		standardPalette.setColor(QPalette::AlternateBase, blue2.lighter());
		standardPalette.setColor(QPalette::ToolTipBase, blue2);
		standardPalette.setColor(QPalette::ToolTipText, Qt::white);
		standardPalette.setColor(QPalette::Text, blackish);
		standardPalette.setColor(QPalette::Button, blue);
		standardPalette.setColor(QPalette::ButtonText, blackish);
		standardPalette.setColor(QPalette::Link, darkBlue);
		standardPalette.setColor(QPalette::Highlight, Qt::white);
		standardPalette.setColor(QPalette::HighlightedText, blackish);

		standardPalette.setColor(QPalette::Active, QPalette::Button, blue);
		standardPalette.setColor(QPalette::Disabled, QPalette::ButtonText, darkBlue);
		standardPalette.setColor(QPalette::Disabled, QPalette::WindowText, darkBlue);
		standardPalette.setColor(QPalette::Disabled, QPalette::Text, darkBlue);
		standardPalette.setColor(QPalette::Disabled, QPalette::Light, darkBlue);

		qApp->setPalette(standardPalette);

		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	else if (theme == "ScarletDevilRed")
	{
		// Custom palette by RedDevilus, Red as main color and Purple as complimentary.
		// Alternative dark theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor darkRed(80, 45, 69);
		const QColor purplishRed(120, 45, 69);
		const QColor brightRed(200, 45, 69);

		QPalette darkPalette;
		darkPalette.setColor(QPalette::Window, darkRed);
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, purplishRed);
		darkPalette.setColor(QPalette::AlternateBase, darkRed);
		darkPalette.setColor(QPalette::ToolTipBase, darkRed);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, purplishRed.darker());
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::Link, brightRed);
		darkPalette.setColor(QPalette::Highlight, brightRed);
		darkPalette.setColor(QPalette::HighlightedText, Qt::white);

		darkPalette.setColor(QPalette::Active, QPalette::Button, purplishRed.darker());
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, brightRed);
		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, brightRed);
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, brightRed);
		darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkRed);

		qApp->setPalette(darkPalette);

		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	else if (theme == "VioletAngelPurple")
	{
		// Custom palette by RedDevilus, Blue as main color and Purple as complimentary.
		// Alternative dark theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor blackishblue(50, 25, 70);
		const QColor darkerPurple(90, 30, 105);
		const QColor nauticalPurple(110, 30, 125);

		QPalette darkPalette;
		darkPalette.setColor(QPalette::Window, blackishblue);
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, nauticalPurple);
		darkPalette.setColor(QPalette::AlternateBase, blackishblue);
		darkPalette.setColor(QPalette::ToolTipBase, nauticalPurple);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, nauticalPurple.darker());
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::Link, darkerPurple.lighter());
		darkPalette.setColor(QPalette::Highlight, darkerPurple.lighter());
		darkPalette.setColor(QPalette::HighlightedText, Qt::white);

		darkPalette.setColor(QPalette::Active, QPalette::Button, nauticalPurple.darker());
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, darkerPurple.lighter());
		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, darkerPurple.lighter());
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, darkerPurple.darker());
		darkPalette.setColor(QPalette::Disabled, QPalette::Light, nauticalPurple);

		qApp->setPalette(darkPalette);

		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	else if (theme == "Ruby")
	{
		// Custom palette by Daisouji, Black as main color and Red as complimentary.
		// Alternative dark (black) theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor gray(128, 128, 128);
		const QColor slate(18, 18, 18);
		const QColor rubyish(172, 21, 31);

		QPalette darkPalette;
		darkPalette.setColor(QPalette::Window, slate);
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, slate.lighter());
		darkPalette.setColor(QPalette::AlternateBase, slate.lighter());
		darkPalette.setColor(QPalette::ToolTipBase, slate);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, slate);
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::Link, Qt::white);
		darkPalette.setColor(QPalette::Highlight, rubyish);
		darkPalette.setColor(QPalette::HighlightedText, Qt::white);

		darkPalette.setColor(QPalette::Active, QPalette::Button, slate);
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Light, slate.lighter());

		qApp->setPalette(darkPalette);

		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	else if (theme == "Sapphire")
	{
		// Custom palette by RedDevilus, Black as main color and Blue as complimentary.
		// Alternative dark (black) theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor gray(128, 128, 128);
		const QColor slate(18, 18, 18);
		const QColor persianBlue(32, 35, 204);

		QPalette darkPalette;
		darkPalette.setColor(QPalette::Window, slate);
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, slate.lighter());
		darkPalette.setColor(QPalette::AlternateBase, slate.lighter());
		darkPalette.setColor(QPalette::ToolTipBase, slate);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, slate);
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::Link, Qt::white);
		darkPalette.setColor(QPalette::Highlight, persianBlue);
		darkPalette.setColor(QPalette::HighlightedText, Qt::white);

		darkPalette.setColor(QPalette::Active, QPalette::Button, slate);
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Light, slate.lighter());

		qApp->setPalette(darkPalette);

		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
	}
	else if (theme == "Custom")
	{

		//Additional Theme option than loads .qss from main PCSX2 Directory
		qApp->setStyle(QStyleFactory::create("Fusion"));

		QString sheet_content;
		QFile sheets(QString::fromStdString(Path::Combine(EmuFolders::DataRoot, "custom.qss")));

		if (sheets.open(QFile::ReadOnly))
		{
			QString sheet_content = QString::fromUtf8(sheets.readAll().data());
			qApp->setStyleSheet(sheet_content);
		}
		else
		{
			qApp->setStyle(QStyleFactory::create("Fusion"));
		}
	}
	else
	{
		qApp->setPalette(s_unthemed_palette);
		qApp->setStyleSheet(QString());
		qApp->setStyle(s_unthemed_style_name);
	}
}

void MainWindow::setIconThemeFromStyle()
{
	QPalette palette = qApp->palette();
	bool dark = palette.windowText().color().value() > palette.window().color().value();
	QIcon::setThemeName(dark ? QStringLiteral("white") : QStringLiteral("black"));
}

void MainWindow::onScreenshotActionTriggered()
{
	g_emu_thread->queueSnapshot(0);
}

void MainWindow::onSaveGSDumpActionTriggered()
{
	g_emu_thread->queueSnapshot(1);
}

void MainWindow::onBlockDumpActionToggled(bool checked)
{
	if (!checked)
		return;

	std::string old_directory(Host::GetBaseStringSettingValue("EmuCore", "BlockDumpSaveDirectory", ""));
	if (old_directory.empty())
		old_directory = FileSystem::GetWorkingDirectory();

	// prompt for a location to save
	const QString new_dir(
		QFileDialog::getExistingDirectory(this, tr("Select location to save block dump:"), QString::fromStdString(old_directory)));
	if (new_dir.isEmpty())
	{
		// disable it again
		m_ui.actionSaveBlockDump->setChecked(false);
		return;
	}

	Host::SetBaseStringSettingValue("EmuCore", "BlockDumpSaveDirectory", new_dir.toUtf8().constData());
	Host::CommitBaseSettingChanges();

	g_emu_thread->applySettings();
}

void MainWindow::onShowAdvancedSettingsToggled(bool checked)
{
	if (checked && !Host::GetBaseBoolSettingValue("UI", "AdvancedSettingsWarningShown", false))
	{
		QCheckBox* cb = new QCheckBox(tr("Do not show again"));
		QMessageBox mb(this);
		mb.setWindowTitle(tr("Show Advanced Settings"));
		mb.setText(tr("Changing advanced settings can have unpredictable effects on games, including graphical glitches, lock-ups, and "
					  "even corrupted save files. "
					  "We do not recommend changing advanced settings unless you know what you are doing, and the implications of changing "
					  "each setting.\n\n"
					  "The PCSX2 team will not provide any support for configurations that modify these settings, you are on your own.\n\n"
					  "Are you sure you want to continue?"));
		mb.setIcon(QMessageBox::Warning);
		mb.addButton(QMessageBox::Yes);
		mb.addButton(QMessageBox::No);
		mb.setDefaultButton(QMessageBox::No);
		mb.setCheckBox(cb);

		if (mb.exec() == QMessageBox::No)
		{
			QSignalBlocker sb(m_ui.actionShowAdvancedSettings);
			m_ui.actionShowAdvancedSettings->setChecked(false);
			return;
		}

		if (cb->isChecked())
		{
			Host::SetBaseBoolSettingValue("UI", "AdvancedSettingsWarningShown", true);
			Host::CommitBaseSettingChanges();
		}
	}

	Host::SetBaseBoolSettingValue("UI", "ShowAdvancedSettings", checked);
	Host::CommitBaseSettingChanges();

	m_ui.menuDebug->menuAction()->setVisible(checked);

	// just recreate the entire settings window, it's easier.
	if (m_settings_dialog)
		recreateSettings();
}

void MainWindow::onToolsVideoCaptureToggled(bool checked)
{
	if (!s_vm_valid)
		return;

	if (!checked)
	{
		g_emu_thread->endCapture();
		return;
	}

	const QString container(QString::fromStdString(
		Host::GetStringSettingValue("EmuCore/GS", "VideoCaptureContainer", Pcsx2Config::GSOptions::DEFAULT_VIDEO_CAPTURE_CONTAINER)));
	const QString filter(tr("%1 Files (*.%2)").arg(container.toUpper()).arg(container));

	QString path(QStringLiteral("%1.%2").arg(QString::fromStdString(GSGetBaseSnapshotFilename())).arg(container));
	path = QFileDialog::getSaveFileName(this, tr("Video Capture"), path, filter);
	if (path.isEmpty())
	{
		QSignalBlocker sb(m_ui.actionToolsVideoCapture);
		m_ui.actionToolsVideoCapture->setChecked(false);
		return;
	}

	g_emu_thread->beginCapture(path);
}

void MainWindow::saveStateToConfig()
{
	if (!isVisible())
		return;

	{
		const QByteArray geometry = saveGeometry();
		const QByteArray geometry_b64 = geometry.toBase64();
		const std::string old_geometry_b64 = Host::GetBaseStringSettingValue("UI", "MainWindowGeometry");
		if (old_geometry_b64 != geometry_b64.constData())
		{
			Host::SetBaseStringSettingValue("UI", "MainWindowGeometry", geometry_b64.constData());
			Host::CommitBaseSettingChanges();
		}
	}

	{
		const QByteArray state = saveState();
		const QByteArray state_b64 = state.toBase64();
		const std::string old_state_b64 = Host::GetBaseStringSettingValue("UI", "MainWindowState");
		if (old_state_b64 != state_b64.constData())
		{
			Host::SetBaseStringSettingValue("UI", "MainWindowState", state_b64.constData());
			Host::CommitBaseSettingChanges();
		}
	}
}

void MainWindow::restoreStateFromConfig()
{
	{
		const std::string geometry_b64 = Host::GetBaseStringSettingValue("UI", "MainWindowGeometry");
		const QByteArray geometry = QByteArray::fromBase64(QByteArray::fromStdString(geometry_b64));
		if (!geometry.isEmpty())
			restoreGeometry(geometry);
	}

	{
		const std::string state_b64 = Host::GetBaseStringSettingValue("UI", "MainWindowState");
		const QByteArray state = QByteArray::fromBase64(QByteArray::fromStdString(state_b64));
		if (!state.isEmpty())
			restoreState(state);

		{
			QSignalBlocker sb(m_ui.actionViewToolbar);
			m_ui.actionViewToolbar->setChecked(!m_ui.toolBar->isHidden());
		}
		{
			QSignalBlocker sb(m_ui.actionViewStatusBar);
			m_ui.actionViewStatusBar->setChecked(!m_ui.statusBar->isHidden());
		}
	}
}

void MainWindow::updateEmulationActions(bool starting, bool running)
{
	const bool starting_or_running = starting || running;

	m_ui.actionStartFile->setDisabled(starting_or_running);
	m_ui.actionStartDisc->setDisabled(starting_or_running);
	m_ui.actionStartBios->setDisabled(starting_or_running);

	m_ui.actionPowerOff->setEnabled(running);
	m_ui.actionPowerOffWithoutSaving->setEnabled(running);
	m_ui.actionReset->setEnabled(running);
	m_ui.actionPause->setEnabled(running);
	m_ui.actionChangeDisc->setEnabled(running);
	m_ui.actionScreenshot->setEnabled(running);
	m_ui.menuChangeDisc->setEnabled(running);

	m_ui.actionSaveState->setEnabled(running);
	m_ui.menuSaveState->setEnabled(running);

	m_ui.actionViewGameProperties->setEnabled(running);

	m_ui.actionToolsVideoCapture->setEnabled(running);
	if (!running && m_ui.actionToolsVideoCapture->isChecked())
		m_ui.actionToolsVideoCapture->setChecked(false);

	m_game_list_widget->setDisabled(starting && !running);

	if (!starting && !running)
		m_ui.actionPause->setChecked(false);

	// scanning needs to be disabled while running
	m_ui.actionScanForNewGames->setDisabled(starting_or_running);
	m_ui.actionRescanAllGames->setDisabled(starting_or_running);
}

void MainWindow::updateDisplayRelatedActions(bool has_surface, bool render_to_main, bool fullscreen)
{
	// rendering to main, or switched to gamelist/grid
	m_ui.actionViewSystemDisplay->setEnabled((has_surface && render_to_main) || (!has_surface && g_host_display));
	m_ui.menuWindowSize->setEnabled(has_surface && !fullscreen);
	m_ui.actionFullscreen->setEnabled(has_surface);

	{
		QSignalBlocker blocker(m_ui.actionFullscreen);
		m_ui.actionFullscreen->setChecked(fullscreen);
	}
}

void MainWindow::updateStatusBarWidgetVisibility()
{
	auto Update = [this](QWidget* widget, bool visible, int stretch) {
		if (widget->isVisible())
		{
			m_ui.statusBar->removeWidget(widget);
			widget->hide();
		}

		if (visible)
		{
			m_ui.statusBar->addPermanentWidget(widget, stretch);
			widget->show();
		}
	};

	Update(m_status_verbose_widget, s_vm_valid, 1);
	Update(m_status_renderer_widget, s_vm_valid, 0);
	Update(m_status_resolution_widget, s_vm_valid, 0);
	Update(m_status_fps_widget, s_vm_valid, 0);
	Update(m_status_vps_widget, s_vm_valid, 0);
}

void MainWindow::updateWindowTitle()
{
	QString suffix(QtHost::GetAppConfigSuffix());
	QString main_title(QtHost::GetAppNameAndVersion() + suffix);
	QString display_title(m_current_game_name + suffix);

	if (!s_vm_valid || m_current_game_name.isEmpty())
		display_title = main_title;
	else if (isRenderingToMain())
		main_title = display_title;

	if (windowTitle() != main_title)
		setWindowTitle(main_title);

	if (m_display_widget && !isRenderingToMain())
	{
		QWidget* container = m_display_container ? static_cast<QWidget*>(m_display_container) : static_cast<QWidget*>(m_display_widget);
		if (container->windowTitle() != display_title)
			container->setWindowTitle(display_title);
	}
}

void MainWindow::updateWindowState(bool force_visible)
{
	// Skip all of this when we're closing, since we don't want to make ourselves visible and cancel it.
	if (m_is_closing)
		return;

	const bool hide_window = !isRenderingToMain() && shouldHideMainWindow();
	const bool disable_resize = Host::GetBoolSettingValue("UI", "DisableWindowResize", false);
	const bool has_window = s_vm_valid || m_display_widget;

	// Need to test both valid and display widget because of startup (vm invalid while window is created).
	const bool visible = force_visible || !hide_window || !has_window;
	if (isVisible() != visible)
		setVisible(visible);

	// No point changing realizability if we're not visible.
	const bool resizeable = force_visible || !disable_resize || !has_window;
	if (visible)
		QtUtils::SetWindowResizeable(this, resizeable);

	// Update the display widget too if rendering separately.
	if (m_display_widget && !isRenderingToMain())
		QtUtils::SetWindowResizeable(getDisplayContainer(), resizeable);
}

void MainWindow::setProgressBar(int current, int total)
{
	const int value = (total != 0) ? ((current * 100) / total) : 0;
	if (m_status_progress_widget->value() != value)
		m_status_progress_widget->setValue(value);

	if (m_status_progress_widget->isVisible())
		return;

	m_status_progress_widget->show();
	m_ui.statusBar->addPermanentWidget(m_status_progress_widget);
}

void MainWindow::clearProgressBar()
{
	if (!m_status_progress_widget->isVisible())
		return;

	m_status_progress_widget->hide();
	m_ui.statusBar->removeWidget(m_status_progress_widget);
}

bool MainWindow::isShowingGameList() const
{
	if (s_use_central_widget)
		return (centralWidget() == m_game_list_widget);
	else
		return (m_ui.mainContainer->currentIndex() == 0);
}

bool MainWindow::isRenderingFullscreen() const
{
	if (!g_host_display || !m_display_widget)
		return false;

	return getDisplayContainer()->isFullScreen() || g_host_display->IsFullscreen();
}

bool MainWindow::isRenderingToMain() const
{
	if (s_use_central_widget)
		return (m_display_widget && centralWidget() == m_display_widget);
	else
		return (m_display_widget && m_ui.mainContainer->indexOf(m_display_widget) == 1);
}

bool MainWindow::shouldHideMouseCursor() const
{
	return (isRenderingFullscreen() && Host::GetBoolSettingValue("UI", "HideMouseCursor", false)) || m_relative_mouse_mode;
}

bool MainWindow::shouldHideMainWindow() const
{
	// NOTE: We can't use isRenderingToMain() here, because this happens post-fullscreen-switch.
	return Host::GetBoolSettingValue("UI", "HideMainWindowWhenRunning", false) ||
		   (g_emu_thread->shouldRenderToMain() && isRenderingFullscreen()) || QtHost::InNoGUIMode();
}

void MainWindow::switchToGameListView()
{
	if (isShowingGameList())
	{
		m_game_list_widget->setFocus();
		return;
	}

	if (m_display_created)
	{
		m_was_paused_on_surface_loss = s_vm_paused;
		if (!s_vm_paused)
			g_emu_thread->setVMPaused(true);

		// switch to surfaceless. we have to wait until the display widget is gone before we swap over.
		g_emu_thread->setSurfaceless(true);
		while (m_display_widget)
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1);
	}
}

void MainWindow::switchToEmulationView()
{
	if (!m_display_created || !isShowingGameList())
		return;

	// we're no longer surfaceless! this will call back to UpdateDisplay(), which will swap the widget out.
	g_emu_thread->setSurfaceless(false);

	// resume if we weren't paused at switch time
	if (s_vm_paused && !m_was_paused_on_surface_loss)
		g_emu_thread->setVMPaused(false);

	if (m_display_widget)
		m_display_widget->setFocus();
}

void MainWindow::refreshGameList(bool invalidate_cache)
{
	// can't do this while the VM is running because of CDVD
	if (s_vm_valid)
		return;

	m_game_list_widget->refresh(invalidate_cache);
}

void MainWindow::cancelGameListRefresh()
{
	m_game_list_widget->cancelRefresh();
}

void MainWindow::invalidateSaveStateCache()
{
	m_save_states_invalidated = true;
}

void MainWindow::reportError(const QString& title, const QString& message)
{
	QMessageBox::critical(this, title, message);
}

bool MainWindow::confirmMessage(const QString& title, const QString& message)
{
	VMLock lock(pauseAndLockVM());
	return (QMessageBox::question(this, title, message) == QMessageBox::Yes);
}

void MainWindow::runOnUIThread(const std::function<void()>& func)
{
	func();
}

bool MainWindow::requestShutdown(bool allow_confirm /* = true */, bool allow_save_to_state /* = true */,
	bool default_save_to_state /* = true */, bool block_until_done /* = false */)
{
	if (!s_vm_valid)
		return true;

	// If we don't have a crc, we can't save state.
	allow_save_to_state &= (m_current_game_crc != 0);
	bool save_state = allow_save_to_state && default_save_to_state;

	// Only confirm on UI thread because we need to display a msgbox.
	if (!m_is_closing && allow_confirm && !GSDumpReplayer::IsReplayingDump() && Host::GetBoolSettingValue("UI", "ConfirmShutdown", true))
	{
		VMLock lock(pauseAndLockVM());

		QMessageBox msgbox(lock.getDialogParent());
		msgbox.setIcon(QMessageBox::Question);
		msgbox.setWindowTitle(tr("Confirm Shutdown"));
		msgbox.setText(tr("Are you sure you want to shut down the virtual machine?"));

		QCheckBox* save_cb = new QCheckBox(tr("Save State For Resume"), &msgbox);
		save_cb->setChecked(save_state);
		save_cb->setEnabled(allow_save_to_state);
		msgbox.setCheckBox(save_cb);
		msgbox.addButton(QMessageBox::Yes);
		msgbox.addButton(QMessageBox::No);
		msgbox.setDefaultButton(QMessageBox::Yes);
		if (msgbox.exec() != QMessageBox::Yes)
			return false;

		save_state = save_cb->isChecked();

		// Don't switch back to fullscreen when we're shutting down anyway.
		lock.cancelResume();
	}

	// This is a little bit annoying. Qt will close everything down if we don't have at least one window visible,
	// but we might not be visible because the user is using render-to-separate and hide. We don't want to always
	// reshow the main window during display updates, because otherwise fullscreen transitions and renderer switches
	// would briefly show and then hide the main window. So instead, we do it on shutdown, here. Except if we're in
	// batch mode, when we're going to exit anyway.
	if (!isRenderingToMain() && isHidden() && !QtHost::InBatchMode() && !g_emu_thread->isRunningFullscreenUI())
		updateWindowState(true);

	// Now we can actually shut down the VM.
	g_emu_thread->shutdownVM(save_state);

	if (block_until_done || m_is_closing || QtHost::InBatchMode())
	{
		// We need to yield here, since the display gets destroyed.
		while (VMManager::GetState() != VMState::Shutdown)
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1);
	}

	if (!m_is_closing && QtHost::InBatchMode())
	{
		// If we don't set the closing flag here, the VM shutdown may not complete by the time closeEvent() is called,
		// leading to a confirm.
		m_is_closing = true;
		QGuiApplication::quit();
	}

	return true;
}

void MainWindow::requestExit()
{
	// this is block, because otherwise closeEvent() will also prompt
	if (!requestShutdown(true, true, EmuConfig.SaveStateOnShutdown, true))
		return;

	// We could use close here, but if we're not visible (e.g. quitting from fullscreen), closing the window
	// doesn't quit the application.
	QGuiApplication::quit();
}

void MainWindow::checkForSettingChanges()
{
	if (m_display_widget)
		updateDisplayWidgetCursor();

	updateWindowState();
}

std::optional<WindowInfo> MainWindow::getWindowInfo()
{
	if (!m_display_widget || isRenderingToMain())
		return QtUtils::GetWindowInfoForWidget(this);
	else if (QWidget* widget = getDisplayContainer())
		return QtUtils::GetWindowInfoForWidget(widget);
	else
		return std::nullopt;
}

void Host::InvalidateSaveStateCache()
{
	QMetaObject::invokeMethod(g_main_window, &MainWindow::invalidateSaveStateCache, Qt::QueuedConnection);
}

void MainWindow::onGameListRefreshProgress(const QString& status, int current, int total)
{
	m_ui.statusBar->showMessage(status);
	setProgressBar(current, total);
}

void MainWindow::onGameListRefreshComplete()
{
	clearProgressBar();
}

void MainWindow::onGameListSelectionChanged()
{
	auto lock = GameList::GetLock();
	const GameList::Entry* entry = m_game_list_widget->getSelectedEntry();
	if (!entry)
		return;

	m_ui.statusBar->showMessage(QString::fromStdString(entry->path));
}

void MainWindow::onGameListEntryActivated()
{
	auto lock = GameList::GetLock();
	const GameList::Entry* entry = m_game_list_widget->getSelectedEntry();
	if (!entry)
		return;

	if (s_vm_valid)
	{
		// change disc on double click
		if (!entry->IsDisc())
		{
			QMessageBox::critical(this, tr("Error"), tr("You must select a disc to change discs."));
			return;
		}

		doDiscChange(CDVD_SourceType::Iso, QString::fromStdString(entry->path));
		return;
	}

	// we might still be saving a resume state...
	VMManager::WaitForSaveStateFlush();

	const std::optional<bool> resume =
		promptForResumeState(QString::fromStdString(VMManager::GetSaveStateFileName(entry->serial.c_str(), entry->crc, -1)));
	if (!resume.has_value())
	{
		// cancelled
		return;
	}

	// only resume if the option is enabled, and we have one for this game
	startGameListEntry(entry, resume.value() ? std::optional<s32>(-1) : std::optional<s32>(), std::nullopt);
}

void MainWindow::onGameListEntryContextMenuRequested(const QPoint& point)
{
	auto lock = GameList::GetLock();
	const GameList::Entry* entry = m_game_list_widget->getSelectedEntry();

	QMenu menu;

	if (entry)
	{
		QAction* action = menu.addAction(tr("Properties..."));
		action->setEnabled(!entry->serial.empty() || entry->type == GameList::EntryType::ELF);
		if (action->isEnabled())
		{
			connect(action, &QAction::triggered, [entry]() {
				SettingsDialog::openGamePropertiesDialog(
					entry, (entry->type != GameList::EntryType::ELF) ? std::string_view(entry->serial) : std::string_view(), entry->crc);
			});
		}

		action = menu.addAction(tr("Open Containing Directory..."));
		connect(action, &QAction::triggered, [this, entry]() {
			const QFileInfo fi(QString::fromStdString(entry->path));
			QtUtils::OpenURL(this, QUrl::fromLocalFile(fi.absolutePath()));
		});

		action = menu.addAction(tr("Set Cover Image..."));
		connect(action, &QAction::triggered, [this, entry]() { setGameListEntryCoverImage(entry); });

		connect(menu.addAction(tr("Exclude From List")), &QAction::triggered,
			[this, entry]() { getSettingsDialog()->getGameListSettingsWidget()->addExcludedPath(entry->path); });

		connect(menu.addAction(tr("Reset Play Time")), &QAction::triggered, [this, entry]() { clearGameListEntryPlayTime(entry); });

		menu.addSeparator();

		if (!s_vm_valid)
		{
			action = menu.addAction(tr("Default Boot"));
			connect(action, &QAction::triggered, [this, entry]() { startGameListEntry(entry); });

			// Make bold to indicate it's the default choice when double-clicking
			if (!VMManager::HasSaveStateInSlot(entry->serial.c_str(), entry->crc, -1))
				QtUtils::MarkActionAsDefault(action);

			action = menu.addAction(tr("Fast Boot"));
			connect(action, &QAction::triggered, [this, entry]() { startGameListEntry(entry, std::nullopt, true); });

			action = menu.addAction(tr("Full Boot"));
			connect(action, &QAction::triggered, [this, entry]() { startGameListEntry(entry, std::nullopt, false); });

			if (m_ui.menuDebug->menuAction()->isVisible())
			{
				// TODO: Hook this up once it's implemented.
				action = menu.addAction(tr("Boot and Debug"));
			}

			menu.addSeparator();
			populateLoadStateMenu(&menu, QString::fromStdString(entry->path), QString::fromStdString(entry->serial), entry->crc);
		}
		else if (entry->IsDisc())
		{
			action = menu.addAction(tr("Change Disc"));
			connect(action, &QAction::triggered, [this, entry]() {
				g_emu_thread->changeDisc(CDVD_SourceType::Iso, QString::fromStdString(entry->path));
				switchToEmulationView();
			});
			QtUtils::MarkActionAsDefault(action);
		}

		menu.addSeparator();
	}

	connect(menu.addAction(tr("Add Search Directory...")), &QAction::triggered,
		[this]() { getSettingsDialog()->getGameListSettingsWidget()->addSearchDirectory(this); });

	menu.exec(point);
}

void MainWindow::onStartFileActionTriggered()
{
	const QString path(
		QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Start File"), QString(), tr(OPEN_FILE_FILTER), nullptr)));
	if (path.isEmpty())
		return;

	doStartFile(std::nullopt, path);
}

void MainWindow::onStartDiscActionTriggered()
{
	QString path(getDiscDevicePath(tr("Start Disc")));
	if (path.isEmpty())
		return;

	doStartFile(CDVD_SourceType::Disc, path);
}

void MainWindow::onStartBIOSActionTriggered()
{
	std::shared_ptr<VMBootParameters> params = std::make_shared<VMBootParameters>();
	g_emu_thread->startVM(std::move(params));
}

void MainWindow::onChangeDiscFromFileActionTriggered()
{
	VMLock lock(pauseAndLockVM());
	QString filename =
		QFileDialog::getOpenFileName(lock.getDialogParent(), tr("Select Disc Image"), QString(), tr(DISC_IMAGE_FILTER), nullptr);
	if (filename.isEmpty())
		return;

	g_emu_thread->changeDisc(CDVD_SourceType::Iso, filename);
}

void MainWindow::onChangeDiscFromGameListActionTriggered()
{
	m_was_disc_change_request = true;
	switchToGameListView();
}

void MainWindow::onChangeDiscFromDeviceActionTriggered()
{
	QString path(getDiscDevicePath(tr("Change Disc")));
	if (path.isEmpty())
		return;

	g_emu_thread->changeDisc(CDVD_SourceType::Disc, path);
}

void MainWindow::onRemoveDiscActionTriggered()
{
	g_emu_thread->changeDisc(CDVD_SourceType::NoDisc, QString());
}

void MainWindow::onChangeDiscMenuAboutToShow()
{
	// TODO: This is where we would populate the playlist if there is one.
}

void MainWindow::onChangeDiscMenuAboutToHide()
{
}

void MainWindow::onLoadStateMenuAboutToShow()
{
	if (m_save_states_invalidated)
		updateSaveStateMenus(m_current_disc_path, m_current_game_serial, m_current_game_crc);
}

void MainWindow::onSaveStateMenuAboutToShow()
{
	if (m_save_states_invalidated)
		updateSaveStateMenus(m_current_disc_path, m_current_game_serial, m_current_game_crc);
}

void MainWindow::onViewToolbarActionToggled(bool checked)
{
	Host::SetBaseBoolSettingValue("UI", "ShowToolbar", checked);
	Host::CommitBaseSettingChanges();
	m_ui.toolBar->setVisible(checked);
}

void MainWindow::onViewLockToolbarActionToggled(bool checked)
{
	Host::SetBaseBoolSettingValue("UI", "LockToolbar", checked);
	Host::CommitBaseSettingChanges();
	m_ui.toolBar->setMovable(!checked);
}

void MainWindow::onViewStatusBarActionToggled(bool checked)
{
	Host::SetBaseBoolSettingValue("UI", "ShowStatusBar", checked);
	Host::CommitBaseSettingChanges();
	m_ui.statusBar->setVisible(checked);
}

void MainWindow::onViewGameListActionTriggered()
{
	switchToGameListView();
	m_game_list_widget->showGameList();
}

void MainWindow::onViewGameGridActionTriggered()
{
	switchToGameListView();
	m_game_list_widget->showGameGrid();
}

void MainWindow::onViewSystemDisplayTriggered()
{
	if (m_display_created)
		switchToEmulationView();
}

void MainWindow::onViewGamePropertiesActionTriggered()
{
	if (!s_vm_valid)
		return;

	// prefer to use a game list entry, if we have one, that way the summary is populated
	if (!m_current_disc_path.isEmpty() || !m_current_elf_override.isEmpty())
	{
		auto lock = GameList::GetLock();
		const GameList::Entry* entry = m_current_elf_override.isEmpty() ?
										   GameList::GetEntryForPath(m_current_disc_path.toUtf8().constData()) :
										   GameList::GetEntryForPath(m_current_elf_override.toUtf8().constData());
		if (entry)
		{
			SettingsDialog::openGamePropertiesDialog(
				entry, m_current_elf_override.isEmpty() ? std::string_view(entry->serial) : std::string_view(), entry->crc);
			return;
		}
	}

	// open properties for the current running file (isn't in the game list)
	if (m_current_game_crc != 0)
		SettingsDialog::openGamePropertiesDialog(nullptr, m_current_game_serial.toStdString(), m_current_game_crc);
}

void MainWindow::onGitHubRepositoryActionTriggered()
{
	QtUtils::OpenURL(this, AboutDialog::getGitHubRepositoryUrl());
}

void MainWindow::onSupportForumsActionTriggered()
{
	QtUtils::OpenURL(this, AboutDialog::getSupportForumsUrl());
}

void MainWindow::onDiscordServerActionTriggered()
{
	QtUtils::OpenURL(this, AboutDialog::getDiscordServerUrl());
}

void MainWindow::onAboutActionTriggered()
{
	AboutDialog about(this);
	about.exec();
}

void MainWindow::onCheckForUpdatesActionTriggered()
{
	// Wipe out the last version, that way it displays the update if we've previously skipped it.
	Host::RemoveBaseSettingValue("AutoUpdater", "LastVersion");
	Host::CommitBaseSettingChanges();
	checkForUpdates(true);
}

void MainWindow::checkForUpdates(bool display_message)
{
	if (!AutoUpdaterDialog::isSupported())
	{
		if (display_message)
		{
			QMessageBox mbox(this);
			mbox.setWindowTitle(tr("Updater Error"));
			mbox.setTextFormat(Qt::RichText);

			QString message;
#ifdef _WIN32
			message = tr("<p>Sorry, you are trying to update a PCSX2 version which is not an official GitHub release. To "
						 "prevent incompatibilities, the auto-updater is only enabled on official builds.</p>"
						 "<p>To obtain an official build, please download from the link below:</p>"
						 "<p><a href=\"https://pcsx2.net/downloads/\">https://pcsx2.net/downloads/</a></p>");
#else
			message = tr("Automatic updating is not supported on the current platform.");
#endif

			mbox.setText(message);
			mbox.setIcon(QMessageBox::Critical);
			mbox.exec();
		}

		return;
	}

	if (m_auto_updater_dialog)
		return;

	m_auto_updater_dialog = new AutoUpdaterDialog(this);
	connect(m_auto_updater_dialog, &AutoUpdaterDialog::updateCheckCompleted, this, &MainWindow::onUpdateCheckComplete);
	m_auto_updater_dialog->queueUpdateCheck(display_message);
}

void MainWindow::onUpdateCheckComplete()
{
	if (!m_auto_updater_dialog)
		return;

	m_auto_updater_dialog->deleteLater();
	m_auto_updater_dialog = nullptr;
}

void MainWindow::startupUpdateCheck()
{
	if (!Host::GetBaseBoolSettingValue("AutoUpdater", "CheckAtStartup", true))
		return;

	checkForUpdates(false);
}

void MainWindow::onToolsOpenDataDirectoryTriggered()
{
	const QString path(QString::fromStdString(EmuFolders::DataRoot));
	QtUtils::OpenURL(this, QUrl::fromLocalFile(path));
}

void MainWindow::onToolsCoverDownloaderTriggered()
{
	CoverDownloadDialog dlg(this);
	connect(&dlg, &CoverDownloadDialog::coverRefreshRequested, m_game_list_widget, &GameListWidget::refreshGridCovers);
	dlg.exec();
}

void MainWindow::updateTheme()
{
	updateApplicationTheme();
	m_game_list_widget->refreshImages();
}

void MainWindow::onInputRecNewActionTriggered()
{
	const bool wasPaused = s_vm_paused;
	const bool wasRunning = s_vm_valid;
	if (wasRunning && !wasPaused)
	{
		g_emu_thread->setVMPaused(true);
	}

	NewInputRecordingDlg dlg(this);
	const auto result = dlg.exec();

	if (result == QDialog::Accepted)
	{
		Host::RunOnCPUThread(
			[&, filePath = dlg.getFilePath(), fromSavestate = dlg.getInputRecType() == InputRecording::Type::FROM_SAVESTATE,
				authorName = dlg.getAuthorName()]() {
				if (g_InputRecording.create(filePath, fromSavestate, authorName))
				{
					QtHost::RunOnUIThread([&]() {
						m_ui.actionInputRecNew->setEnabled(false);
						m_ui.actionInputRecStop->setEnabled(true);
						m_ui.actionReset->setEnabled(!g_InputRecording.isTypeSavestate());
					});
				}
			});
	}

	if (wasRunning && !wasPaused)
	{
		g_emu_thread->setVMPaused(false);
	}
}

void MainWindow::onInputRecPlayActionTriggered()
{
	const bool wasPaused = s_vm_paused;

	if (!wasPaused)
	{
		g_emu_thread->setVMPaused(true);
	}

	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setWindowTitle("Select a File");
	dialog.setNameFilter(tr("Input Recording Files (*.p2m2)"));
	QStringList fileNames;
	if (dialog.exec())
	{
		fileNames = dialog.selectedFiles();
	}
	else
	{
		if (!wasPaused)
		{
			g_emu_thread->setVMPaused(false);
			return;
		}
	}

	if (fileNames.length() > 0)
	{
		if (g_InputRecording.isActive())
		{
			Host::RunOnCPUThread([]() { g_InputRecording.stop(); });
			m_ui.actionInputRecStop->setEnabled(false);
		}
		Host::RunOnCPUThread([&, filename = fileNames.first().toStdString()]() {
			if (g_InputRecording.play(filename))
			{
				QtHost::RunOnUIThread([&]() {
					m_ui.actionInputRecNew->setEnabled(false);
					m_ui.actionInputRecStop->setEnabled(true);
					m_ui.actionReset->setEnabled(!g_InputRecording.isTypeSavestate());
				});
			}
		});
	}
}

void MainWindow::onInputRecStopActionTriggered()
{
	if (g_InputRecording.isActive())
	{
		Host::RunOnCPUThread([&]() {
			g_InputRecording.stop();
			QtHost::RunOnUIThread([&]() {
				m_ui.actionInputRecNew->setEnabled(true);
				m_ui.actionInputRecStop->setEnabled(false);
				m_ui.actionReset->setEnabled(true);
			});
		});
	}
}

void MainWindow::onInputRecOpenSettingsTriggered()
{
	// TODO - Vaser - Implement
}

InputRecordingViewer* MainWindow::getInputRecordingViewer()
{
	if (!m_input_recording_viewer)
	{
		m_input_recording_viewer = new InputRecordingViewer(this);
	}

	return m_input_recording_viewer;
}

void MainWindow::updateInputRecordingActions(bool started)
{
	m_ui.actionInputRecNew->setEnabled(started);
	m_ui.actionInputRecPlay->setEnabled(started);
}

void MainWindow::onInputRecOpenViewer()
{
	InputRecordingViewer* viewer = getInputRecordingViewer();
	if (!viewer->isVisible())
	{
		viewer->show();
	}
}


void MainWindow::onVMStarting()
{
	s_vm_valid = true;
	updateEmulationActions(true, false);
	updateWindowTitle();

	// prevent loading state until we're fully initialized
	updateSaveStateMenus(QString(), QString(), 0);
}

void MainWindow::onVMStarted()
{
	s_vm_valid = true;
	m_was_disc_change_request = false;
	updateEmulationActions(true, true);
	updateWindowTitle();
	updateStatusBarWidgetVisibility();
	updateInputRecordingActions(true);
}

void MainWindow::onVMPaused()
{
	// update UI
	{
		QSignalBlocker sb(m_ui.actionPause);
		m_ui.actionPause->setChecked(true);
	}

	s_vm_paused = true;
	updateWindowTitle();
	updateStatusBarWidgetVisibility();
	m_last_fps_status = m_status_verbose_widget->text();
	m_status_verbose_widget->setText(tr("Paused"));
	if (m_display_widget)
		updateDisplayWidgetCursor();
}

void MainWindow::onVMResumed()
{
	// update UI
	{
		QSignalBlocker sb(m_ui.actionPause);
		m_ui.actionPause->setChecked(false);
	}

	s_vm_paused = false;
	m_was_disc_change_request = false;
	updateWindowTitle();
	updateStatusBarWidgetVisibility();
	m_status_verbose_widget->setText(m_last_fps_status);
	m_last_fps_status = QString();
	if (m_display_widget)
	{
		updateDisplayWidgetCursor();
		m_display_widget->setFocus();
	}
}

void MainWindow::onVMStopped()
{
	s_vm_valid = false;
	s_vm_paused = false;
	m_last_fps_status = QString();
	updateEmulationActions(false, false);
	updateWindowTitle();
	updateWindowState();
	updateStatusBarWidgetVisibility();
	updateInputRecordingActions(false);

	if (m_display_widget)
		updateDisplayWidgetCursor();
	else
		switchToGameListView();

	// reload played time
	if (m_game_list_widget->isShowingGameList())
		m_game_list_widget->refresh(false);
}

void MainWindow::onGameChanged(const QString& path, const QString& elf_override, const QString& serial, const QString& name, quint32 crc)
{
	m_current_disc_path = path;
	m_current_elf_override = elf_override;
	m_current_game_serial = serial;
	m_current_game_name = name;
	m_current_game_crc = crc;
	updateWindowTitle();
	updateSaveStateMenus(path, serial, crc);
}

void MainWindow::showEvent(QShowEvent* event)
{
	QMainWindow::showEvent(event);

	// This is a bit silly, but for some reason resizing *before* the window is shown
	// gives the incorrect sizes for columns, if you set the style before setting up
	// the rest of the window... so, instead, let's just force it to be resized on show.
	if (isShowingGameList())
		m_game_list_widget->resizeTableViewColumnsToFit();

#ifdef ENABLE_RAINTEGRATION
	if (Achievements::IsUsingRAIntegration())
		Achievements::RAIntegration::MainWindowChanged((void*)winId());
#endif
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	if (!requestShutdown(true, true, EmuConfig.SaveStateOnShutdown, true))
	{
		event->ignore();
		return;
	}

	saveStateToConfig();
	m_is_closing = true;

	QMainWindow::closeEvent(event);
}

static QString getFilenameFromMimeData(const QMimeData* md)
{
	QString filename;
	if (md->hasUrls())
	{
		// only one url accepted
		const QList<QUrl> urls(md->urls());
		if (urls.size() == 1)
			filename = urls.front().toLocalFile();
	}

	return filename;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	const std::string filename(getFilenameFromMimeData(event->mimeData()).toStdString());

	// allow save states being dragged in
	if (!VMManager::IsLoadableFileName(filename) && !VMManager::IsSaveStateFileName(filename))
		return;

	event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
	const QString filename(getFilenameFromMimeData(event->mimeData()));
	const std::string filename_str(filename.toStdString());
	if (VMManager::IsSaveStateFileName(filename_str))
	{
		// can't load a save state without a current VM
		if (s_vm_valid)
		{
			event->acceptProposedAction();
			g_emu_thread->loadState(filename);
		}
		else
		{
			QMessageBox::critical(this, tr("Load State Failed"), tr("Cannot load a save state without a running VM."));
		}
	}
	else if (VMManager::IsLoadableFileName(filename_str))
	{
		// if we're already running, do a disc change, otherwise start
		event->acceptProposedAction();
		if (s_vm_valid)
			doDiscChange(CDVD_SourceType::Iso, filename);
		else
			doStartFile(std::nullopt, filename);
	}
}

void MainWindow::registerForDeviceNotifications()
{
#ifdef _WIN32
	// We use these notifications to detect when a controller is connected or disconnected.
	DEV_BROADCAST_DEVICEINTERFACE_W filter = {sizeof(DEV_BROADCAST_DEVICEINTERFACE_W), DBT_DEVTYP_DEVICEINTERFACE};
	m_device_notification_handle =
		RegisterDeviceNotificationW((HANDLE)winId(), &filter, DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
#endif
}

void MainWindow::unregisterForDeviceNotifications()
{
#ifdef _WIN32
	if (!m_device_notification_handle)
		return;

	UnregisterDeviceNotification(static_cast<HDEVNOTIFY>(m_device_notification_handle));
	m_device_notification_handle = nullptr;
#endif
}

#ifdef _WIN32

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
	static constexpr const char win_type[] = "windows_generic_MSG";
	if (eventType == QByteArray(win_type, sizeof(win_type) - 1))
	{
		const MSG* msg = static_cast<const MSG*>(message);
		if (msg->message == WM_DEVICECHANGE && msg->wParam == DBT_DEVNODES_CHANGED)
		{
			g_emu_thread->reloadInputDevices();
			*result = 1;
			return true;
		}
	}

	return QMainWindow::nativeEvent(eventType, message, result);
}

#endif

DisplayWidget* MainWindow::createDisplay(bool fullscreen, bool render_to_main)
{
	DevCon.WriteLn("createDisplay(%u, %u)", static_cast<u32>(fullscreen), static_cast<u32>(render_to_main));

	if (!g_host_display)
		return nullptr;

	const std::string fullscreen_mode(Host::GetBaseStringSettingValue("EmuCore/GS", "FullscreenMode", ""));
	const bool is_exclusive_fullscreen = (fullscreen && !fullscreen_mode.empty() && g_host_display->SupportsFullscreen());

	createDisplayWidget(fullscreen, render_to_main, is_exclusive_fullscreen);

	// we need the surface visible.. this might be able to be replaced with something else
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	std::optional<WindowInfo> wi = m_display_widget->getWindowInfo();
	if (!wi.has_value())
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to get window info from widget"));
		destroyDisplayWidget(true);
		return nullptr;
	}

	g_emu_thread->connectDisplaySignals(m_display_widget);

	if (!g_host_display->CreateDevice(wi.value(), Host::GetEffectiveVSyncMode()))
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to create host display device context."));
		destroyDisplayWidget(true);
		return nullptr;
	}

	m_display_created = true;

	if (is_exclusive_fullscreen)
		setDisplayFullscreen(fullscreen_mode);

	updateWindowTitle();
	updateWindowState();

	m_ui.actionStartFullscreenUI->setEnabled(false);
	m_ui.actionStartFullscreenUI2->setEnabled(false);

	updateDisplayWidgetCursor();
	m_display_widget->setFocus();

	g_host_display->DoneCurrent();
	return m_display_widget;
}

DisplayWidget* MainWindow::updateDisplay(bool fullscreen, bool render_to_main, bool surfaceless)
{
	DevCon.WriteLn("updateDisplay() fullscreen=%s render_to_main=%s surfaceless=%s", fullscreen ? "true" : "false",
		render_to_main ? "true" : "false", surfaceless ? "true" : "false");

	QWidget* container = m_display_container ? static_cast<QWidget*>(m_display_container) : static_cast<QWidget*>(m_display_widget);
	const bool is_fullscreen = isRenderingFullscreen();
	const bool is_rendering_to_main = isRenderingToMain();
	const std::string fullscreen_mode(Host::GetBaseStringSettingValue("EmuCore/GS", "FullscreenMode", ""));
	const bool is_exclusive_fullscreen = (fullscreen && !fullscreen_mode.empty() && g_host_display->SupportsFullscreen());
	const bool changing_surfaceless = (!m_display_widget != surfaceless);
	if (fullscreen == is_fullscreen && is_rendering_to_main == render_to_main && !changing_surfaceless)
		return m_display_widget;

	// Skip recreating the surface if we're just transitioning between fullscreen and windowed with render-to-main off.
	// .. except on Wayland, where everything tends to break if you don't recreate.
	const bool has_container = (m_display_container != nullptr);
	const bool needs_container = DisplayContainer::isNeeded(fullscreen, render_to_main);
	if (!is_rendering_to_main && !render_to_main && !is_exclusive_fullscreen && has_container == needs_container && !needs_container &&
		!changing_surfaceless)
	{
		DevCon.WriteLn("Toggling to %s without recreating surface", (fullscreen ? "fullscreen" : "windowed"));
		if (g_host_display->IsFullscreen())
			g_host_display->SetFullscreen(false, 0, 0, 0.0f);

		// since we don't destroy the display widget, we need to save it here
		if (!is_fullscreen && !is_rendering_to_main)
			saveDisplayWindowGeometryToConfig();

		if (fullscreen)
		{
			container->showFullScreen();
		}
		else
		{
			restoreDisplayWindowGeometryFromConfig();
			container->showNormal();
		}

		updateDisplayWidgetCursor();
		m_display_widget->setFocus();
		updateWindowState();

		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		return m_display_widget;
	}

	g_host_display->DestroySurface();

	destroyDisplayWidget(surfaceless);

	// if we're going to surfaceless, we're done here
	if (surfaceless)
		return nullptr;

	createDisplayWidget(fullscreen, render_to_main, is_exclusive_fullscreen);

	std::optional<WindowInfo> wi = m_display_widget->getWindowInfo();
	if (!wi.has_value())
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to get new window info from widget"));
		destroyDisplayWidget(true);
		return nullptr;
	}

	g_emu_thread->connectDisplaySignals(m_display_widget);

	if (!g_host_display->ChangeWindow(wi.value()))
		pxFailRel("Failed to recreate surface on new widget.");

	if (is_exclusive_fullscreen)
		setDisplayFullscreen(fullscreen_mode);

	updateWindowTitle();
	updateWindowState();

	updateDisplayWidgetCursor();
	m_display_widget->setFocus();

	return m_display_widget;
}

void MainWindow::createDisplayWidget(bool fullscreen, bool render_to_main, bool is_exclusive_fullscreen)
{
	// If we're rendering to main and were hidden (e.g. coming back from fullscreen),
	// make sure we're visible before trying to add ourselves. Otherwise Wayland breaks.
	if (!fullscreen && render_to_main && !isVisible())
	{
		setVisible(true);
		QGuiApplication::sync();
	}

	QWidget* container;
	if (DisplayContainer::isNeeded(fullscreen, render_to_main))
	{
		m_display_container = new DisplayContainer();
		m_display_widget = new DisplayWidget(m_display_container);
		m_display_container->setDisplayWidget(m_display_widget);
		container = m_display_container;
	}
	else
	{
		m_display_widget = new DisplayWidget((!fullscreen && render_to_main) ? getContentParent() : nullptr);
		container = m_display_widget;
	}

	if (fullscreen || !render_to_main)
	{
		container->setWindowTitle(windowTitle());
		container->setWindowIcon(windowIcon());
	}

	if (fullscreen)
	{
		// Don't risk doing this on Wayland, it really doesn't like window state changes,
		// and positioning has no effect anyway.
		if (!s_use_central_widget)
			restoreDisplayWindowGeometryFromConfig();

		if (!is_exclusive_fullscreen)
			container->showFullScreen();
		else
			container->showNormal();
	}
	else if (!render_to_main)
	{
		restoreDisplayWindowGeometryFromConfig();
		container->showNormal();
	}
	else if (s_use_central_widget)
	{
		m_game_list_widget->setVisible(false);
		takeCentralWidget();
		m_game_list_widget->setParent(this); // takeCentralWidget() removes parent
		setCentralWidget(m_display_widget);
		m_display_widget->setFocus();
		update();
	}
	else
	{
		pxAssertRel(m_ui.mainContainer->count() == 1, "Has no display widget");
		m_ui.mainContainer->addWidget(container);
		m_ui.mainContainer->setCurrentIndex(1);
	}

	updateDisplayRelatedActions(true, render_to_main, fullscreen);

	// We need the surface visible.
	QGuiApplication::sync();
}

void MainWindow::displayResizeRequested(qint32 width, qint32 height)
{
	if (!m_display_widget)
		return;

	// unapply the pixel scaling factor for hidpi
	const float dpr = devicePixelRatioF();
	width = static_cast<qint32>(std::max(static_cast<int>(std::lroundf(static_cast<float>(width) / dpr)), 1));
	height = static_cast<qint32>(std::max(static_cast<int>(std::lroundf(static_cast<float>(height) / dpr)), 1));

	if (m_display_container || !m_display_widget->parent())
	{
		// no parent - rendering to separate window. easy.
		QtUtils::ResizePotentiallyFixedSizeWindow(getDisplayContainer(), width, height);
		return;
	}

	// we are rendering to the main window. we have to add in the extra height from the toolbar/status bar.
	const s32 extra_height = this->height() - m_display_widget->height();
	QtUtils::ResizePotentiallyFixedSizeWindow(this, width, height + extra_height);
}

void MainWindow::relativeMouseModeRequested(bool enabled)
{
	if (m_relative_mouse_mode == enabled)
		return;

	m_relative_mouse_mode = enabled;
	if (s_vm_valid && !s_vm_paused)
		updateDisplayWidgetCursor();
}

void MainWindow::destroyDisplay()
{
	// Now we can safely destroy the display window.
	destroyDisplayWidget(true);
	m_display_created = false;

	m_ui.actionViewSystemDisplay->setEnabled(false);
	m_ui.actionFullscreen->setEnabled(false);
	m_ui.actionStartFullscreenUI->setEnabled(true);
	m_ui.actionStartFullscreenUI2->setEnabled(true);
}

void MainWindow::destroyDisplayWidget(bool show_game_list)
{
	if (!m_display_widget)
		return;

	if (!isRenderingFullscreen() && !isRenderingToMain())
		saveDisplayWindowGeometryToConfig();

	if (m_display_container)
		m_display_container->removeDisplayWidget();

	if (isRenderingToMain())
	{
		if (s_use_central_widget)
		{
			pxAssertRel(centralWidget() == m_display_widget, "Display widget is currently central");
			takeCentralWidget();
			if (show_game_list)
			{
				m_game_list_widget->setVisible(true);
				setCentralWidget(m_game_list_widget);
				m_game_list_widget->resizeTableViewColumnsToFit();
			}
		}
		else
		{
			pxAssertRel(m_ui.mainContainer->indexOf(m_display_widget) == 1, "Display widget in stack");
			m_ui.mainContainer->removeWidget(m_display_widget);
			if (show_game_list)
			{
				m_ui.mainContainer->setCurrentIndex(0);
				m_game_list_widget->resizeTableViewColumnsToFit();
			}
		}
	}

	if (m_display_widget)
	{
		m_display_widget->deleteLater();
		m_display_widget = nullptr;
	}

	if (m_display_container)
	{
		m_display_container->deleteLater();
		m_display_container = nullptr;
	}

	updateDisplayRelatedActions(false, false, false);
}

void MainWindow::updateDisplayWidgetCursor()
{
	m_display_widget->updateRelativeMode(s_vm_valid && !s_vm_paused && m_relative_mouse_mode);
	m_display_widget->updateCursor(s_vm_valid && !s_vm_paused && shouldHideMouseCursor());
}

void MainWindow::focusDisplayWidget()
{
	if (!m_display_widget || centralWidget() != m_display_widget)
		return;

	m_display_widget->setFocus();
}

QWidget* MainWindow::getDisplayContainer() const
{
	return (m_display_container ? static_cast<QWidget*>(m_display_container) : static_cast<QWidget*>(m_display_widget));
}

void MainWindow::saveDisplayWindowGeometryToConfig()
{
	QWidget* container = getDisplayContainer();
	if (container->windowState() & Qt::WindowFullScreen)
	{
		// if we somehow ended up here, don't save the fullscreen state to the config
		return;
	}

	const QByteArray geometry = getDisplayContainer()->saveGeometry();
	const QByteArray geometry_b64 = geometry.toBase64();
	const std::string old_geometry_b64 = Host::GetBaseStringSettingValue("UI", "DisplayWindowGeometry");
	if (old_geometry_b64 != geometry_b64.constData())
	{
		Host::SetBaseStringSettingValue("UI", "DisplayWindowGeometry", geometry_b64.constData());
		Host::CommitBaseSettingChanges();
	}
}

void MainWindow::restoreDisplayWindowGeometryFromConfig()
{
	const std::string geometry_b64 = Host::GetBaseStringSettingValue("UI", "DisplayWindowGeometry");
	const QByteArray geometry = QByteArray::fromBase64(QByteArray::fromStdString(geometry_b64));
	QWidget* container = getDisplayContainer();
	if (!geometry.isEmpty())
	{
		container->restoreGeometry(geometry);

		// make sure we're not loading a dodgy config which had fullscreen set...
		container->setWindowState(container->windowState() & ~(Qt::WindowFullScreen | Qt::WindowActive));
	}
	else
	{
		// default size
		container->resize(640, 480);
	}
}

void MainWindow::setDisplayFullscreen(const std::string& fullscreen_mode)
{
	u32 width, height;
	float refresh_rate;
	if (HostDisplay::ParseFullscreenMode(fullscreen_mode, &width, &height, &refresh_rate))
	{
		if (g_host_display->SetFullscreen(true, width, height, refresh_rate))
		{
			Host::AddOSDMessage("Acquired exclusive fullscreen.", Host::OSD_INFO_DURATION);
		}
		else
		{
			Host::AddOSDMessage("Failed to acquire exclusive fullscreen.", Host::OSD_WARNING_DURATION);
		}
	}
}

SettingsDialog* MainWindow::getSettingsDialog()
{
	if (!m_settings_dialog)
	{
		m_settings_dialog = new SettingsDialog(this);
		connect(m_settings_dialog->getInterfaceSettingsWidget(), &InterfaceSettingsWidget::themeChanged, this, &MainWindow::updateTheme);
	}

	return m_settings_dialog;
}

void MainWindow::doSettings(const char* category /* = nullptr */)
{
	SettingsDialog* dlg = getSettingsDialog();
	if (!dlg->isVisible())
	{
		dlg->setModal(false);
		dlg->show();
	}

	if (category)
		dlg->setCategory(category);
}

DebuggerWindow* MainWindow::getDebuggerWindow()
{
	if (!m_debugger_window)
		m_debugger_window = new DebuggerWindow(this);

	return m_debugger_window;
}

void MainWindow::openDebugger()
{
	DebuggerWindow* dwnd = getDebuggerWindow();
	dwnd->isVisible() ? dwnd->hide() : dwnd->show();
}

ControllerSettingsDialog* MainWindow::getControllerSettingsDialog()
{
	if (!m_controller_settings_dialog)
		m_controller_settings_dialog = new ControllerSettingsDialog(this);

	return m_controller_settings_dialog;
}

void MainWindow::doControllerSettings(ControllerSettingsDialog::Category category)
{
	ControllerSettingsDialog* dlg = getControllerSettingsDialog();
	if (!dlg->isVisible())
	{
		dlg->setModal(false);
		dlg->show();
	}

	if (category != ControllerSettingsDialog::Category::Count)
		dlg->setCategory(category);
}

QString MainWindow::getDiscDevicePath(const QString& title)
{
	QString ret;

	const std::vector<std::string> devices(GetOpticalDriveList());
	if (devices.empty())
	{
		QMessageBox::critical(this, title,
			tr("Could not find any CD/DVD-ROM devices. Please ensure you have a drive connected and "
			   "sufficient permissions to access it."));
		return ret;
	}

	// if there's only one, select it automatically
	if (devices.size() == 1)
	{
		ret = QString::fromStdString(devices.front());
		return ret;
	}

	QStringList input_options;
	for (const std::string& name : devices)
		input_options.append(QString::fromStdString(name));

	QInputDialog input_dialog(this);
	input_dialog.setWindowTitle(title);
	input_dialog.setLabelText(tr("Select disc drive:"));
	input_dialog.setInputMode(QInputDialog::TextInput);
	input_dialog.setOptions(QInputDialog::UseListViewForComboBoxItems);
	input_dialog.setComboBoxEditable(false);
	input_dialog.setComboBoxItems(std::move(input_options));
	if (input_dialog.exec() == 0)
		return ret;

	ret = input_dialog.textValue();
	return ret;
}

void MainWindow::startGameListEntry(const GameList::Entry* entry, std::optional<s32> save_slot, std::optional<bool> fast_boot)
{
	std::shared_ptr<VMBootParameters> params = std::make_shared<VMBootParameters>();
	params->fast_boot = fast_boot;

	GameList::FillBootParametersForEntry(params.get(), entry);

	if (save_slot.has_value() && !entry->serial.empty())
	{
		std::string state_filename = VMManager::GetSaveStateFileName(entry->serial.c_str(), entry->crc, save_slot.value());
		if (!FileSystem::FileExists(state_filename.c_str()))
		{
			QMessageBox::critical(this, tr("Error"), tr("This save state does not exist."));
			return;
		}

		params->save_state = std::move(state_filename);
	}

	g_emu_thread->startVM(std::move(params));
}

void MainWindow::setGameListEntryCoverImage(const GameList::Entry* entry)
{
	const QString filename(
		QFileDialog::getOpenFileName(this, tr("Select Cover Image"), QString(), tr("All Cover Image Types (*.jpg *.jpeg *.png)")));
	if (filename.isEmpty())
		return;

	if (!GameList::GetCoverImagePathForEntry(entry).empty())
	{
		if (QMessageBox::question(this, tr("Cover Already Exists"),
				tr("A cover image for this game already exists, do you wish to replace it?"), QMessageBox::Yes,
				QMessageBox::No) != QMessageBox::Yes)
		{
			return;
		}
	}

	const QString new_filename(QString::fromStdString(GameList::GetNewCoverImagePathForEntry(entry, filename.toUtf8().constData())));
	if (new_filename.isEmpty())
		return;

	if (QFile::exists(new_filename) && !QFile::remove(new_filename))
	{
		QMessageBox::critical(this, tr("Copy Error"), tr("Failed to remove existing cover '%1'").arg(new_filename));
		return;
	}

	if (!QFile::copy(filename, new_filename))
	{
		QMessageBox::critical(this, tr("Copy Error"), tr("Failed to copy '%1' to '%2'").arg(filename).arg(new_filename));
		return;
	}

	m_game_list_widget->refreshGridCovers();
}

void MainWindow::clearGameListEntryPlayTime(const GameList::Entry* entry)
{
	if (QMessageBox::question(this, tr("Confirm Reset"),
			tr("Are you sure you want to reset the play time for '%1'?\n\nThis action cannot be undone.")
				.arg(QString::fromStdString(entry->title))) != QMessageBox::Yes)
	{
		return;
	}

	GameList::ClearPlayedTimeForSerial(entry->serial);
	m_game_list_widget->refresh(false);
}

std::optional<bool> MainWindow::promptForResumeState(const QString& save_state_path)
{
	if (save_state_path.isEmpty())
		return false;

	QFileInfo fi(save_state_path);
	if (!fi.exists())
		return false;

	QMessageBox msgbox(this);
	msgbox.setIcon(QMessageBox::Question);
	msgbox.setWindowTitle(tr("Load Resume State"));
	msgbox.setText(
		tr("A resume save state was found for this game, saved at:\n\n%1.\n\nDo you want to load this state, or start from a fresh boot?")
			.arg(fi.lastModified().toLocalTime().toString()));

	QPushButton* load = msgbox.addButton(tr("Load State"), QMessageBox::AcceptRole);
	QPushButton* boot = msgbox.addButton(tr("Fresh Boot"), QMessageBox::RejectRole);
	QPushButton* delboot = msgbox.addButton(tr("Delete And Boot"), QMessageBox::RejectRole);
	msgbox.addButton(QMessageBox::Cancel);
	msgbox.setDefaultButton(load);
	msgbox.exec();

	QAbstractButton* clicked = msgbox.clickedButton();
	if (load == clicked)
	{
		return true;
	}
	else if (boot == clicked)
	{
		return false;
	}
	else if (delboot == clicked)
	{
		if (!QFile::remove(save_state_path))
			QMessageBox::critical(this, tr("Error"), tr("Failed to delete save state file '%1'.").arg(save_state_path));

		return false;
	}

	return std::nullopt;
}

void MainWindow::loadSaveStateSlot(s32 slot)
{
	if (s_vm_valid)
	{
		// easy when we're running
		g_emu_thread->loadStateFromSlot(slot);
		return;
	}
	else
	{
		// we're not currently running, therefore we must've right clicked in the game list
		const GameList::Entry* entry = m_game_list_widget->getSelectedEntry();
		if (!entry)
			return;

		startGameListEntry(entry, slot, std::nullopt);
	}
}

void MainWindow::loadSaveStateFile(const QString& filename, const QString& state_filename)
{
	if (s_vm_valid)
	{
		if (!filename.isEmpty() && m_current_disc_path != filename)
			g_emu_thread->changeDisc(CDVD_SourceType::Iso, m_current_disc_path);
		g_emu_thread->loadState(state_filename);
	}
	else
	{
		std::shared_ptr<VMBootParameters> params = std::make_shared<VMBootParameters>();
		params->filename = filename.toStdString();
		params->save_state = state_filename.toStdString();
		g_emu_thread->startVM(std::move(params));
	}
}

static QString formatTimestampForSaveStateMenu(time_t timestamp)
{
	const QDateTime qtime(QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestamp)));
	return qtime.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
}

void MainWindow::populateLoadStateMenu(QMenu* menu, const QString& filename, const QString& serial, quint32 crc)
{
	if (serial.isEmpty())
		return;

	const bool is_right_click_menu = (menu != m_ui.menuLoadState);
	bool has_any_states = false;

	QAction* action = menu->addAction(is_right_click_menu ? tr("Load State File...") : tr("Load From File..."));
	connect(action, &QAction::triggered, [this, filename]() {
		const QString path(QFileDialog::getOpenFileName(this, tr("Select Save State File"), QString(), tr("Save States (*.p2s)")));
		if (path.isEmpty())
			return;

		loadSaveStateFile(filename, path);
	});

	QAction* delete_save_states_action = menu->addAction(tr("Delete Save States..."));

	// don't include undo in the right click menu
	if (!is_right_click_menu)
	{
		QAction* load_undo_state = menu->addAction(tr("Undo Load State"));
		load_undo_state->setEnabled(false); // CanUndoLoadState()
		// connect(load_undo_state, &QAction::triggered, this, &QtHostInterface::undoLoadState);
		menu->addSeparator();
	}

	const QByteArray game_serial_utf8(serial.toUtf8());
	std::string state_filename;
	FILESYSTEM_STAT_DATA sd;
	if (is_right_click_menu)
	{
		state_filename = VMManager::GetSaveStateFileName(game_serial_utf8.constData(), crc, -1);
		if (FileSystem::StatFile(state_filename.c_str(), &sd))
		{
			action = menu->addAction(tr("Resume (%2)").arg(formatTimestampForSaveStateMenu(sd.ModificationTime)));
			connect(action, &QAction::triggered, [this]() { loadSaveStateSlot(-1); });

			// Make bold to indicate it's the default choice when double-clicking
			QtUtils::MarkActionAsDefault(action);
			has_any_states = true;
		}
	}

	for (s32 i = 1; i <= VMManager::NUM_SAVE_STATE_SLOTS; i++)
	{
		FILESYSTEM_STAT_DATA sd;
		state_filename = VMManager::GetSaveStateFileName(game_serial_utf8.constData(), crc, i);
		if (!FileSystem::StatFile(state_filename.c_str(), &sd))
			continue;

		action = menu->addAction(tr("Load Slot %1 (%2)").arg(i).arg(formatTimestampForSaveStateMenu(sd.ModificationTime)));
		connect(action, &QAction::triggered, [this, i]() { loadSaveStateSlot(i); });
		has_any_states = true;
	}

	delete_save_states_action->setEnabled(has_any_states);
	if (has_any_states)
	{
		connect(delete_save_states_action, &QAction::triggered, this, [this, serial, crc] {
			if (QMessageBox::warning(this, tr("Delete Save States"),
					tr("Are you sure you want to delete all save states for %1?\n\nThe saves will not be recoverable.").arg(serial),
					QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
			{
				return;
			}

			const u32 deleted = VMManager::DeleteSaveStates(serial.toUtf8().constData(), crc, true);
			QMessageBox::information(this, tr("Delete Save States"), tr("%1 save states deleted.").arg(deleted));
		});
	}
}

void MainWindow::populateSaveStateMenu(QMenu* menu, const QString& serial, quint32 crc)
{
	if (serial.isEmpty())
		return;

	connect(menu->addAction(tr("Save To File...")), &QAction::triggered, [this]() {
		const QString path(QFileDialog::getSaveFileName(this, tr("Select Save State File"), QString(), tr("Save States (*.p2s)")));
		if (path.isEmpty())
			return;

		g_emu_thread->saveState(path);
	});

	menu->addSeparator();

	const QByteArray game_serial_utf8(serial.toUtf8());
	for (s32 i = 1; i <= VMManager::NUM_SAVE_STATE_SLOTS; i++)
	{
		std::string filename(VMManager::GetSaveStateFileName(game_serial_utf8.constData(), crc, i));
		FILESYSTEM_STAT_DATA sd;
		QString timestamp;
		if (FileSystem::StatFile(filename.c_str(), &sd))
			timestamp = formatTimestampForSaveStateMenu(sd.ModificationTime);
		else
			timestamp = tr("Empty");

		QString title(tr("Save Slot %1 (%2)").arg(i).arg(timestamp));
		connect(menu->addAction(title), &QAction::triggered, [i]() { g_emu_thread->saveStateToSlot(i); });
	}
}

void MainWindow::updateSaveStateMenus(const QString& filename, const QString& serial, quint32 crc)
{
	const bool load_enabled = !serial.isEmpty();
	const bool save_enabled = !serial.isEmpty() && s_vm_valid;
	m_ui.menuLoadState->clear();
	m_ui.menuLoadState->setEnabled(load_enabled);
	m_ui.actionLoadState->setEnabled(load_enabled);
	m_ui.menuSaveState->clear();
	m_ui.menuSaveState->setEnabled(save_enabled);
	m_ui.actionSaveState->setEnabled(save_enabled);
	m_save_states_invalidated = false;
	if (load_enabled)
		populateLoadStateMenu(m_ui.menuLoadState, filename, serial, crc);
	if (save_enabled)
		populateSaveStateMenu(m_ui.menuSaveState, serial, crc);
}

void MainWindow::doStartFile(std::optional<CDVD_SourceType> source, const QString& path)
{
	if (s_vm_valid)
		return;

	std::shared_ptr<VMBootParameters> params = std::make_shared<VMBootParameters>();
	params->source_type = source;
	params->filename = path.toStdString();

	// we might still be saving a resume state...
	VMManager::WaitForSaveStateFlush();

	const std::optional<bool> resume(
		promptForResumeState(QString::fromStdString(VMManager::GetSaveStateFileName(params->filename.c_str(), -1))));
	if (!resume.has_value())
		return;
	else if (resume.value())
		params->state_index = -1;

	g_emu_thread->startVM(std::move(params));
}

void MainWindow::doDiscChange(CDVD_SourceType source, const QString& path)
{
	bool reset_system = false;
	if (!m_was_disc_change_request)
	{
		QMessageBox message(QMessageBox::Question, tr("Confirm Disc Change"),
			tr("Do you want to swap discs or boot the new image (via system reset)?"));
		message.addButton(tr("Swap Disc"), QMessageBox::ActionRole);
		QPushButton* reset_button = message.addButton(tr("Reset"), QMessageBox::ActionRole);
		QPushButton* cancel_button = message.addButton(QMessageBox::Cancel);
		message.setDefaultButton(cancel_button);
		message.exec();

		if (message.clickedButton() == cancel_button)
			return;
		reset_system = (message.clickedButton() == reset_button);
	}

	switchToEmulationView();

	g_emu_thread->changeDisc(source, path);
	if (reset_system)
		g_emu_thread->resetVM();
}

MainWindow::VMLock MainWindow::pauseAndLockVM()
{
	const bool was_fullscreen = isRenderingFullscreen();
	const bool was_paused = s_vm_paused;

	// We use surfaceless rather than switching out of fullscreen, because
	// we're paused, so we're not going to be rendering anyway.
	if (was_fullscreen)
		g_emu_thread->setSurfaceless(true);
	if (!was_paused)
		g_emu_thread->setVMPaused(true);

	// We want to parent dialogs to the display widget, except if we were fullscreen,
	// since it's going to get destroyed by the surfaceless call above.
	QWidget* dialog_parent = was_fullscreen ? static_cast<QWidget*>(this) : getDisplayContainer();

	return VMLock(dialog_parent, was_paused, was_fullscreen);
}

void MainWindow::rescanFile(const std::string& path)
{
	m_game_list_widget->rescanFile(path);
}

MainWindow::VMLock::VMLock(QWidget* dialog_parent, bool was_paused, bool was_fullscreen)
	: m_dialog_parent(dialog_parent)
	, m_was_paused(was_paused)
	, m_was_fullscreen(was_fullscreen)
{
}

MainWindow::VMLock::VMLock(VMLock&& lock)
	: m_dialog_parent(lock.m_dialog_parent)
	, m_was_paused(lock.m_was_paused)
	, m_was_fullscreen(lock.m_was_fullscreen)
{
	lock.m_dialog_parent = nullptr;
	lock.m_was_paused = true;
	lock.m_was_fullscreen = false;
}

MainWindow::VMLock::~VMLock()
{
	if (m_was_fullscreen)
		g_emu_thread->setSurfaceless(false);
	if (!m_was_paused)
		g_emu_thread->setVMPaused(false);
}

void MainWindow::VMLock::cancelResume()
{
	m_was_paused = true;
	m_was_fullscreen = false;
}

bool QtHost::IsVMValid()
{
	return s_vm_valid;
}

bool QtHost::IsVMPaused()
{
	return s_vm_paused;
}

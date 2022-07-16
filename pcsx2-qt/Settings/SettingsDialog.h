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
#include "ui_SettingsDialog.h"
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtWidgets/QDialog>
#include <array>
#include <memory>

class INISettingsInterface;
class SettingsInterface;

namespace GameList
{
struct Entry;
}

class InterfaceSettingsWidget;
class GameListSettingsWidget;
class EmulationSettingsWidget;
class BIOSSettingsWidget;
class SystemSettingsWidget;
class AdvancedSystemSettingsWidget;
class GameFixSettingsWidget;
class GraphicsSettingsWidget;
class AudioSettingsWidget;
class MemoryCardSettingsWidget;
class FolderSettingsWidget;
class DEV9SettingsWidget;

class SettingsDialog final : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget* parent);
	SettingsDialog(QWidget* parent, std::unique_ptr<SettingsInterface> sif, const GameList::Entry* game, u32 game_crc);
	~SettingsDialog();

	static void openGamePropertiesDialog(const GameList::Entry* game, const std::string_view& serial, u32 crc);

	__fi bool isPerGameSettings() const { return static_cast<bool>(m_sif); }
	__fi SettingsInterface* getSettingsInterface() const { return m_sif.get(); }

	__fi InterfaceSettingsWidget* getInterfaceSettingsWidget() const { return m_interface_settings; }
	__fi GameListSettingsWidget* getGameListSettingsWidget() const { return m_game_list_settings; }
	__fi BIOSSettingsWidget* getBIOSSettingsWidget() const { return m_bios_settings; }
	__fi EmulationSettingsWidget* getEmulationSettingsWidget() const { return m_emulation_settings; }
	__fi SystemSettingsWidget* getSystemSettingsWidget() const { return m_system_settings; }
	__fi AdvancedSystemSettingsWidget* getAdvancedSystemSettingsWidget() const { return m_advanced_system_settings; }
	__fi GameFixSettingsWidget* getGameFixSettingsWidget() const { return m_game_fix_settings_widget; }
	__fi GraphicsSettingsWidget* getGraphicsSettingsWidget() const { return m_graphics_settings; }
	__fi AudioSettingsWidget* getAudioSettingsWidget() const { return m_audio_settings; }
	__fi MemoryCardSettingsWidget* getMemoryCardSettingsWidget() const { return m_memory_card_settings; }
	__fi FolderSettingsWidget* getFolderSettingsWidget() const { return m_folder_settings; }
	__fi DEV9SettingsWidget* getDEV9SettingsWidget() const { return m_dev9_settings; }

	void registerWidgetHelp(QObject* object, QString title, QString recommended_value, QString text);
	bool eventFilter(QObject* object, QEvent* event) override;

	void setCategory(const char* category);

	// Helper functions for reading effective setting values (from game -> global settings).
	bool getEffectiveBoolValue(const char* section, const char* key, bool default_value) const;
	int getEffectiveIntValue(const char* section, const char* key, int default_value) const;
	float getEffectiveFloatValue(const char* section, const char* key, float default_value) const;
	std::string getEffectiveStringValue(const char* section, const char* key, const char* default_value) const;

	// Helper functions for reading setting values for this layer (game settings or global).
	std::optional<bool> getBoolValue(const char* section, const char* key, std::optional<bool> default_value) const;
	std::optional<int> getIntValue(const char* section, const char* key, std::optional<int> default_value) const;
	std::optional<float> getFloatValue(const char* section, const char* key, std::optional<float> default_value) const;
	std::optional<std::string> getStringValue(const char* section, const char* key, std::optional<const char*> default_value) const;
	void setBoolSettingValue(const char* section, const char* key, std::optional<bool> value);
	void setIntSettingValue(const char* section, const char* key, std::optional<int> value);
	void setFloatSettingValue(const char* section, const char* key, std::optional<float> value);
	void setStringSettingValue(const char* section, const char* key, std::optional<const char*> value);

Q_SIGNALS:
	void settingsResetToDefaults();

private Q_SLOTS:
	void onCategoryCurrentRowChanged(int row);
	void onRestoreDefaultsClicked();

protected:
	void closeEvent(QCloseEvent*) override;

private:
	enum : u32
	{
		MAX_SETTINGS_WIDGETS = 11
	};

	void setupUi(const GameList::Entry* game);

	void addWidget(QWidget* widget, QString title, QString icon, QString help_text);

	std::unique_ptr<SettingsInterface> m_sif;

	Ui::SettingsDialog m_ui;

	InterfaceSettingsWidget* m_interface_settings = nullptr;
	GameListSettingsWidget* m_game_list_settings = nullptr;
	BIOSSettingsWidget* m_bios_settings = nullptr;
	EmulationSettingsWidget* m_emulation_settings = nullptr;
	SystemSettingsWidget* m_system_settings = nullptr;
	AdvancedSystemSettingsWidget* m_advanced_system_settings = nullptr;
	GameFixSettingsWidget* m_game_fix_settings_widget = nullptr;
	GraphicsSettingsWidget* m_graphics_settings = nullptr;
	AudioSettingsWidget* m_audio_settings = nullptr;
	MemoryCardSettingsWidget* m_memory_card_settings = nullptr;
	FolderSettingsWidget* m_folder_settings = nullptr;
	DEV9SettingsWidget* m_dev9_settings = nullptr;

	std::array<QString, MAX_SETTINGS_WIDGETS> m_category_help_text;

	QObject* m_current_help_widget = nullptr;
	QMap<QObject*, QString> m_widget_help_text_map;

	u32 m_game_crc;
};

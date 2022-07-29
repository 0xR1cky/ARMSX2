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

#include <QtGui/QIcon>
#include <QtWidgets/QFileDialog>
#include <algorithm>

#include "pcsx2/HostSettings.h"
#include "pcsx2/ps2/BiosTools.h"

#include "BIOSSettingsWidget.h"
#include "QtHost.h"
#include "QtUtils.h"
#include "SettingWidgetBinder.h"
#include "SettingsDialog.h"

BIOSSettingsWidget::BIOSSettingsWidget(SettingsDialog* dialog, QWidget* parent)
	: QWidget(parent)
{
	SettingsInterface* sif = dialog->getSettingsInterface();

	m_ui.setupUi(this);

	SettingWidgetBinder::BindWidgetToBoolSetting(sif, m_ui.fastBoot, "EmuCore", "EnableFastBoot", true);
	SettingWidgetBinder::BindWidgetToBoolSetting(sif, m_ui.patchRegion, "EmuCore", "PatchBios", false);
	SettingWidgetBinder::BindWidgetToEnumSetting(sif, m_ui.regionComboBox, "EmuCore", "PatchRegion", BiosZoneStrings, BiosZoneBytes, BiosZoneBytes[0]);
	SettingWidgetBinder::BindWidgetToFolderSetting(sif, m_ui.searchDirectory, m_ui.browseSearchDirectory, m_ui.openSearchDirectory,
		m_ui.resetSearchDirectory, "Folders", "Bios", "bios");

	dialog->registerWidgetHelp(m_ui.patchRegion, tr("Patch Region"), tr("Unchecked"),
		tr("Patches the BIOS region byte in ROM. Not recommended unless you really know what you're doing."));
	dialog->registerWidgetHelp(m_ui.fastBoot, tr("Fast Boot"), tr("Checked"),
		tr("Patches the BIOS to skip the console's boot animation."));

	refreshList();

	connect(m_ui.searchDirectory, &QLineEdit::textChanged, this, &BIOSSettingsWidget::refreshList);
	connect(m_ui.refresh, &QPushButton::clicked, this, &BIOSSettingsWidget::refreshList);
	connect(m_ui.fileList, &QTreeWidget::currentItemChanged, this, &BIOSSettingsWidget::listItemChanged);

	connect(m_ui.patchRegion, &QCheckBox::clicked, this, [&] { m_ui.regionComboBox->setEnabled(m_ui.patchRegion->isChecked()); });
	m_ui.regionComboBox->setEnabled(m_ui.patchRegion->isChecked());
}

BIOSSettingsWidget::~BIOSSettingsWidget()
{
	if (m_refresh_thread)
		m_refresh_thread->wait();
}

void BIOSSettingsWidget::refreshList()
{
	if (m_refresh_thread)
	{
		m_refresh_thread->wait();
		delete m_refresh_thread;
	}

	QSignalBlocker blocker(m_ui.fileList);
	m_ui.fileList->clear();
	m_ui.fileList->setEnabled(false);

	m_refresh_thread = new RefreshThread(this, m_ui.searchDirectory->text());
	m_refresh_thread->start();
}

void BIOSSettingsWidget::listRefreshed(const QVector<BIOSInfo>& items)
{
	const std::string selected_bios(Host::GetBaseStringSettingValue("Filenames", "BIOS"));
	const QString res_path(QtHost::GetResourcesBasePath());

	QSignalBlocker sb(m_ui.fileList);
	for (const BIOSInfo& bi : items)
	{
		QTreeWidgetItem* item = new QTreeWidgetItem();
		item->setText(0, QString::fromStdString(bi.filename));
		item->setText(1, QString::fromStdString(bi.description));

		switch (bi.region)
		{
			case 2: // Japan
				item->setIcon(0, QIcon(QStringLiteral("%1/icons/flags/NTSC-J.png").arg(res_path)));
				break;

			case 3: // USA
				item->setIcon(0, QIcon(QStringLiteral("%1/icons/flags/NTSC-U.png").arg(res_path)));
				break;

			case 4: // Europe
				item->setIcon(0, QIcon(QStringLiteral("%1/icons/flags/PAL-E.png").arg(res_path)));
				break;

			case 7: // China
				item->setIcon(0, QIcon(QStringLiteral("%1/icons//flags/NTSC-C.png").arg(res_path)));
				break;

			case 5: // HK
				item->setIcon(0, QIcon(QStringLiteral("%1/icons/flags/NTSC-HK.png").arg(res_path)));
				break;

			case 6: // Free
			case 0: // T10K
			case 1: // Test
			default:
				item->setIcon(0, QIcon(QStringLiteral("%1/icons/flags/NTSC-J.png").arg(res_path)));
				break;
		}

		m_ui.fileList->addTopLevelItem(item);

		if (bi.filename == selected_bios)
			item->setSelected(true);
	}
	m_ui.fileList->setEnabled(true);
}

void BIOSSettingsWidget::listItemChanged(const QTreeWidgetItem* current, const QTreeWidgetItem* previous)
{
	QtHost::SetBaseStringSettingValue("Filenames", "BIOS", current->text(0).toUtf8().constData());
}

BIOSSettingsWidget::RefreshThread::RefreshThread(BIOSSettingsWidget* parent, const QString& directory)
	: QThread(parent)
	, m_parent(parent)
	, m_directory(directory)
{
}

BIOSSettingsWidget::RefreshThread::~RefreshThread() = default;

void BIOSSettingsWidget::RefreshThread::run()
{
	QVector<BIOSInfo> items;

	QDir dir(m_directory);
	if (dir.exists())
	{
		for (const QFileInfo& info : dir.entryInfoList(QDir::Files))
		{
			BIOSInfo bi;
			QString full_path(info.absoluteFilePath());
			if (!IsBIOS(full_path.toUtf8().constData(), bi.version, bi.description, bi.region, bi.zone))
				continue;

			bi.filename = info.fileName().toStdString();
			items.push_back(std::move(bi));
		}
	}

	QMetaObject::invokeMethod(m_parent, "listRefreshed", Qt::QueuedConnection, Q_ARG(const QVector<BIOSInfo>&, items));
}

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

#include "AboutDialog.h"
#include "QtHost.h"
#include "QtUtils.h"
#include <QtCore/QString>
#include <QtWidgets/QDialog>

AboutDialog::AboutDialog(QWidget* parent)
	: QDialog(parent)
{
	m_ui.setupUi(this);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setFixedSize(geometry().width(), geometry().height());

	m_ui.scmversion->setTextInteractionFlags(Qt::TextSelectableByMouse);
	m_ui.scmversion->setText(QtHost::GetAppNameAndVersion());

	m_ui.links->setTextInteractionFlags(Qt::TextBrowserInteraction);
	m_ui.links->setOpenExternalLinks(true);
	m_ui.links->setText(QStringLiteral(
		R"(<a href="%1">%2</a> | <a href="%3">%4</a> | <a href="%5">%6</a> | <a href="%7">%8</a>)")
							.arg(getWebsiteUrl())
							.arg(tr("Website"))
							.arg(getSupportForumsUrl())
							.arg(tr("Support Forums"))
							.arg(getGitHubRepositoryUrl())
							.arg(tr("GitHub Repository"))
							.arg(getLicenseUrl())
							.arg(tr("License")));

	connect(m_ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
}

AboutDialog::~AboutDialog() = default;

QString AboutDialog::getWebsiteUrl()
{
	return QStringLiteral("https://pcsx2.net/");
}

QString AboutDialog::getSupportForumsUrl()
{
	return QStringLiteral("https://forums.pcsx2.net/");
}

QString AboutDialog::getGitHubRepositoryUrl()
{
	return QStringLiteral("https://github.com/PCSX2/pcsx2");
}

QString AboutDialog::getLicenseUrl()
{
	return QStringLiteral("https://github.com/PCSX2/pcsx2/blob/master/pcsx2/Docs/License.txt");
}

QString AboutDialog::getDiscordServerUrl()
{
	return QStringLiteral("https://discord.com/invite/TCz3t9k");
}

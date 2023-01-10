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

#include "ui_CpuWidget.h"

#include "DebugTools/DebugInterface.h"
#include "DebugTools/Breakpoints.h"
#include "DebugTools/BiosDebugData.h"
#include "DebugTools/MipsStackWalk.h"

#include "Models/BreakpointModel.h"
#include "Models/ThreadModel.h"
#include "Models/StackModel.h"

#include "QtHost.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>

#include <vector>

using namespace MipsStackWalk;

class CpuWidget final : public QWidget
{
	Q_OBJECT

public:
	CpuWidget(QWidget* parent, DebugInterface& cpu);
	~CpuWidget();

public slots:
	void paintEvent(QPaintEvent* event);

	void onStepInto();
	void onStepOver();
	void onStepOut();

	void onVMPaused();

	void updateBreakpoints();
	void onBPListDoubleClicked(const QModelIndex& index);
	void onBPListContextMenu(QPoint pos);

	void contextBPListCopy();
	void contextBPListDelete();
	void contextBPListNew();
	void contextBPListEdit();

	void updateThreads();
	void onThreadListDoubleClick(const QModelIndex& index);
	void onThreadListContextMenu(QPoint pos);

	void updateStackFrames();
	void onStackListContextMenu(QPoint pos);
	void onStackListDoubleClick(const QModelIndex& index);

	void updateFunctionList(bool whenEmpty = false);
	void onFuncListContextMenu(QPoint pos);
	void onFuncListDoubleClick(QListWidgetItem* item);

	void reloadCPUWidgets()
	{
		if (!QtHost::IsOnUIThread())
		{
			QtHost::RunOnUIThread(CBreakPoints::GetUpdateHandler());
			return;
		}

		updateBreakpoints();
		updateThreads();
		updateStackFrames();

		m_ui.registerWidget->update();
		m_ui.disassemblyWidget->update();
		m_ui.memoryviewWidget->update();
	};

	void onSearchButtonClicked();

private:
	std::vector<QTableWidget*> m_registerTableViews;

	QMenu* m_stacklistContextMenu = 0;
	QMenu* m_funclistContextMenu = 0;

	Ui::CpuWidget m_ui;

	DebugInterface& m_cpu;

	BreakpointModel m_bpModel;
	ThreadModel m_threadModel;
	StackModel m_stackModel;

	std::vector<EEThread> m_threadlistObjects;
	std::vector<StackFrame> m_stacklistObjects;

	bool m_demangleFunctions = true;
};

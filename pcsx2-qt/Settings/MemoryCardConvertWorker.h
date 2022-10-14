
#pragma once

#include <QtCore/QtCore>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressDialog>

#include "QtProgressCallback.h"

#include "pcsx2/MemoryCardFile.h"
#include "pcsx2/MemoryCardFolder.h"

class MemoryCardConvertWorker : public QtAsyncProgressThread
{
public:
	MemoryCardConvertWorker(QWidget* parent, MemoryCardType type, MemoryCardFileType fileType, const std::string& srcFileName, const std::string& destFileName);
	~MemoryCardConvertWorker();

protected:
	void runAsync() override;

private:
	MemoryCardType type;
	MemoryCardFileType fileType;
	std::string srcFileName;
	std::string destFileName;


	bool ConvertToFile(const std::string& srcFolderName, const std::string& destFileName, const MemoryCardFileType type);
	bool ConvertToFolder(const std::string& srcFolderName, const std::string& destFileName, const MemoryCardFileType type);
};

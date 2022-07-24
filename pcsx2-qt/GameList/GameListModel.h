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
#include "pcsx2/Frontend/GameList.h"
#include "common/LRUCache.h"
#include <QtCore/QAbstractTableModel>
#include <QtGui/QPixmap>
#include <algorithm>
#include <atomic>
#include <array>
#include <optional>
#include <unordered_map>

class GameListModel final : public QAbstractTableModel
{
	Q_OBJECT

public:
	enum Column : int
	{
		Column_Type,
		Column_Serial,
		Column_Title,
		Column_FileTitle,
		Column_CRC,
		Column_Size,
		Column_Region,
		Column_Compatibility,
		Column_Cover,

		Column_Count
	};

	static std::optional<Column> getColumnIdForName(std::string_view name);
	static const char* getColumnName(Column col);

	static QIcon getIconForType(GameList::EntryType type);
	static QIcon getIconForRegion(GameList::Region region);

	GameListModel(QObject* parent = nullptr);
	~GameListModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	__fi const QString& getColumnDisplayName(int column) { return m_column_display_names[column]; }

	void refresh();
	void refreshImages();

	bool titlesLessThan(int left_row, int right_row) const;

	bool lessThan(const QModelIndex& left_index, const QModelIndex& right_index, int column) const;

	bool getShowCoverTitles() const { return m_show_titles_for_covers; }
	void setShowCoverTitles(bool enabled) { m_show_titles_for_covers = enabled; }

	float getCoverScale() const { return m_cover_scale; }
	void setCoverScale(float scale);
	int getCoverArtWidth() const;
	int getCoverArtHeight() const;
	int getCoverArtSpacing() const;
	void refreshCovers();
	void updateCacheSize(int width, int height);

private:
	void loadCommonImages();
	void setColumnDisplayNames();
	void loadOrGenerateCover(const GameList::Entry* ge);
	void invalidateCoverForPath(const std::string& path);

	float m_cover_scale = 0.0f;
	std::atomic<u32> m_cover_scale_counter{0};
	bool m_show_titles_for_covers = false;

	std::array<QString, Column_Count> m_column_display_names;
	std::array<QPixmap, static_cast<u32>(GameList::EntryType::Count)> m_type_pixmaps;
	std::array<QPixmap, static_cast<u32>(GameList::Region::Count)> m_region_pixmaps;
	QPixmap m_placeholder_pixmap;
	QPixmap m_loading_pixmap;

	std::array<QPixmap, static_cast<int>(GameList::CompatibilityRatingCount)> m_compatibility_pixmaps;
	mutable LRUCache<std::string, QPixmap> m_cover_pixmap_cache;
};

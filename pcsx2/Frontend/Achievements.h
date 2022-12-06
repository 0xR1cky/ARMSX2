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
#include "common/Pcsx2Defs.h"
#include "pcsx2/Achievements.h"
#include "Config.h"
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Achievements
{
	enum class AchievementCategory : u8
	{
		Local = 0,
		Core = 3,
		Unofficial = 5
	};

	struct Achievement
	{
		u32 id;
		std::string title;
		std::string description;
		std::string memaddr;
		std::string badge_name;

		// badge paths are mutable because they're resolved when they're needed.
		mutable std::string locked_badge_path;
		mutable std::string unlocked_badge_path;

		u32 points;
		AchievementCategory category;
		bool locked;
		bool active;
		bool primed;
	};

	struct Leaderboard
	{
		u32 id;
		std::string title;
		std::string description;
		int format;
	};

	struct LeaderboardEntry
	{
		std::string user;
		std::string formatted_score;
		time_t submitted;
		u32 rank;
		bool is_self;
	};

	// RAIntegration only exists for Windows, so no point checking it on other platforms.
#ifdef ENABLE_RAINTEGRATION

	bool IsUsingRAIntegration();

#else

	static __fi bool IsUsingRAIntegration() { return false; }

#endif

	bool IsActive();
	bool IsLoggedIn();
	bool ChallengeModeActive();
	bool LeaderboardsActive();
	bool IsTestModeActive();
	bool IsUnofficialTestModeActive();
	bool IsRichPresenceEnabled();
	bool HasActiveGame();

	u32 GetGameID();

	/// Acquires the achievements lock. Must be held when accessing any achievement state from another thread.
	std::unique_lock<std::recursive_mutex> GetLock();

	void Initialize();
	void UpdateSettings(const Pcsx2Config::AchievementsOptions& old_config);

	/// Called when the system is being reset. If it returns false, the reset should be aborted.
	bool OnReset();

	/// Called when the system is being shut down. If Shutdown() returns false, the shutdown should be aborted.
	bool Shutdown();

	/// Called when the system is being paused and resumed.
	void OnPaused(bool paused);

	/// Called once a frame at vsync time on the CPU thread.
	void VSyncUpdate();

	/// Called to process pending HTTP requests when the VM is paused, because otherwise the vsync event won't fire.
	void ProcessPendingHTTPRequestsFromGSThread();

	void LoadState(const u8* state_data, u32 state_data_size);
	std::vector<u8> SaveState();

	/// Returns true if the current game has any achievements or leaderboards.
	/// Does not need to have the lock held.
	bool SafeHasAchievementsOrLeaderboards();

	const std::string& GetUsername();
	const std::string& GetRichPresenceString();

	bool LoginAsync(const char* username, const char* password);
	bool Login(const char* username, const char* password);
	void Logout();

	void GameChanged(u32 crc);

	const std::string& GetGameTitle();
	const std::string& GetGameIcon();

	bool EnumerateAchievements(std::function<bool(const Achievement&)> callback);
	u32 GetUnlockedAchiementCount();
	u32 GetAchievementCount();
	u32 GetMaximumPointsForGame();
	u32 GetCurrentPointsForGame();

	bool EnumerateLeaderboards(std::function<bool(const Leaderboard&)> callback);
	std::optional<bool> TryEnumerateLeaderboardEntries(u32 id, std::function<bool(const LeaderboardEntry&)> callback);
	const Leaderboard* GetLeaderboardByID(u32 id);
	u32 GetLeaderboardCount();
	bool IsLeaderboardTimeType(const Leaderboard& leaderboard);

	const Achievement* GetAchievementByID(u32 id);
	std::pair<u32, u32> GetAchievementProgress(const Achievement& achievement);
	std::string GetAchievementProgressText(const Achievement& achievement);
	const std::string& GetAchievementBadgePath(
		const Achievement& achievement, bool download_if_missing = true, bool force_unlocked_icon = false);
	std::string GetAchievementBadgeURL(const Achievement& achievement);
	u32 GetPrimedAchievementCount();

#ifdef ENABLE_RAINTEGRATION
	void SwitchToRAIntegration();

	namespace RAIntegration
	{
		void MainWindowChanged(void* new_handle);
		void GameChanged();
		std::vector<std::tuple<int, std::string, bool>> GetMenuItems();
		void ActivateMenuItem(int item);
	} // namespace RAIntegration
#endif
} // namespace Achievements

/// Functions implemented in the frontend.
namespace Host
{
	void OnAchievementsRefreshed();
	void OnAchievementsChallengeModeChanged();
} // namespace Host

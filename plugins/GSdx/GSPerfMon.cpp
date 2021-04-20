/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSPerfMon.h"

GSPerfMon::GSPerfMon()
	: m_frame(0)
	, m_lastframe(0)
	, m_count(0)
{
	memset(m_counters, 0, sizeof(m_counters));
	memset(m_stats, 0, sizeof(m_stats));
	memset(m_total, 0, sizeof(m_total));
	memset(m_begin, 0, sizeof(m_begin));
}

void GSPerfMon::Put(counter_t c, double val)
{
#ifndef DISABLE_PERF_MON
	if (c == Frame)
	{
#if defined(__unix__) || defined(__APPLE__)
		// clock on linux will return CLOCK_PROCESS_CPUTIME_ID.
		// CLOCK_THREAD_CPUTIME_ID is much more useful to measure the fps
		struct timespec ts;
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
		uint64 now = (uint64)ts.tv_sec * (uint64)1e6 + (uint64)ts.tv_nsec / (uint64)1e3;
#else
		clock_t now = clock();
#endif

		if (m_lastframe != 0)
		{
			m_counters[c] += (now - m_lastframe) * 1000 / CLOCKS_PER_SEC;
		}

		m_lastframe = now;
		m_frame++;
		m_count++;
	}
	else
	{
		m_counters[c] += val;
	}
#endif
}

void GSPerfMon::Update()
{
#ifndef DISABLE_PERF_MON
	if (m_count > 0)
	{
		for (size_t i = 0; i < countof(m_counters); i++)
		{
			m_stats[i] = m_counters[i] / m_count;
		}

		m_count = 0;
	}

	memset(m_counters, 0, sizeof(m_counters));
#endif
}

void GSPerfMon::Start(int timer)
{
#ifndef DISABLE_PERF_MON
	m_start[timer] = __rdtsc();

	if (m_begin[timer] == 0)
	{
		m_begin[timer] = m_start[timer];
	}
#endif
}

void GSPerfMon::Stop(int timer)
{
#ifndef DISABLE_PERF_MON
	if (m_start[timer] > 0)
	{
		m_total[timer] += __rdtsc() - m_start[timer];
		m_start[timer] = 0;
	}
#endif
}

int GSPerfMon::CPU(int timer, bool reset)
{
	int percent = (int)(100 * m_total[timer] / (__rdtsc() - m_begin[timer]));

	if (reset)
	{
		m_begin[timer] = 0;
		m_start[timer] = 0;
		m_total[timer] = 0;
	}

	return percent;
}

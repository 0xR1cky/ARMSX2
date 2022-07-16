/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2022 PCSX2 Dev Team
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

#if ! __has_feature(objc_arc)
	#error "Compile this with -fobjc-arc"
#endif

#include "CocoaTools.h"
#include "Console.h"
#include "WindowInfo.h"
#include <vector>
#include <Cocoa/Cocoa.h>
#include <QuartzCore/QuartzCore.h>

// MARK: - Metal Layers

bool CocoaTools::CreateMetalLayer(WindowInfo* wi)
{
	if (![NSThread isMainThread])
	{
		bool ret;
		dispatch_sync(dispatch_get_main_queue(), [&ret, wi]{ ret = CreateMetalLayer(wi); });
		return ret;
	}

	CAMetalLayer* layer = [CAMetalLayer layer];
	if (!layer)
	{
		Console.Error("Failed to create Metal layer.");
		return false;
	}

	NSView* view = (__bridge NSView*)wi->window_handle;
	[view setWantsLayer:YES];
	[view setLayer:layer];
	[layer setContentsScale:[[[view window] screen] backingScaleFactor]];
	// Store the layer pointer, that way MoltenVK doesn't call [NSView layer] outside the main thread.
	wi->surface_handle = (__bridge_retained void*)layer;
	return true;
}

void CocoaTools::DestroyMetalLayer(WindowInfo* wi)
{
	if (![NSThread isMainThread])
	{
		dispatch_sync_f(dispatch_get_main_queue(), wi, [](void* ctx){ DestroyMetalLayer(static_cast<WindowInfo*>(ctx)); });
		return;
	}

	NSView* view = (__bridge NSView*)wi->window_handle;
	CAMetalLayer* layer = (__bridge_transfer CAMetalLayer*)wi->surface_handle;
	if (!layer)
		return;
	wi->surface_handle = nullptr;
	[view setLayer:nil];
	[view setWantsLayer:NO];
}

// MARK: - Theme Change Handlers

@interface PCSX2KVOHelper : NSObject

- (void)addCallback:(void*)ctx run:(void(*)(void*))callback;
- (void)removeCallback:(void*)ctx;

@end

@implementation PCSX2KVOHelper
{
	std::vector<std::pair<void*, void(*)(void*)>> _callbacks;
}

- (void)addCallback:(void*)ctx run:(void(*)(void*))callback
{
	_callbacks.push_back(std::make_pair(ctx, callback));
}

- (void)removeCallback:(void*)ctx
{
	auto new_end = std::remove_if(_callbacks.begin(), _callbacks.end(), [ctx](const auto& entry){
		return ctx == entry.first;
	});
	_callbacks.erase(new_end, _callbacks.end());
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context
{
	for (const auto& callback : _callbacks)
		callback.second(callback.first);
}

@end

static PCSX2KVOHelper* s_themeChangeHandler;

void CocoaTools::AddThemeChangeHandler(void* ctx, void(handler)(void* ctx))
{
	assert([NSThread isMainThread]);
	if (!s_themeChangeHandler)
	{
		s_themeChangeHandler = [[PCSX2KVOHelper alloc] init];
		NSApplication* app = [NSApplication sharedApplication];
		[app addObserver:s_themeChangeHandler
		      forKeyPath:@"effectiveAppearance"
		         options:0
		         context:nil];
	}
	[s_themeChangeHandler addCallback:ctx run:handler];
}

void CocoaTools::RemoveThemeChangeHandler(void* ctx)
{
	assert([NSThread isMainThread]);
	[s_themeChangeHandler removeCallback:ctx];
}

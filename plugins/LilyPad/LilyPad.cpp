/*  LilyPad - Pad plugin for PS2 Emulator
 *  Copyright (C) 2002-2017  PCSX2 Dev Team/ChickenLiver
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU Lesser General Public License as published by the Free
 *  Software Found- ation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with PCSX2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Global.h"

// For escape timer, so as not to break GSDX+DX9.
#include <time.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "resource.h"
#include "InputManager.h"
#include "Config.h"

#define PADdefs

#include "DeviceEnumerator.h"
#ifdef _MSC_VER
#include "WndProcEater.h"
#include "HidDevice.h"
#endif
#include "KeyboardQueue.h"
#include "svnrev.h"
#include "DualShock3.h"

#define WMA_FORCE_UPDATE (WM_APP + 0x537)
#define FORCE_UPDATE_WPARAM ((WPARAM)0x74328943)
#define FORCE_UPDATE_LPARAM ((LPARAM)0x89437437)

// LilyPad version.
#define VERSION ((0 << 8) | 12 | (1 << 24))

#ifdef __linux__
Display *GSdsp;
Window GSwin;
#else
HINSTANCE hInst;
HWND hWnd;
HWND hWndTop;

WndProcEater hWndGSProc;
WndProcEater hWndTopProc;

// ButtonProc is used mostly by the Config panel for eating the procedures of the
// button with keyboard focus.
WndProcEater hWndButtonProc;
#endif

// Keeps the various sources for Update polling (PADpoll, PADupdate, etc) from wreaking
// havoc on each other...
#ifdef __linux__
static std::mutex updateLock;
#else
CRITICAL_SECTION updateLock;
#endif

// Used to toggle mouse listening.
u8 miceEnabled;

// 2 when both pads are initialized, 1 for one pad, etc.
int openCount = 0;

int activeWindow = 0;
#ifdef _MSC_VER
int windowThreadId = 0;
int updateQueued = 0;
#endif

u32 bufSize = 0;
unsigned char outBuf[50];
unsigned char inBuf[50];

//		windowThreadId = GetWindowThreadProcessId(hWnd, 0);

#define MODE_PS1_MOUSE 0x12
#define MODE_NEGCON 0x23
#define MODE_DIGITAL 0x41
#define MODE_ANALOG 0x73
#define MODE_DS2_NATIVE 0x79

#ifdef _MSC_VER
int IsWindowMaximized(HWND hWnd)
{
    RECT rect;
    if (GetWindowRect(hWnd, &rect)) {
        POINT p;
        p.x = rect.left;
        p.y = rect.top;
        MONITORINFO info;
        memset(&info, 0, sizeof(info));
        info.cbSize = sizeof(info);
        HMONITOR hMonitor;
        if ((hMonitor = MonitorFromPoint(p, MONITOR_DEFAULTTOPRIMARY)) &&
            GetMonitorInfo(hMonitor, &info) &&
            memcmp(&info.rcMonitor, &rect, sizeof(rect)) == 0) {
            return 1;
        }
    }
    return 0;
}
#endif

void DEBUG_TEXT_OUT(const char *text)
{
    if (!config.debug)
        return;

    std::ofstream file("logs/padLog.txt", std::ios::app);
    if (!file.good())
        return;
    file << text;
}

void DEBUG_NEW_SET()
{
    if (config.debug && bufSize > 1) {
        std::ofstream file("logs/padLog.txt", std::ios::app);
        if (file.good()) {
            std::stringstream stream;
            stream.setf(std::ios::hex, std::ios::basefield);
            stream.setf(std::ios::uppercase);
            stream.fill('0');

            unsigned char *buffer[2] = {inBuf, outBuf};
            for (const auto &buf : buffer) {
                // Port/FF
                stream << std::setw(2) << int(buf[0]);
                // Active slots/Enabled (only relevant for multitap)
                stream << " (" << std::setw(2) << int(buf[1]) << ')';

                // Command/Response
                for (u32 n = 2; n < bufSize; ++n)
                    stream << ' ' << std::setw(2) << int(buf[n]);
                stream << '\n';
            }
            stream << '\n';
            file << stream.rdbuf();
        }
    }
    bufSize = 0;
}

inline void DEBUG_IN(unsigned char c)
{
    if (bufSize < sizeof(inBuf))
        inBuf[bufSize] = c;
}
inline void DEBUG_OUT(unsigned char c)
{
    if (bufSize < sizeof(outBuf))
        outBuf[bufSize++] = c;
}

struct Stick
{
    int horiz;
    int vert;
};

// Sum of states of all controls for a pad (Not including toggles).
struct ButtonSum
{
    int buttons[16];
    Stick sticks[2];
};

#define PAD_SAVE_STATE_VERSION 5

// Freeze data, for a single pad.  Basically has all pad state that
// a PS2 can set.
struct PadFreezeData
{
    // Digital / Analog / DS2 Native
    u8 mode;

    u8 previousType;

    u8 modeLock;

    // In config mode
    u8 config;

    u8 vibrate[8];
    u8 umask[2];

    // Vibration indices.
    u8 vibrateI[2];

    // Last vibration value sent to controller.
    // Only used so as not to call vibration
    // functions when old and new values are both 0.
    u8 currentVibrate[2];

    // Next vibrate val to send to controller.  If next and current are
    // both 0, nothing is sent to the controller.  Otherwise, it's sent
    // on every update.
    u8 nextVibrate[2];
};

class Pad : public PadFreezeData
{
public:
    // Current pad state.
    ButtonSum sum;
    // State of locked buttons.  Already included by sum, used
    // as initial value of sum.
    ButtonSum lockedSum;

    // Flags for which controls (buttons or axes) are locked, if any.
    DWORD lockedState;

    // Used to keep track of which pads I'm running.
    // Note that initialized pads *can* be disabled.
    // I keep track of state of non-disabled non-initialized
    // pads, but should never be asked for their state.
    u8 initialized;

    // Set to 1 if the state of this pad has been updated since its state
    // was last queried.
    char stateUpdated;

    // initialized and not disabled (and mtap state for slots > 0).
    u8 enabled;
} pads[2][4];

// Active slots for each port.
int slots[2];
// Which ports we're running on.
int portInitialized[2];

// Force value to be from 0 to 255.
u8 Cap(int i)
{
    if (i < 0)
        return 0;
    if (i > 255)
        return 255;
    return (u8)i;
}

inline void ReleaseModifierKeys()
{
    QueueKeyEvent(VK_SHIFT, KEYRELEASE);
    QueueKeyEvent(VK_MENU, KEYRELEASE);
    QueueKeyEvent(VK_CONTROL, KEYRELEASE);
}

// RefreshEnabledDevices() enables everything that can potentially
// be bound to, as well as the "Ignore keyboard" device.
//
// This enables everything that input should be read from while the
// emulator is running.  Takes into account  mouse and focus state
// and which devices have bindings for enabled pads.  Releases
// keyboards if window is not focused.  Releases game devices if
// background monitoring is not checked.
// And releases games if not focused and config.background is not set.
void UpdateEnabledDevices(int updateList = 0)
{
    // Enable all devices I might want.  Can ignore the rest.
    RefreshEnabledDevices(updateList);
    // Figure out which pads I'm getting input for.
    for (int port = 0; port < 2; port++) {
        for (int slot = 0; slot < 4; slot++) {
            if (slot > 0 && !config.multitap[port]) {
                pads[port][slot].enabled = 0;
            } else {
                pads[port][slot].enabled = pads[port][slot].initialized && config.padConfigs[port][slot].type != DisabledPad;
            }
        }
    }
    for (int i = 0; i < dm->numDevices; i++) {
        Device *dev = dm->devices[i];

        if (!dev->enabled)
            continue;
        if (!dev->attached) {
            dm->DisableDevice(i);
            continue;
        }

        // Disable ignore keyboard if don't have focus or there are no keys to ignore.
        if (dev->api == IGNORE_KEYBOARD) {
            if ((config.keyboardApi == NO_API || !dev->pads[0][0][0].numBindings) || !activeWindow) {
                dm->DisableDevice(i);
            }
            continue;
        }
        // Keep for PCSX2 keyboard shotcuts, unless unfocused.
        if (dev->type == KEYBOARD) {
            if (!activeWindow)
                dm->DisableDevice(i);
        }
        // Keep for cursor hiding consistency, unless unfocused.
        // miceEnabled tracks state of mouse enable/disable button, not if mouse API is set to disabled.
        else if (dev->type == MOUSE) {
            if (!miceEnabled || !activeWindow)
                dm->DisableDevice(i);
        } else if (!activeWindow && !config.background)
            dm->DisableDevice(i);
        else {
            int numActiveBindings = 0;
            for (int port = 0; port < 2; port++) {
                for (int slot = 0; slot < 4; slot++) {
                    int padtype = config.padConfigs[port][slot].type;
                    if (pads[port][slot].enabled) {
                        numActiveBindings += dev->pads[port][slot][padtype].numBindings + dev->pads[port][slot][padtype].numFFBindings;
                    }
                }
            }
            if (!numActiveBindings)
                dm->DisableDevice(i);
        }
    }
}

#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, void *lpvReserved)
{
    hInst = hInstance;
    if (fdwReason == DLL_PROCESS_ATTACH) {
        InitializeCriticalSection(&updateLock);

        DisableThreadLibraryCalls(hInstance);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        while (openCount)
            PADclose();
        PADshutdown();
        UninitLibUsb();
        DeleteCriticalSection(&updateLock);
    }
    return 1;
}
#endif

void AddForce(ButtonSum *sum, u8 cmd, int delta = 255)
{
    if (!delta)
        return;
    if (cmd < 0x14) {
        sum->buttons[cmd - 0x10] += delta;
    }
    // D-pad.  Command numbering is based on ordering of digital values.
    else if (cmd < 0x18) {
        sum->buttons[cmd - 0x08] += delta;
    } else if (cmd < 0x20) {
        sum->buttons[cmd - 0x10 - 4] += delta;
    }
    // Left stick.
    else if (cmd < 0x24) {
        if (cmd == 0x20) { // Up
            sum->sticks[1].vert -= delta;
        } else if (cmd == 0x21) { // Right
            sum->sticks[1].horiz += delta;
        } else if (cmd == 0x22) { // Down
            sum->sticks[1].vert += delta;
        } else if (cmd == 0x23) { // Left
            sum->sticks[1].horiz -= delta;
        }
    }
    // Right stick.
    else if (cmd < 0x28) {
        if (cmd == 0x24) { // Up
            sum->sticks[0].vert -= delta;
        } else if (cmd == 0x25) { // Right
            sum->sticks[0].horiz += delta;
        } else if (cmd == 0x26) { // Down
            sum->sticks[0].vert += delta;
        } else if (cmd == 0x27) { // Left
            sum->sticks[0].horiz -= delta;
        }
    }
}

void ProcessButtonBinding(Binding *b, ButtonSum *sum, int value)
{
    if (value < b->deadZone || value == 0) {
        return;
    }
    if (b->skipDeadZone > b->deadZone) {
        value = std::min((int)(((__int64)value * (FULLY_DOWN - (__int64)b->skipDeadZone)) / FULLY_DOWN) + b->skipDeadZone, FULLY_DOWN);
    }

    if (b->command == 0x2D) { // Turbo key
        static unsigned int LastCheck = 0;
        unsigned int t = timeGetTime();
        if (t - LastCheck < 300)
            return;
        QueueKeyEvent(VK_TAB, KEYPRESS);
        LastCheck = t;
    }

    int sensitivity = b->sensitivity;
    if (sensitivity < 0) {
        sensitivity = -sensitivity;
        value = (1 << 16) - value;
    }
    if (value < 0)
        return;

    /* Note:  Value ranges of FULLY_DOWN, and sensitivity of
	 *  BASE_SENSITIVITY corresponds to an axis/button being exactly fully down.
	 *  Math in next line takes care of those two conditions, rounding as necessary.
	 *  Done using __int64s because overflows will occur when
	 *  sensitivity > BASE_SENSITIVITY and/or value > FULLY_DOWN.  Latter only happens
	 *  for relative axis.
	 */
    int force = (int)((((sensitivity * (255 * (__int64)value)) + BASE_SENSITIVITY / 2) / BASE_SENSITIVITY + FULLY_DOWN / 2) / FULLY_DOWN);
    AddForce(sum, b->command, force);
}

// Restricts d-pad/analog stick values to be from -255 to 255 and button values to be from 0 to 255.
// With D-pad in DS2 native mode, the negative and positive ranges are both independently from 0 to 255,
// which is why I use 9 bits of all sticks.  For left and right sticks, I have to remove a bit before sending.
void CapSum(ButtonSum *sum)
{
    int i;
    for (i = 0; i < 2; i++) {
        int div = std::max(abs(sum->sticks[i].horiz), abs(sum->sticks[i].vert));
        if (div > 255) {
            sum->sticks[i].horiz = sum->sticks[i].horiz * 255 / div;
            sum->sticks[i].vert = sum->sticks[i].vert * 255 / div;
        }
    }
    for (i = 0; i < 16; i++) {
        sum->buttons[i] = Cap(sum->buttons[i]);
    }
}

// Counter similar to stateUpdated for each pad, except used for PADkeyEvent instead.
// Only matters when GS thread updates is disabled (Just like summed pad values
// for pads beyond the first slot).

// Values, in order, correspond to PADkeyEvent, PADupdate(0), PADupdate(1), and
// WndProc(WMA_FORCE_UPDATE).  Last is always 0.
char padReadKeyUpdated[4] = {0, 0, 0, 0};

#define LOCK_DIRECTION 2
#define LOCK_BUTTONS 4
#define LOCK_BOTH 1

#ifdef _MSC_VER
struct EnterScopedSection
{
    CRITICAL_SECTION &m_cs;

    EnterScopedSection(CRITICAL_SECTION &cs)
        : m_cs(cs)
    {
        EnterCriticalSection(&m_cs);
    }

    ~EnterScopedSection()
    {
        LeaveCriticalSection(&m_cs);
    }
};
#endif

void Update(unsigned int port, unsigned int slot)
{
    char *stateUpdated;
    if (port < 2) {
        stateUpdated = &pads[port][slot].stateUpdated;
    } else if (port < 6) {
        stateUpdated = padReadKeyUpdated + port - 2;
    } else
        return;

    if (*stateUpdated > 0) {
        stateUpdated[0]--;
        return;
    }

// Lock prior to timecheck code to avoid pesky race conditions.
#ifdef __linux__
    std::lock_guard<std::mutex> lock(updateLock);
#else
    EnterScopedSection padlock(updateLock);
#endif

    static unsigned int LastCheck = 0;
    unsigned int t = timeGetTime();
    if (t - LastCheck < 15 || !openCount)
        return;

#ifdef _MSC_VER
    if (windowThreadId != GetCurrentThreadId()) {
        if (stateUpdated[0] < 0) {
            if (!updateQueued) {
                updateQueued = 1;
                PostMessage(hWnd, WMA_FORCE_UPDATE, FORCE_UPDATE_WPARAM, FORCE_UPDATE_LPARAM);
            }
        } else {
            stateUpdated[0]--;
        }
        return;
    }
#endif

    LastCheck = t;

    int i;
    ButtonSum s[2][4];
    u8 lockStateChanged[2][4];
    memset(lockStateChanged, 0, sizeof(lockStateChanged));

    for (i = 0; i < 8; i++) {
        s[i & 1][i >> 1] = pads[i & 1][i >> 1].lockedSum;
    }
#ifdef __linux__
    InitInfo info = {
        0, 0, GSdsp, GSwin};
#else
    InitInfo info = {
        0, 0, hWndTop, &hWndGSProc};
#endif
    dm->Update(&info);
    static int rapidFire = 0;
    rapidFire++;
    static bool anyDeviceActiveAndBound = true;
    bool currentDeviceActiveAndBound = false;
    for (i = 0; i < dm->numDevices; i++) {
        Device *dev = dm->devices[i];
        // Skip both disabled devices and inactive enabled devices.
        // Shouldn't be any of the latter, in general, but just in case...
        if (!dev->active)
            continue;
        for (int port = 0; port < 2; port++) {
            for (int slot = 0; slot < 4; slot++) {
                int padtype = config.padConfigs[port][slot].type;
                if (padtype == DisabledPad || !pads[port][slot].initialized)
                    continue;
                for (int j = 0; j < dev->pads[port][slot][padtype].numBindings; j++) {
                    Binding *b = dev->pads[port][slot][padtype].bindings + j;
                    int cmd = b->command;
                    int state = dev->virtualControlState[b->controlIndex];
                    if (!(rapidFire & b->rapidFire)) {
                        if ((cmd > 0x0F && cmd < 0x2A && cmd != 0x28) || cmd > 0x2C) {
                            ProcessButtonBinding(b, &s[port][slot], state);
                        } else if ((state >> 15) && !(dev->oldVirtualControlState[b->controlIndex] >> 15)) {
                            if (cmd == 0x0F) { // Mouse
                                miceEnabled = !miceEnabled;
                                UpdateEnabledDevices();
                            } else if (cmd == 0x2A) { // Lock Buttons
                                lockStateChanged[port][slot] |= LOCK_BUTTONS;
                            } else if (cmd == 0x2B) { // Lock Input
                                lockStateChanged[port][slot] |= LOCK_BOTH;
                            } else if (cmd == 0x2C) { // Lock Direction
                                lockStateChanged[port][slot] |= LOCK_DIRECTION;
                            } else if (cmd == 0x28 && !pads[port][slot].modeLock && padtype == Dualshock2Pad) { // Analog
                                if (pads[port][slot].mode == MODE_ANALOG)
                                    pads[port][slot].mode = MODE_DIGITAL;
                                else if (pads[port][slot].mode == MODE_DIGITAL)
                                    pads[port][slot].mode = MODE_ANALOG;
                            }
                        }
                    }
                }
            }
        }
        if (dev->attached && dev->pads[0][0][config.padConfigs[0][0].type].numBindings > 0) {
            if (!anyDeviceActiveAndBound) {
                fprintf(stderr, "LilyPad: A device(%ls) has been attached with bound controls.\n", dev->displayName);
                anyDeviceActiveAndBound = true;
            }
            currentDeviceActiveAndBound = true;
        }
    }
    if (!currentDeviceActiveAndBound && activeWindow) {
        if (anyDeviceActiveAndBound)
            fprintf(stderr, "LilyPad: Warning! No controls are bound to a currently attached device!\nPlease attach a controller that has been setup for use with LilyPad or go to the Plugin settings and setup new controls.\n");
        anyDeviceActiveAndBound = false;
    }
    dm->PostRead();

    {
        for (int port = 0; port < 2; port++) {
            for (int slot = 0; slot < 4; slot++) {
                for (int motor = 0; motor < 2; motor++) {
                    // TODO:  Probably be better to send all of these at once.
                    if (pads[port][slot].nextVibrate[motor] | pads[port][slot].currentVibrate[motor]) {
                        pads[port][slot].currentVibrate[motor] = pads[port][slot].nextVibrate[motor];
                        dm->SetEffect(port, slot, motor, pads[port][slot].nextVibrate[motor]);
                    }
                }
            }
        }

        for (int port = 0; port < 2; port++) {
            for (int slot = 0; slot < 4; slot++) {
                pads[port][slot].stateUpdated = 1;
                if (config.padConfigs[port][slot].type == DisabledPad || !pads[port][slot].initialized)
                    continue;
                if (config.padConfigs[port][slot].type == GuitarPad) {
                    if (!config.GH2) {
                        s[port][slot].sticks[0].vert = -s[port][slot].sticks[0].vert;
                    }
                    // GH2 hack.
                    else if (config.GH2) {
                        const unsigned int oldIdList[5] = {ID_R2, ID_CIRCLE, ID_TRIANGLE, ID_CROSS, ID_SQUARE};
                        const unsigned int idList[5] = {ID_L2, ID_L1, ID_R1, ID_R2, ID_CROSS};
                        int values[5];
                        int i;
                        for (i = 0; i < 5; i++) {
                            int id = oldIdList[i] - ID_DPAD_UP;
                            values[i] = s[port][slot].buttons[id];
                            s[port][slot].buttons[id] = 0;
                        }
                        s[port][slot].buttons[ID_TRIANGLE - ID_DPAD_UP] = values[1];
                        for (i = 0; i < 5; i++) {
                            int id = idList[i] - ID_DPAD_UP;
                            s[port][slot].buttons[id] = values[i];
                        }
                        if (s[port][slot].buttons[14] <= 48 && s[port][slot].buttons[12] <= 48) {
                            for (int i = 0; i < 5; i++) {
                                unsigned int id = idList[i] - ID_DPAD_UP;
                                if (pads[port][slot].sum.buttons[id] < s[port][slot].buttons[id]) {
                                    s[port][slot].buttons[id] = pads[port][slot].sum.buttons[id];
                                }
                            }
                        } else if (pads[port][slot].sum.buttons[14] <= 48 && pads[port][slot].sum.buttons[12] <= 48) {
                            for (int i = 0; i < 5; i++) {
                                unsigned int id = idList[i] - ID_DPAD_UP;
                                if (pads[port][slot].sum.buttons[id]) {
                                    s[port][slot].buttons[id] = 0;
                                }
                            }
                        }
                    }
                }

                if (pads[port][slot].mode == MODE_DIGITAL) {
                    for (int i = 0; i <= 1; i++) {
                        if (s[port][slot].sticks[i].horiz >= 100)
                            s[port][slot].buttons[13] += s[port][slot].sticks[i].horiz;
                        if (s[port][slot].sticks[i].horiz <= -100)
                            s[port][slot].buttons[15] -= s[port][slot].sticks[i].horiz;
                        if (s[port][slot].sticks[i].vert >= 100)
                            s[port][slot].buttons[14] += s[port][slot].sticks[i].vert;
                        if (s[port][slot].sticks[i].vert <= -100)
                            s[port][slot].buttons[12] -= s[port][slot].sticks[i].vert;
                    }
                }

                CapSum(&s[port][slot]);
                if (lockStateChanged[port][slot]) {
                    if (lockStateChanged[port][slot] & LOCK_BOTH) {
                        if (pads[port][slot].lockedState != (LOCK_DIRECTION | LOCK_BUTTONS)) {
                            // Enable the one that's not enabled.
                            lockStateChanged[port][slot] ^= pads[port][slot].lockedState ^ (LOCK_DIRECTION | LOCK_BUTTONS);
                        } else {
                            // Disable both
                            lockStateChanged[port][slot] ^= LOCK_DIRECTION | LOCK_BUTTONS;
                        }
                    }
                    if (lockStateChanged[port][slot] & LOCK_DIRECTION) {
                        if (pads[port][slot].lockedState & LOCK_DIRECTION) {
                            memset(pads[port][slot].lockedSum.sticks, 0, sizeof(pads[port][slot].lockedSum.sticks));
                        } else {
                            memcpy(pads[port][slot].lockedSum.sticks, s[port][slot].sticks, sizeof(pads[port][slot].lockedSum.sticks));
                        }
                        pads[port][slot].lockedState ^= LOCK_DIRECTION;
                    }
                    if (lockStateChanged[port][slot] & LOCK_BUTTONS) {
                        if (pads[port][slot].lockedState & LOCK_BUTTONS) {
                            memset(pads[port][slot].lockedSum.buttons, 0, sizeof(pads[port][slot].lockedSum.buttons));
                        } else {
                            memcpy(pads[port][slot].lockedSum.buttons, s[port][slot].buttons, sizeof(pads[port][slot].lockedSum.buttons));
                        }
                        pads[port][slot].lockedState ^= LOCK_BUTTONS;
                    }
                    for (i = 0; i < (int)sizeof(pads[port][slot].lockedSum) / 4; i++) {
                        if (((int *)&pads[port][slot].lockedSum)[i])
                            break;
                    }
                    if (i == sizeof(pads[port][slot].lockedSum) / 4) {
                        pads[port][slot].lockedState = 0;
                    }
                }
            }
        }
    }
    for (i = 0; i < 8; i++) {
        pads[i & 1][i >> 1].sum = s[i & 1][i >> 1];
    }

    padReadKeyUpdated[0] = padReadKeyUpdated[1] = padReadKeyUpdated[2] = 1;

    if (stateUpdated[0] > 0)
        --stateUpdated[0];
}

void CALLBACK PADupdate(int port)
{
    Update(port + 3, 0);
}

inline void SetVibrate(int port, int slot, int motor, u8 val)
{
    pads[port][slot].nextVibrate[motor] = val;
}

u32 CALLBACK PS2EgetLibType(void)
{
    return PS2E_LT_PAD;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type)
{
    if (type == PS2E_LT_PAD)
        return (PS2E_PAD_VERSION << 16) | VERSION;
    return 0;
}

#ifdef _MSC_VER
// Used in about and config screens.
void GetNameAndVersionString(wchar_t *out)
{
#if defined(PCSX2_DEBUG)
    wsprintfW(out, L"LilyPad Debug %i.%i.%i (%lld)", (VERSION >> 8) & 0xFF, VERSION & 0xFF, (VERSION >> 24) & 0xFF, SVN_REV);
#else
    wsprintfW(out, L"LilyPad %i.%i.%i (%lld)", (VERSION >> 8) & 0xFF, VERSION & 0xFF, (VERSION >> 24) & 0xFF, SVN_REV);
#endif
}
#endif

char *CALLBACK PS2EgetLibName(void)
{
#if defined(PCSX2_DEBUG)
    static char version[50];
    sprintf(version, "LilyPad Debug (%lld)", SVN_REV);
    return version;
#else
    static char version[50];
    sprintf(version, "LilyPad (%lld)", SVN_REV);
    return version;
#endif
}

//void CALLBACK PADgsDriverInfo(GSdriverInfo *info) {
//	info=info;
//}

void CALLBACK PADshutdown()
{
    DEBUG_TEXT_OUT("LilyPad shutdown.\n\n");
    for (int i = 0; i < 8; i++)
        pads[i & 1][i >> 1].initialized = 0;
    portInitialized[0] = portInitialized[1] = 0;
    UnloadConfigs();
}

inline void StopVibrate()
{
    for (int i = 0; i < 8; i++) {
        SetVibrate(i & 1, i >> 1, 0, 0);
        SetVibrate(i & 1, i >> 1, 1, 0);
    }
}

inline void ResetVibrate(int port, int slot)
{
    SetVibrate(port, slot, 0, 0);
    SetVibrate(port, slot, 1, 0);
    pads[port][slot].vibrate[0] = 0x5A;
    for (int i = 1; i < 8; ++i)
        pads[port][slot].vibrate[i] = 0xFF;
}

void ResetPad(int port, int slot)
{
    // Lines before memset currently don't do anything useful,
    // but allow this function to be called at any time.

    // Need to backup, so can be called at any point.
    u8 enabled = pads[port][slot].enabled;

    // Currently should never do anything.
    SetVibrate(port, slot, 0, 0);
    SetVibrate(port, slot, 1, 0);

    memset(&pads[port][slot], 0, sizeof(pads[0][0]));
    if (config.padConfigs[port][slot].type == MousePad)
        pads[port][slot].mode = MODE_PS1_MOUSE;
    else if (config.padConfigs[port][slot].type == neGconPad)
        pads[port][slot].mode = MODE_NEGCON;
    else
        pads[port][slot].mode = MODE_DIGITAL;

    pads[port][slot].umask[0] = pads[port][slot].umask[1] = 0xFF;
    // Sets up vibrate variable.
    ResetVibrate(port, slot);
    pads[port][slot].initialized = 1;

    pads[port][slot].enabled = enabled;

    pads[port][slot].previousType = config.padConfigs[port][slot].type;

    pads[port][slot].config = 0;
}


struct QueryInfo
{
    u8 port;
    u8 slot;
    u8 lastByte;
    u8 currentCommand;
    u8 numBytes;
    u8 queryDone;
    u8 response[42];
} query = {0, 0, 0, 0, 0, 0xFF, {0xF3}};

s32 CALLBACK PADinit(u32 flags)
{
    // Note:  Won't load settings if already loaded.
    if (LoadSettings() < 0) {
        return -1;
    }
    int port = (flags & 3);
    if (port == 3) {
        if (PADinit(1) == -1)
            return -1;
        return PADinit(2);
    }

#if defined(PCSX2_DEBUG) && defined(_MSC_VER)
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif

    port--;

    for (int slot = 0; slot < 4; slot++)
        ResetPad(port, slot);
    slots[port] = 0;
    portInitialized[port] = 1;

    query.lastByte = 1;
    query.numBytes = 0;
    ClearKeyQueue();
#ifdef __linux__
    R_ClearKeyQueue();
#endif
    // Just in case, when resuming emulation.
    ReleaseModifierKeys();

    DEBUG_TEXT_OUT("LilyPad initialized\n\n");
    return 0;
}



// Note to self:  Has to be a define for the sizeof() to work right.
// Note to self 2: All are the same size, anyways, except for longer full DS2 response
//   and shorter digital mode response.
#define SET_RESULT(a)                             \
    {                                             \
        memcpy(query.response + 2, a, sizeof(a)); \
        query.numBytes = 2 + sizeof(a);           \
    }

#define SET_FINAL_RESULT(a)                       \
    {                                             \
        memcpy(query.response + 2, a, sizeof(a)); \
        query.numBytes = 2 + sizeof(a);           \
        query.queryDone = 1;                      \
    }

static const u8 ConfigExit[7] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//static const u8 ConfigExit[7] = {0x5A, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const u8 noclue[7] = {0x5A, 0x00, 0x00, 0x02, 0x00, 0x00, 0x5A};
static u8 queryMaskMode[7] = {0x5A, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x5A};
//static const u8 DSNonNativeMode[7] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 setMode[7] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// DS2
static const u8 queryModelDS2[7] = {0x5A, 0x03, 0x02, 0x00, 0x02, 0x01, 0x00};
// DS1
static const u8 queryModelDS1[7] = {0x5A, 0x01, 0x02, 0x00, 0x02, 0x01, 0x00};

static const u8 queryAct[2][7] = {{0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A},
                                  {0x5A, 0x00, 0x00, 0x01, 0x01, 0x01, 0x14}};

static const u8 queryComb[7] = {0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00};

static const u8 queryMode[7] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


static const u8 setNativeMode[7] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A};

#ifdef _MSC_VER
// Useful sequence before changing into active/inactive state.
// Handles hooking/unhooking of mouse and KB and also mouse cursor visibility.
// towardsActive==true indicates we're gaining activity (on focus etc), false is for losing activity (on close, kill focus, etc).
void PrepareActivityState(bool towardsActive)
{
    if (!towardsActive)
        ReleaseModifierKeys();
    activeWindow = towardsActive;
    UpdateEnabledDevices();
}

// responsible for monitoring device addition/removal, focus changes, and viewport closures.
ExtraWndProcResult StatusWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output)
{
    switch (uMsg) {
        case WMA_FORCE_UPDATE:
            if (wParam == FORCE_UPDATE_WPARAM && lParam == FORCE_UPDATE_LPARAM) {
                if (updateQueued) {
                    updateQueued = 0;
                    Update(5, 0);
                }
                return NO_WND_PROC;
            }
        case WM_DEVICECHANGE:
            if (wParam == DBT_DEVNODES_CHANGED) {
                UpdateEnabledDevices(1);
            }
            break;
        case WM_ACTIVATE:
            // Release any buttons PCSX2 may think are down when
            // losing/gaining focus.
            // Note - I never managed to see this case entered, but SET/KILLFOCUS are entered. - avih 2014-04-16
            PrepareActivityState(LOWORD(wParam) != WA_INACTIVE);
            break;
        case WM_DESTROY:
            QueueKeyEvent(VK_ESCAPE, KEYPRESS);
            break;
        case WM_KILLFOCUS:
            PrepareActivityState(false);
            break;
        case WM_SETFOCUS:
            PrepareActivityState(true);
            break;
        default:
            break;
    }
    return CONTINUE_BLISSFULLY;
}

// All that's needed to force hiding the cursor in the proper thread.
// Could have a special case elsewhere, but this make sure it's called
// only once, rather than repeatedly.
ExtraWndProcResult HideCursorProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output)
{
    ShowCursor(0);
    return CONTINUE_BLISSFULLY_AND_RELEASE_PROC;
}

char restoreFullScreen = 0;
// This hack sends ALT+ENTER to the window to toggle fullscreen.
// PCSX2 doesn't need it (it exits full screen on ESC on its own).
DWORD WINAPI MaximizeWindowThreadProc(void *lpParameter)
{
    Sleep(100);
    keybd_event(VK_LMENU, MapVirtualKey(VK_LMENU, MAPVK_VK_TO_VSC), 0, 0);
    keybd_event(VK_RETURN, MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC), 0, 0);
    Sleep(10);
    keybd_event(VK_RETURN, MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC), KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LMENU, MapVirtualKey(VK_LMENU, MAPVK_VK_TO_VSC), KEYEVENTF_KEYUP, 0);
    return 0;
}
#endif

void CALLBACK PADconfigure()
{
    if (openCount) {
        return;
    }
    Configure();
}

#ifdef _MSC_VER
DWORD WINAPI RenameWindowThreadProc(void *lpParameter)
{
    wchar_t newTitle[200];
    if (hWndTop) {
        int len = GetWindowTextW(hWndTop, newTitle, 200);
        if (len > 0 && len < 199) {
            wchar_t *end;
            if (end = wcsstr(newTitle, L" | State "))
                *end = 0;
            SetWindowTextW(hWndTop, newTitle);
        }
    }
    return 0;
}
#endif

s32 CALLBACK PADopen(void *pDsp)
{
    if (openCount++)
        return 0;
    DEBUG_TEXT_OUT("LilyPad opened\n\n");

    miceEnabled = !config.mouseUnfocus;
#ifdef _MSC_VER
    if (!hWnd) {
        if (IsWindow((HWND)pDsp)) {
            hWnd = (HWND)pDsp;
        } else if (pDsp && !IsBadReadPtr(pDsp, 4) && IsWindow(*(HWND *)pDsp)) {
            hWnd = *(HWND *)pDsp;
        } else {
            openCount = 0;
            MessageBoxA(GetActiveWindow(),
                        "Invalid Window handle passed to LilyPad.\n"
                        "\n"
                        "Either your emulator or gs plugin is buggy,\n"
                        "Despite the fact the emulator is about to\n"
                        "blame LilyPad for failing to initialize.",
                        "Non-LilyPad Error", MB_OK | MB_ICONERROR);
            return -1;
        }
        hWndTop = hWnd;
        while (GetWindowLong(hWndTop, GWL_STYLE) & WS_CHILD)
            hWndTop = GetParent(hWndTop);

        if (!hWndGSProc.SetWndHandle(hWnd)) {
            openCount = 0;
            return -1;
        }

        // Implements most hacks, as well as enabling/disabling mouse
        // capture when focus changes.
        updateQueued = 0;
        hWndGSProc.Eat(StatusWndProc, 0);

        if (hWnd != hWndTop) {
            if (!hWndTopProc.SetWndHandle(hWndTop)) {
                openCount = 0;
                return -1;
            }
        }

        if (config.forceHide) {
            hWndGSProc.Eat(HideCursorProc, 0);
        }

        windowThreadId = GetWindowThreadProcessId(hWndTop, 0);
    }

    if (restoreFullScreen) {
        if (!IsWindowMaximized(hWndTop)) {
            HANDLE hThread = CreateThread(0, 0, MaximizeWindowThreadProc, hWndTop, 0, 0);
            if (hThread)
                CloseHandle(hThread);
        }
        restoreFullScreen = 0;
    }
#endif
    for (int port = 0; port < 2; port++) {
        for (int slot = 0; slot < 4; slot++) {
            memset(&pads[port][slot].sum, 0, sizeof(pads[port][slot].sum));
            memset(&pads[port][slot].lockedSum, 0, sizeof(pads[port][slot].lockedSum));
            pads[port][slot].lockedState = 0;

            if (config.padConfigs[port][slot].type != pads[port][slot].previousType) {
                ResetPad(port, slot);
            }
        }
    }

#ifdef _MSC_VER
// I'd really rather use this line, but GetActiveWindow() does not have complete specs.
// It *seems* to return null when no window from this thread has focus, but the
// Microsoft specs seem to imply it returns the window from this thread that would have focus,
// if any window did (topmost in this thread?).  Which isn't what I want, and doesn't seem
// to be what it actually does.
// activeWindow = GetActiveWindow() == hWnd;

// activeWindow = (GetAncestor(hWnd, GA_ROOT) == GetAncestor(GetForegroundWindow(), GA_ROOT));
#else
    // Not used so far
    GSdsp = *(Display **)pDsp;
    GSwin = (Window) * (((uptr *)pDsp) + 1);
#endif
    activeWindow = 1;
    UpdateEnabledDevices();
    return 0;
}

void CALLBACK PADclose()
{
    if (openCount && !--openCount) {
        DEBUG_TEXT_OUT("LilyPad closed\n\n");
#ifdef _MSC_VER
        updateQueued = 0;
        hWndGSProc.Release();
        hWndTopProc.Release();
        dm->ReleaseInput();
        hWnd = 0;
        hWndTop = 0;
#else
        R_ClearKeyQueue();
#endif
        ClearKeyQueue();
    }
}

u8 CALLBACK PADstartPoll(int port)
{
    DEBUG_NEW_SET();
    port--;
    if ((unsigned int)port <= 1 && pads[port][slots[port]].enabled) {
        query.queryDone = 0;
        query.port = port;
        query.slot = slots[port];
        query.numBytes = 2;
        query.lastByte = 0;
        DEBUG_IN(port);
        DEBUG_OUT(0xFF);
        DEBUG_IN(slots[port]);
        DEBUG_OUT(pads[port][slots[port]].enabled);
        return 0xFF;
    } else {
        query.queryDone = 1;
        query.numBytes = 0;
        query.lastByte = 1;
        DEBUG_IN(0);
        DEBUG_OUT(0);
        DEBUG_IN(port);
        DEBUG_OUT(0);
        return 0;
    }
}

inline int IsDualshock2(u8 port, u8 slot)
{
    return config.padConfigs[query.port][query.slot].type == Dualshock2Pad ||
           (config.padConfigs[query.port][query.slot].type == GuitarPad && config.GH2);
}

u8 CALLBACK PADpoll(u8 value)
{
    DEBUG_IN(value);
    if (query.lastByte + 1 >= query.numBytes) {
        DEBUG_OUT(0);
        return 0;
    }
    if (query.lastByte && query.queryDone) {
        DEBUG_OUT(query.response[1 + query.lastByte]);
        return query.response[++query.lastByte];
    }

    Pad *pad = &pads[query.port][query.slot];
    int padtype = config.padConfigs[query.port][query.slot].type;

    if (query.lastByte == 0) {
        query.lastByte++;
        query.currentCommand = value;

        // Only the 0x42(read input and vibration) and 0x43(enter or exit config mode) command cases work outside of config mode, the other cases will be avoided.
        if ((!pad->config && value != 0x42 && value != 0x43) || (padtype == neGconPad && (value < 0x40 || value > 0x45))) {
            query.numBytes = 0;
            query.queryDone = 1;
            DEBUG_OUT(0xF3);
            return 0xF3;
        }
        switch (value) {
            // CONFIG_MODE
            case 0x43:
                if (pad->config && padtype != neGconPad) {
                    // In config mode.  Might not actually be leaving it.
                    SET_RESULT(ConfigExit);
                    DEBUG_OUT(0xF3);
                    return 0xF3;
                }
                // Fall through
                
            // READ_DATA_AND_VIBRATE
            case 0x42:
                query.response[2] = 0x5A;
                {
                    Update(query.port, query.slot);
                    ButtonSum *sum = &pad->sum;

                    if (padtype == MousePad) {
                        u8 b1 = 0xFC;
                        if (sum->buttons[9] > 0) // Left button
                            b1 -= 8;
                        if (sum->buttons[10] > 0) // Right button
                            b1 -= 4;

                        query.response[3] = 0xFF;
                        query.response[4] = b1;
                        query.response[5] = sum->sticks[1].horiz / 2;
                        query.response[6] = sum->sticks[1].vert / 2;
                        query.numBytes = 7;
                        query.lastByte = 1;
                        DEBUG_OUT(MODE_PS1_MOUSE);
                        return MODE_PS1_MOUSE;
                    }
                    if (padtype == neGconPad) {
                        u8 b1 = 0xFF, b2 = 0xFF;
                        b1 -= (sum->buttons[3] > 0) << 3; // Start

                        for (int i = 3; i < 6; i++) {
                            b2 -= (sum->buttons[i + 4] > 0) << i; // R, A, B
                        }
                        for (int i = 4; i < 8; i++) {
                            b1 -= (sum->buttons[i + 8] > 0) << i; // D-pad Up, Right, Down, Left
                        }

                        query.response[3] = b1;
                        query.response[4] = b2;
                        query.response[5] = Cap((sum->sticks[1].horiz + 255) / 2); // Swivel
                        query.response[6] = (unsigned char)sum->buttons[10];       // I
                        query.response[7] = (unsigned char)sum->buttons[11];       // II
                        query.response[8] = (unsigned char)sum->buttons[6];        // L

                        query.numBytes = 9;
                        query.lastByte = 1;
                        DEBUG_OUT(MODE_NEGCON);
                        return MODE_NEGCON;
                    }

                    u8 b1 = 0xFF, b2 = 0xFF;
                    for (int i = 0; i < 4; i++) {
                        b1 -= (sum->buttons[i] > 0) << i;
                    }
                    for (int i = 0; i < 8; i++) {
                        b2 -= (sum->buttons[i + 4] > 0) << i;
                    }

                    if (padtype == GuitarPad && !config.GH2) {
                        sum->buttons[15] = 255;
                        // Not sure about this.  Forces wammy to be from 0 to 0x7F.
                        // if (sum->sticks[2].vert > 0) sum->sticks[2].vert = 0;
                    }

                    for (int i = 4; i < 8; i++) {
                        b1 -= (sum->buttons[i + 8] > 0) << i;
                    }

                    //Left, Right and Down are always pressed on Pop'n Music controller.
                    if (padtype == PopnPad)
                        b1 = b1 & 0x1f;

                    query.response[3] = b1;
                    query.response[4] = b2;

                    query.numBytes = 5;
                    if (pad->mode != MODE_DIGITAL) {
                        query.response[5] = Cap((sum->sticks[0].horiz + 255) / 2); // Right stick: left & right
                        query.response[6] = Cap((sum->sticks[0].vert + 255) / 2);  // Right stick: up & down
                        query.response[7] = Cap((sum->sticks[1].horiz + 255) / 2); // Left stick: left & right
                        query.response[8] = Cap((sum->sticks[1].vert + 255) / 2);  // Left stick: up & down

                        query.numBytes = 9;
                        if (pad->mode != MODE_ANALOG && !pad->config) {
                            // Good idea?  No clue.
                            //query.response[3] &= pad->mask[0];
                            //query.response[4] &= pad->mask[1];

                            // No need to cap these, already done int CapSum().
                            query.response[9] = (unsigned char)sum->buttons[13];  // D-pad right
                            query.response[10] = (unsigned char)sum->buttons[15]; // D-pad left
                            query.response[11] = (unsigned char)sum->buttons[12]; // D-pad up
                            query.response[12] = (unsigned char)sum->buttons[14]; // D-pad down

                            query.response[13] = (unsigned char)sum->buttons[8];  // Triangle
                            query.response[14] = (unsigned char)sum->buttons[9];  // Circle
                            query.response[15] = (unsigned char)sum->buttons[10]; // Cross
                            query.response[16] = (unsigned char)sum->buttons[11]; // Square

                            query.response[17] = (unsigned char)sum->buttons[6]; // L1
                            query.response[18] = (unsigned char)sum->buttons[7]; // R1
                            query.response[19] = (unsigned char)sum->buttons[4]; // L2
                            query.response[20] = (unsigned char)sum->buttons[5]; // R2
                            query.numBytes = 21;
                        }
                    }
                }
                query.lastByte = 1;
                DEBUG_OUT(pad->mode);
                return pad->mode;
            // SET_VREF_PARAM
            case 0x40:
                SET_FINAL_RESULT(noclue);
                break;
            // QUERY_DS2_ANALOG_MODE
            case 0x41:
                // Right?  Wrong?  No clue.
                if (pad->mode == MODE_DIGITAL || pad->mode == MODE_PS1_MOUSE || pad->mode == MODE_NEGCON) {
                    queryMaskMode[1] = queryMaskMode[2] = queryMaskMode[3] = 0;
                    queryMaskMode[6] = 0x00;
                } else {
                    queryMaskMode[1] = pad->umask[0];
                    queryMaskMode[2] = pad->umask[1];
                    queryMaskMode[3] = 0x03;
                    // Not entirely sure about this.
                    //queryMaskMode[3] = 0x01 | (pad->mode == MODE_DS2_NATIVE)*2;
                    queryMaskMode[6] = 0x5A;
                }
                SET_FINAL_RESULT(queryMaskMode);
                break;
            // SET_MODE_AND_LOCK
            case 0x44:
                SET_RESULT(setMode);
                ResetVibrate(query.port, query.slot);
                break;
            // QUERY_MODEL_AND_MODE
            case 0x45:
                if (IsDualshock2(query.port, query.slot)) {
                    SET_FINAL_RESULT(queryModelDS2);
                } else {
                    SET_FINAL_RESULT(queryModelDS1);
                }
                // Not digital mode.
                query.response[5] = (pad->mode & 0xF) != 1;
                break;
            // QUERY_ACT
            case 0x46:
                SET_RESULT(queryAct[0]);
                break;
            // QUERY_COMB
            case 0x47:
                SET_FINAL_RESULT(queryComb);
                break;
            // QUERY_MODE
            case 0x4C:
                SET_RESULT(queryMode);
                break;
            // VIBRATION_TOGGLE
            case 0x4D:
                memcpy(query.response + 2, pad->vibrate, 7);
                query.numBytes = 9;
                ResetVibrate(query.port, query.slot);
                break;
            // SET_DS2_NATIVE_MODE
            case 0x4F:
                if (IsDualshock2(query.port, query.slot)) {
                    SET_RESULT(setNativeMode);
                } else {
                    SET_FINAL_RESULT(setNativeMode);
                }
                break;
            default:
                query.numBytes = 0;
                query.queryDone = 1;
                break;
        }
        DEBUG_OUT(0xF3);
        return 0xF3;
    } else {
        query.lastByte++;

        // Only the 0x42(read input and vibration) and 0x43(enter or exit config mode) command cases work outside of config mode, the other cases will be avoided.
        if ((!pad->config && query.currentCommand != 0x42 && query.currentCommand != 0x43) || (padtype == neGconPad && (query.currentCommand < 0x40 || query.currentCommand > 0x45))) {
            DEBUG_OUT(query.response[query.lastByte]);
            return query.response[query.lastByte];
        }
        switch (query.currentCommand) {
            // READ_DATA_AND_VIBRATE
            case 0x42:
                if (query.lastByte == pad->vibrateI[0]) {
                    SetVibrate(query.port, query.slot, 1, 255 * (value & 1));
                } else if (query.lastByte == pad->vibrateI[1]) {
                    SetVibrate(query.port, query.slot, 0, value);
                }
                break;
            // CONFIG_MODE
            case 0x43:
                if (query.lastByte == 3) {
                    query.queryDone = 1;
                    pad->config = value;
                }
                break;
            // SET_MODE_AND_LOCK
            case 0x44:
                if (query.lastByte == 3 && value < 2) {
                    if (padtype == MousePad) {
                        pad->mode = MODE_PS1_MOUSE;
                    } else if (padtype == neGconPad) {
                        pad->mode = MODE_NEGCON;
                    } else {
                        static const u8 modes[2] = {MODE_DIGITAL, MODE_ANALOG};
                        pad->mode = modes[value];
                    }
                } else if (query.lastByte == 4) {
                    if (value == 3) {
                        pad->modeLock = 3;
                    } else {
                        pad->modeLock = 0;
                    }
                    query.queryDone = 1;
                }
                break;
            // QUERY_ACT
            case 0x46:
                if (query.lastByte == 3) {
                    if (value < 2)
                        SET_RESULT(queryAct[value])
                    // bunch of 0's
                    // else SET_RESULT(setMode);
                    query.queryDone = 1;
                }
                break;
            // QUERY_MODE
            case 0x4C:
                if (query.lastByte == 3 && value < 2) {
                    query.response[6] = 4 + value * 3;
                    query.queryDone = 1;
                }
                // bunch of 0's
                //else data = setMode;
                break;
            // VIBRATION_TOGGLE
            case 0x4D:
                if (query.lastByte >= 3) {
                    if (value == 0) {
                        pad->vibrateI[0] = (u8)query.lastByte;
                    } else if (value == 1) {
                        pad->vibrateI[1] = (u8)query.lastByte;
                    }
                    pad->vibrate[query.lastByte - 2] = value;
                }
                break;
            // SET_DS2_NATIVE_MODE
            case 0x4F:
                if (query.lastByte == 3 || query.lastByte == 4) {
                    pad->umask[query.lastByte - 3] = value;
                } else if (query.lastByte == 5) {
                    if (!(value & 1)) {
                        pad->mode = MODE_DIGITAL;
                    } else if (!(value & 2)) {
                        pad->mode = MODE_ANALOG;
                    } else {
                        pad->mode = MODE_DS2_NATIVE;
                    }
                }
                break;
            default:
                DEBUG_OUT(0);
                return 0;
        }
        DEBUG_OUT(query.response[query.lastByte]);
        return query.response[query.lastByte];
    }
}

// returns: 1 if supports pad1
//			2 if supports pad2
//			3 if both are supported
u32 CALLBACK PADquery()
{
    return 3;
}

#ifdef _MSC_VER
INT_PTR CALLBACK AboutDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG) {
        wchar_t idString[100];
        GetNameAndVersionString(idString);
        SetDlgItemTextW(hWndDlg, IDC_VERSION, idString);
    } else if (uMsg == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)) {
        EndDialog(hWndDlg, 0);
        return 1;
    }
    return 0;
}
#endif


void CALLBACK PADabout()
{
#ifdef _MSC_VER
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), 0, AboutDialogProc);
#endif
}

s32 CALLBACK PADtest()
{
    return 0;
}

keyEvent *CALLBACK PADkeyEvent()
{
    // If running both pads, ignore every other call.  So if two keys pressed in same interval...
    static char eventCount = 0;
    eventCount++;
    if (eventCount < openCount) {
        return 0;
    }
    eventCount = 0;

    Update(2, 0);
    static keyEvent ev;
    if (!GetQueuedKeyEvent(&ev))
        return 0;

#ifdef _MSC_VER
    static char shiftDown = 0;
    static char altDown = 0;
    if (!activeWindow)
        altDown = shiftDown = 0;

    if (miceEnabled && (ev.key == VK_ESCAPE || (int)ev.key == -2) && ev.evt == KEYPRESS) {
        // Disable mouse/KB hooks on escape (before going into paused mode).
        // This is a hack, since PADclose (which is called on pause) should enevtually also deactivate the
        // mouse/kb capture. In practice, WindowsMessagingMouse::Deactivate is called from PADclose, but doesn't
        // manage to release the mouse, maybe due to the thread from which it's called or some
        // state or somehow being too late.
        // This explicitly triggers inactivity (releasing mouse/kb hooks) before PCSX2 starts to close the plugins.
        // Regardless, the mouse/kb hooks will get re-enabled on resume if required without need for further hacks.

        PrepareActivityState(false);
    }

    // So don't change skip mode on alt-F4.
    if (ev.key == VK_F4 && altDown) {
        return 0;
    }

    if (ev.key == VK_LSHIFT || ev.key == VK_RSHIFT || ev.key == VK_SHIFT) {
        ev.key = VK_SHIFT;
        shiftDown = (ev.evt == KEYPRESS);
    } else if (ev.key == VK_LCONTROL || ev.key == VK_RCONTROL) {
        ev.key = VK_CONTROL;
    } else if (ev.key == VK_LMENU || ev.key == VK_RMENU || ev.key == VK_SHIFT) {
        ev.key = VK_MENU;
        altDown = (ev.evt == KEYPRESS);
    }
#endif
    return &ev;
}

struct PadPluginFreezeData
{
    char format[8];
    // Currently all different versions are incompatible.
    // May split into major/minor with some compatibility rules.
    u32 version;
    // So when loading, know which plugin's settings I'm loading.
    // Not a big deal.  Use a static variable when saving to figure it out.
    u8 port;
    // active slot for port
    u8 slot[2];
    PadFreezeData padData[2][4];
    QueryInfo query;
};

s32 CALLBACK PADfreeze(int mode, freezeData *data)
{
    if (!data) {
        printf("LilyPad savestate null pointer!\n");
        return -1;
    }

    if (mode == FREEZE_SIZE) {
        data->size = sizeof(PadPluginFreezeData);
    } else if (mode == FREEZE_LOAD) {
        PadPluginFreezeData &pdata = *(PadPluginFreezeData *)(data->data);
        StopVibrate();
        if (data->size != sizeof(PadPluginFreezeData) ||
            pdata.version != PAD_SAVE_STATE_VERSION ||
            strcmp(pdata.format, "PadMode"))
            return 0;

        if (pdata.port >= 2)
            return 0;

        query = pdata.query;
        if (pdata.query.slot < 4) {
            query = pdata.query;
        }

        // Tales of the Abyss - pad fix
        // - restore data for both ports
        for (int port = 0; port < 2; port++) {
            for (int slot = 0; slot < 4; slot++) {
                u8 mode = pdata.padData[port][slot].mode;

                if (mode != MODE_DIGITAL && mode != MODE_ANALOG && mode != MODE_DS2_NATIVE && mode != MODE_PS1_MOUSE && mode != MODE_NEGCON) {
                    break;
                }

                // Not sure if the cast is strictly necessary, but feel safest with it there...
                *(PadFreezeData *)&pads[port][slot] = pdata.padData[port][slot];
            }

            if (pdata.slot[port] < 4)
            slots[port] = pdata.slot[port];
        }
    } else if (mode == FREEZE_SAVE) {
        if (data->size != sizeof(PadPluginFreezeData))
            return 0;
        PadPluginFreezeData &pdata = *(PadPluginFreezeData *)(data->data);


        // Tales of the Abyss - pad fix
        // - PCSX2 only saves port0 (save #1), then port1 (save #2)

        memset(&pdata, 0, sizeof(pdata));
        strcpy(pdata.format, "PadMode");
        pdata.version = PAD_SAVE_STATE_VERSION;
        pdata.port = 0;
        pdata.query = query;

        for (int port = 0; port < 2; port++) {
            for (int slot = 0; slot < 4; slot++) {
                pdata.padData[port][slot] = pads[port][slot];
            }

            pdata.slot[port] = slots[port];
        }
    } else
        return -1;
    return 0;
}

u32 CALLBACK PADreadPort1(PadDataS *pads)
{
    PADstartPoll(1);
    PADpoll(0x42);
    memcpy(pads, query.response + 1, 7);
    pads->controllerType = pads[0].controllerType >> 4;
    memset(pads + 7, 0, sizeof(PadDataS) - 7);
    return 0;
}

u32 CALLBACK PADreadPort2(PadDataS *pads)
{
    PADstartPoll(2);
    PADpoll(0x42);
    memcpy(pads, query.response + 1, 7);
    pads->controllerType = pads->controllerType >> 4;
    memset(pads + 7, 0, sizeof(PadDataS) - 7);
    return 0;
}

s32 CALLBACK PADqueryMtap(u8 port)
{
    port--;
    if (port > 1)
        return 0;
    return config.multitap[port];
}

s32 CALLBACK PADsetSlot(u8 port, u8 slot)
{
    port--;
    slot--;
    if (port > 1 || slot > 3) {
        return 0;
    }
    // Even if no pad there, record the slot, as it is the active slot regardless.
    slots[port] = slot;
    // First slot always allowed.
    // return pads[port][slot].enabled | !slot;
    return 1;
}

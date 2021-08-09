/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
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

#include "padproxy.h"
#include "padconfig.h"
#include "usb-pad.h"
#include "USB/qemu-usb/desc.h"
#include "USB/shared/inifile_usb.h"
#include "USB/usb-hid/hidproxy.h"
#include "USB/usb-hid/usb-hid.h"
#include "usb-guncon2-wx.h"

extern void OSDCursorPos(float x, float y, float w, float h);
#ifdef WIN32
extern HWND gsWnd;
#endif

namespace usb_pad
{

#define BUTTON_TRIGGER 0x2000
#define BUTTON_A 0x0008
#define BUTTON_B 0x0004
#define BUTTON_C 0x0002
#define BUTTON_SELECT 0x4000
#define BUTTON_START 0x8000
#define DPAD_UP 0x0010
#define DPAD_DOWN 0x0040
#define DPAD_LEFT 0x0080
#define DPAD_RIGHT 0x0020

/*
Progressive scan = 2-shot calibration
1) Point the gun away from the TV screen
2) Hold the trigger and press the select button on the gun.
3) This toggles the words ''''Progressive scan'' to ''100hz''

Seems to work better than interlace
*/
#define PROGRESSIVE_SCAN 0x0100

// (mouse only!! real EMS TopGun 2 should do this correctly)
//
// both progressive / interlace
// - to make things worse, game measures -RESPONSE- time (fast fire)
// - seems to work best ~8-9
#define LATCH_DELAY (8 + 1)

#define GUNCON2_SET_PARAM 9

#define GUNCON_RELOAD_MANUAL 0
#define GUNCON_RELOAD_SEMI 1

#define GUNCON_SHOW_CURSOR 0
#define GUNCON_HIDE_CURSOR 1

#define GUNCON_MODEL_NAMCO 0

	static const USBDescStrings desc_strings = {
		"",
	};

	static const uint8_t dev_descriptor[] = {
		0x12, // bLength
		0x01, // bDescriptorType (Device)
		WBVAL(0x0110), // bcdUSB 1.00
		0xFF, // bDeviceClass
		0x00, // bDeviceSubClass
		0x00, // bDeviceProtocol
		0x08, // bMaxPacketSize0 8
		WBVAL(0x0B9A), // idVendor 0x0B9A
		WBVAL(0x016A), // idProduct 0x016A
		WBVAL(0x0100), // bcdDevice 2.00
		0x00, // iManufacturer (String Index)
		0x00, // iProduct (String Index)
		0x00, // iSerialNumber (String Index)
		0x01, // bNumConfigurations 1

		// 18 bytes
	};

	static const uint8_t hid_report_descriptor[] = {
		0};

	static const uint8_t config_descriptor[] = {
		0x09, // bLength
		0x02, // bDescriptorType (Configuration)
		WBVAL(25), // wTotalLength 25
		0x01, // bNumInterfaces 1
		0x01, // bConfigurationValue
		0x00, // iConfiguration (String Index)
		0x80, // bmAttributes
		0x19, // bMaxPower 50mA

		0x09, // bLength
		USB_INTERFACE_DESCRIPTOR_TYPE, // bDescriptorType (Interface)
		0x00, // bInterfaceNumber 0
		0x00, // bAlternateSetting
		0x01, // bNumEndpoints 1
		USB_CLASS_VENDOR_SPEC, // bInterfaceClass
		0x6A, // bInterfaceSubClass
		0x00, // bInterfaceProtocol
		0x00, // iInterface (String Index)

		0x07, // bLength
		USB_ENDPOINT_DESCRIPTOR_TYPE, // bDescriptorType (Endpoint)
		USB_ENDPOINT_IN(1), // bEndpointAddress (IN/D2H)
		USB_ENDPOINT_TYPE_INTERRUPT, // bmAttributes (Interrupt)
		WBVAL(8), // wMaxPacketSize 8
		0x08, // bInterval 8 (unit depends on device speed)

		// 25 bytes
	};

	std::vector<std::string> Guncon2Device::ListAPIs()
	{
		return usb_hid::RegisterUsbHID::instance().Names();
	}

	const TCHAR* Guncon2Device::LongAPIName(const std::string& name)
	{
		auto proxy = usb_hid::RegisterUsbHID::instance().Proxy(name);
		if (proxy)
			return proxy->Name();
		return nullptr;
	}

	typedef struct Guncon2State
	{
		USBDevice dev;
		USBDesc desc;
		USBDescDevice desc_dev;
		// 		Pad* pad;
		uint8_t port;

		usb_hid::UsbHID *usbhid_ms, *usbhid_kbd;
		USBEndpoint *intr_ms, *intr_kbd;
		HIDState hid_mouse;
		HIDState hid_kbd;

		struct freeze
		{
			int guncon_analog_x = 640 / 2;
			int guncon_analog_y = 480 / 2;

			int current_analog_x;
			int current_analog_y;
			int current_analog_z;
			int buttons[5]{};

			bool absolute;
			// trigger -> flash -> get x/y data
			int latch_trigger_delay;
			int latch_trigger_x;
			int latch_trigger_y;

			u16 param_x;
			u16 param_y;
			int param_mode;
		} f;


		int device_x, device_y, device_z;

		float guncon_sensitivity = 100;
		int guncon_threshold = 512;
		int guncon_deadzone = 0;
		/*
		int guncon_left = GUNCON_TRIGGER;
		int guncon_right = GUNCON_A;
		int guncon_middle = GUNCON_B;

		int guncon_aux1 = GUNCON_NONE;
		int guncon_aux2 = GUNCON_NONE;
		int guncon_wheelup = GUNCON_NONE;
		int guncon_wheeldown = GUNCON_NONE;*/
		uint8_t btn_modes[7]{};

		int guncon_keyboard_dpad = 0;
		int guncon_start_hotkey = 0;

		int guncon_reload = GUNCON_RELOAD_MANUAL;
		bool guncon_calibrate;
		int guncon_reload_auto;

		int guncon_cursor = GUNCON_SHOW_CURSOR;

		int guncon_lightgun_left = 1;
		int guncon_lightgun_top = 1;
		int guncon_lightgun_right = 65534;
		int guncon_lightgun_bottom = 65534;
		u8 keyboard_state[Q_KEY_CODE__MAX]{};
		bool abs_coords2window = false;

		struct
		{
			int model = GUNCON_MODEL_NAMCO;

			float scale_x = 97.625;
			float scale_y = 94.625;

			int width = 384;
			int height = 256;

			/*
			take __average__ of data for y-center

			- Point Blank + Time Crisis vs
			Resident Evil Survivor + Extreme Ghostbusters
			(tight bullseye variation)

			- Extreme Ghostbusters vs RES suggests x-center
			(tight bullseye variation)
			*/

			int screen_center_x = 274;
			int screen_center_y = 168;
		} preset;

	} Guncon2State;

	static void usb_hid_changed(HIDState* hs)
	{
		// no-op
	}

	static void guncon2_keyboard_event(HIDState* hs, InputEvent* evt)
	{
		if (hs->kind != HID_KEYBOARD)
			return;

		Guncon2State* s = CONTAINER_OF(hs, Guncon2State, hid_kbd);
		const InputKeyEvent& key = evt->u.key;
		if (key.key.u.qcode < Q_KEY_CODE__MAX)
			s->keyboard_state[key.key.u.qcode] = key.down;
	}

	static void guncon2_pointer_sync(HIDState* hs)
	{
		HIDPointerEvent *prev, *curr, *next;
		bool event_compression = false;

		if (hs->n == QUEUE_LENGTH - 1)
		{
			/*
			* Queue full.  We are losing information, but we at least
			* keep track of most recent button state.
			*/
			return;
		}

		prev = &hs->ptr.queue[(hs->head + hs->n - 1) & QUEUE_MASK];
		curr = &hs->ptr.queue[(hs->head + hs->n) & QUEUE_MASK];
		next = &hs->ptr.queue[(hs->head + hs->n + 1) & QUEUE_MASK];

		if (hs->n > 0)
		{
			/*
			* No button state change between previous and current event
			* (and previous wasn't seen by the guest yet), so there is
			* motion information only and we can combine the two event
			* into one.
			*/
			if (curr->buttons_state == prev->buttons_state)
			{
				event_compression = true;
			}
		}

		if (event_compression)
		{
			/* add current motion to previous, clear current */
			if (curr->kind == INPUT_EVENT_KIND_REL)
			{
				if (prev->kind == INPUT_EVENT_KIND_ABS) //TODO yes, no ,maybe?
				{
					prev->kind = INPUT_EVENT_KIND_REL;
					prev->xdx = 0;
					prev->ydy = 0;
				}
				prev->xdx += curr->xdx;
				curr->xdx = 0;
				prev->ydy += curr->ydy;
				curr->ydy = 0;
			}
			else if (curr->kind == INPUT_EVENT_KIND_ABS)
			{
				prev->xdx = curr->xdx;
				prev->ydy = curr->ydy;
			}
			prev->dz += curr->dz;
			curr->dz = 0;
		}
		else
		{
			/* prepate next (clear rel, copy abs + btns) */
			if (curr->kind == INPUT_EVENT_KIND_REL)
			{
				next->xdx = 0;
				next->ydy = 0;
			}
			else if (curr->kind == INPUT_EVENT_KIND_ABS)
			{
				next->xdx = curr->xdx;
				next->ydy = curr->ydy;
			}
			next->dz = 0;
			next->buttons_state = curr->buttons_state;
			/* make current guest visible, notify guest */
			hs->n++;
			hs->event(hs);
		}
	}

	static void gcon2_pointer_poll(Guncon2State* s)
	{
		int dx = 0, dy = 0, dz = 0;
		auto hs = &s->hid_mouse;

		hs->idle_pending = false;

		hid_pointer_activate(hs);

		/* When the buffer is empty, return the last event.  Relative
		movements will all be zero.  */
		const int index = (hs->n ? hs->head : hs->head - 1);
		HIDPointerEvent* e = &hs->ptr.queue[index & QUEUE_MASK];

		if (e->kind == INPUT_EVENT_KIND_REL)
		{
			dx = std::clamp(e->xdx, -127, 127);
			dy = std::clamp(e->ydy, -127, 127);
			e->xdx -= dx;
			e->ydy -= dy;
			s->f.absolute = false;
		}
		else if (e->kind == INPUT_EVENT_KIND_ABS)
		{
			dx = e->xdx;
			dy = e->ydy;
			s->f.absolute = true;
		}
		dz = std::clamp(e->dz, -127, 127);
		e->dz -= dz;

		if (hs->n /*&&
			!e->dz &&
			(hs->kind == HID_TABLET)*/
		)
		{
			/* that deals with this event */
			QUEUE_INCR(hs->head);
			hs->n--;
		}

		/* Appears we have to invert the wheel direction */
		dz = 0 - dz;

		if (dx || dy || dz || e->buttons_state)
			Console.WriteLn("mouse: %d %d %d %08x", dx, dy, dz, e->buttons_state);

		s->device_x = dx;
		s->device_y = dy;
		s->device_z = dz;
		s->f.buttons[0] = e->buttons_state & 0x01;
		s->f.buttons[1] = e->buttons_state & 0x02;
		s->f.buttons[2] = e->buttons_state & 0x04;
		s->f.buttons[3] = e->buttons_state & 0x08;
		s->f.buttons[4] = e->buttons_state & 0x10;
		//s->guncon_buttons[5] = e->buttons_state & 0x20
	}

	// TODO Update inputs about once per vblank (16.666ms)
	static void gcon2_update(Guncon2State* s)
	{
		int data_x, data_y, data_z;

		int gun_width, gun_height;
		int gun_right, gun_bottom;

		gcon2_pointer_poll(s);

		data_x = s->device_x;
		data_y = s->device_y;
		data_z = s->device_z;

		gun_width = 640;
		gun_height = 480;
		gun_right = 640;
		gun_bottom = 480;


		// relative mouse
		if (!s->f.absolute)
		{
			data_x = ((float)data_x * s->guncon_sensitivity) / 100;
			data_y = ((float)data_y * s->guncon_sensitivity) / 100;


			if (data_x > s->guncon_threshold)
				data_x = s->guncon_threshold;
			if (data_y > s->guncon_threshold)
				data_y = s->guncon_threshold;
			if (data_x < -s->guncon_threshold)
				data_x = -s->guncon_threshold;
			if (data_y < -s->guncon_threshold)
				data_y = -s->guncon_threshold;


			if (data_x >= -s->guncon_deadzone && data_x <= s->guncon_deadzone)
				data_x = 0;
			if (data_y >= -s->guncon_deadzone && data_y <= s->guncon_deadzone)
				data_y = 0;


			// relative motion
			s->f.guncon_analog_x += data_x;
			s->f.guncon_analog_y += data_y;


			// clip to 16-bit absolute window
			if (s->f.guncon_analog_x < 0)
				s->f.guncon_analog_x = 0;
			if (s->f.guncon_analog_x > gun_right)
				s->f.guncon_analog_x = gun_right;
			if (s->f.guncon_analog_y < 0)
				s->f.guncon_analog_y = 0;
			if (s->f.guncon_analog_y > gun_bottom)
				s->f.guncon_analog_y = gun_bottom;


			if (s->f.guncon_analog_x + data_x != s->f.guncon_analog_x || s->f.guncon_analog_y + data_y != s->f.guncon_analog_y)
				Console.WriteLn("(relative) %d %d || %d %d || param %d %d",
					s->f.guncon_analog_x, s->f.guncon_analog_y,
					data_x, data_y,
					(s16)s->f.param_x, (s16)s->f.param_y);

			// offscreen shots
			if (s->f.guncon_analog_x == 0 ||
				s->f.guncon_analog_y == 0 ||
				s->f.guncon_analog_x == gun_right ||
				s->f.guncon_analog_y == gun_bottom)
			{
				data_x = 0;
				data_y = 0;
			}
			else
			{
				// re-adjust center values
				data_x = s->f.guncon_analog_x - (gun_width / 2);
				data_y = s->f.guncon_analog_y - (gun_height / 2);


				// scale based on guncon window
				data_x = (data_x * (s->preset.width / 2)) / (gun_width / 2);
				data_y = (data_y * (s->preset.height / 2)) / (gun_height / 2);


				// aiming scaling
				data_x = (data_x * s->preset.scale_x) / 100;
				data_y = (data_y * s->preset.scale_y) / 100;


				// re-adjust to guncon center
				data_x += s->preset.screen_center_x;
				data_y += s->preset.screen_center_y;


				// special edge check (valid data)
				if (data_x < 1)
					data_x = 1;
				if (data_y < 1)
					data_y = 1;
			}
		}

		// 16-bit device absolute (x,y) -> local guncon coordinates
		// assumes (0, 0) is left-top, (65535, 65535) is right-bottom
		else if (s->f.absolute)
		{
			gun_width = s->guncon_lightgun_right - s->guncon_lightgun_left + 1;
			gun_height = s->guncon_lightgun_bottom - s->guncon_lightgun_top + 1;
			gun_right = s->guncon_lightgun_right;
			gun_bottom = s->guncon_lightgun_bottom;


			// absolute data (range)
			s->f.guncon_analog_x = data_x;
			s->f.guncon_analog_y = data_y;

#ifdef WIN32
			if (s->abs_coords2window)
			{
				RECT r{}, dr{};
				GetWindowRect(gsWnd, &r);
				GetWindowRect(GetDesktopWindow(), &dr); // FIXME really only works with window on primary screen
				constexpr float render_aspect = 640.f / 480.f; //FIXME hardcoded to 640x480
				const int rh = r.bottom - r.top;
				const int rw = r.right - r.left;
				const int dw = dr.right - dr.left;
				const int dh = dr.bottom - dr.top;

				if (render_aspect > static_cast<float>(rw) / rh)
				{
					const int aspect_height = rw / render_aspect;
					s->f.guncon_analog_x = ((dw * data_x / 65535) - r.left) * 65535 / rw;
					s->f.guncon_analog_y = ((dh * data_y / 65535) - r.top - (rh - aspect_height) / 2) * 65535 / aspect_height;
				}
				else
				{
					const int aspect_width = rh * render_aspect;
					s->f.guncon_analog_x = ((dw * data_x / 65535) - r.left - (rw - aspect_width) / 2) * 65535 / aspect_width;
					s->f.guncon_analog_y = ((dh * data_y / 65535) - r.top) * 65535 / rh;
				}
			}
#endif

			// offscreen data
			if (s->f.guncon_analog_x <= 0 ||
				s->f.guncon_analog_y <= 0 ||
				s->f.guncon_analog_x >= 65535 ||
				s->f.guncon_analog_y >= 65535)
			{
				data_x = 0;
				data_y = 0;
			}
			else
			{
				data_x = ((s->f.guncon_analog_x - s->guncon_lightgun_left) * s->preset.width) / gun_width;
				data_y = ((s->f.guncon_analog_y - s->guncon_lightgun_top) * s->preset.height) / gun_height;


				// re-adjust center values
				data_x = s->f.guncon_analog_x - (gun_width / 2);
				data_y = s->f.guncon_analog_y - (gun_height / 2);


				// scale based on guncon window
				data_x = (data_x * (s->preset.width / 2)) / (gun_width / 2);
				data_y = (data_y * (s->preset.height / 2)) / (gun_height / 2);


				// aiming scaling
				data_x = (data_x * s->preset.scale_x) / 100;
				data_y = (data_y * s->preset.scale_y) / 100;


				// re-adjust to guncon center
				data_x += s->preset.screen_center_x;
				data_y += s->preset.screen_center_y;


				// special edge check (valid data)
				if (data_x < 1)
					data_x = 1;
				if (data_y < 1)
					data_y = 1;
			}
		}


		// save for later
		s->f.current_analog_x = data_x;
		s->f.current_analog_y = data_y;
		s->f.current_analog_z = data_z;

		// draw real-time lightgun cursor (true device position)
		if (s->guncon_cursor == GUNCON_SHOW_CURSOR)
		{
			int screen_x, screen_y;


			if (!s->f.absolute)
			{
				screen_x = (s->f.guncon_analog_x * 512) / 640;
				screen_y = (s->f.guncon_analog_y * 256) / 480;
			}
			else
			{
				// offscreen shot detection
				if (s->f.guncon_analog_x == 65535 ||
					s->f.guncon_analog_y == 65535)
				{
					screen_x = 0;
					screen_y = 0;
				}
				else
				{
					screen_x = (s->f.guncon_analog_x * 512) / gun_width;
					screen_y = (s->f.guncon_analog_y * 256) / gun_height;
				}
			}

			// clip to GPU cursor coordinates
			if (screen_x < 0)
				screen_x = 0;
			if (screen_y < 0)
				screen_y = 0;
			if (screen_x > 511)
				screen_x = 511;
			if (screen_y > 255)
				screen_y = 255;

			//Console.WriteLn("(pos) %d %d || %d %d || param %d %d",
			//	s->f.guncon_analog_x, s->f.guncon_analog_y,
			//	data_x, data_y,
			//	(s16)s->f.param_x, (s16)s->f.param_y);
			//Console.WriteLn("screen: %dx%d", screen_x, screen_y);
			const auto sx = s->preset.scale_x / 100.f;
			const auto sy = s->preset.scale_y / 100.f;
			OSDCursorPos((1.f - sx) / 2 + screen_x / 512.f * sx, (1.f - sy) / 2 + screen_y / 256.f * sy, 512, 256);

		} // end cursor
	}

	static void copy_data(Guncon2State* s, uint8_t* buf) //, wheel_data_t data)
	{
		uint16_t* pad_out = reinterpret_cast<uint16_t*>(buf);
		int data_x, data_y, data_z, reload = 0;
		gcon2_update(s);

		// restore from cache
		data_x = s->f.current_analog_x;
		data_y = s->f.current_analog_y;
		data_z = s->f.current_analog_z;


		if (s->f.param_mode & PROGRESSIVE_SCAN)
		{
			// interlace mode
			data_x -= static_cast<s16>(s->f.param_x) / 2;
			data_y -= static_cast<s16>(s->f.param_y) / 2;
		}

		else
		{
			// progressive mode
			data_x -= static_cast<s16>(s->f.param_x);
			data_y -= static_cast<s16>(s->f.param_y);
		}


		// clip to guncon window
		if (data_x < 0)
			data_x = 0;
		if (data_y < 0)
			data_y = 0;

		// ===================================
		// ===================================
		// ===================================

		// buttons (default)
		pad_out[0] = 0xffff & ~PROGRESSIVE_SCAN;

		// usually tv scan flag (mode $0100 = interlace)
		pad_out[0] |= s->f.param_mode;

		// read mouse buttons
		for (int lcv = 0; lcv < 7; lcv++)
		{
			// check not pushed
			if (lcv < 5 && !s->f.buttons[lcv])
				continue;

			// assert wheel up
			else if (lcv == 5 && data_z <= 0)
				continue;

			// assert wheel down
			else if (lcv == 6 && data_z >= 0)
				continue;

			switch (s->btn_modes[lcv])
			{
				case GUNCON_RELOAD:
					pad_out[0] &= ~BUTTON_TRIGGER;

					data_x = 0;
					data_y = 0;

					reload = 1;
					break;

				case GUNCON_TRIGGER:
					pad_out[0] &= ~BUTTON_TRIGGER;

					// black screen flash detection
					if (s->f.latch_trigger_delay == 0)
					{
						s->f.latch_trigger_delay = LATCH_DELAY;
						s->f.latch_trigger_x = data_x;
						s->f.latch_trigger_y = data_y;
					}
					break;

				case GUNCON_A:
					pad_out[0] &= ~BUTTON_A;
					break;
				case GUNCON_B:
					pad_out[0] &= ~BUTTON_B;
					break;
				case GUNCON_C:
					pad_out[0] &= ~BUTTON_C;
					break;

				case GUNCON_START:
					pad_out[0] &= ~BUTTON_START;
					break;
				case GUNCON_SELECT:
					pad_out[0] &= ~BUTTON_SELECT;
					break;

				case GUNCON_DPAD_UP:
					pad_out[0] &= ~DPAD_UP;
					break;
				case GUNCON_DPAD_DOWN:
					pad_out[0] &= ~DPAD_DOWN;
					break;
				case GUNCON_DPAD_LEFT:
					pad_out[0] &= ~DPAD_LEFT;
					break;
				case GUNCON_DPAD_RIGHT:
					pad_out[0] &= ~DPAD_RIGHT;
					break;

				case GUNCON_DPAD_A_SELECT:
					pad_out[0] &= ~BUTTON_A;
					pad_out[0] &= ~BUTTON_SELECT;
					break;
				case GUNCON_DPAD_B_SELECT:
					pad_out[0] &= ~BUTTON_B;
					pad_out[0] &= ~BUTTON_SELECT;
					break;

				case GUNCON_DPAD_UP_SELECT:
					pad_out[0] &= ~DPAD_UP;
					pad_out[0] &= ~BUTTON_SELECT;
					break;
				case GUNCON_DPAD_DOWN_SELECT:
					pad_out[0] &= ~DPAD_DOWN;
					pad_out[0] &= ~BUTTON_SELECT;
					break;
				case GUNCON_DPAD_LEFT_SELECT:
					pad_out[0] &= ~DPAD_LEFT;
					pad_out[0] &= ~BUTTON_SELECT;
					break;
				case GUNCON_DPAD_RIGHT_SELECT:
					pad_out[0] &= ~DPAD_RIGHT;
					pad_out[0] &= ~BUTTON_SELECT;
					break;
			}
		}

		// read keyboard dpad
		if (s->guncon_keyboard_dpad)
		{
			if (s->keyboard_state[Q_KEY_CODE_W])
				pad_out[0] &= ~DPAD_UP;
			if (s->keyboard_state[Q_KEY_CODE_A])
				pad_out[0] &= ~DPAD_LEFT;
			if (s->keyboard_state[Q_KEY_CODE_S])
				pad_out[0] &= ~DPAD_DOWN;
			if (s->keyboard_state[Q_KEY_CODE_D])
				pad_out[0] &= ~DPAD_RIGHT;

			if (s->keyboard_state[Q_KEY_CODE_Q])
				pad_out[0] &= ~BUTTON_START;
			if (s->keyboard_state[Q_KEY_CODE_E])
				pad_out[0] &= ~BUTTON_SELECT;
			if (s->keyboard_state[Q_KEY_CODE_F])
				pad_out[0] &= ~BUTTON_C;
		}

		// START hack = A + B + TRIGGER
		if (s->guncon_start_hotkey)
		{
			if ((~pad_out[0] & BUTTON_A) &&
				(~pad_out[0] & BUTTON_B) &&
				(~pad_out[0] & BUTTON_TRIGGER))
			{
				pad_out[0] &= ~BUTTON_START;
			}
		}

		// ==================================
		// ==================================
		// ==================================

		// offscreen reload - semi-automatic trigger
		if (s->guncon_reload == GUNCON_RELOAD_SEMI)
		{
			if (s->guncon_reload_auto == 0)
			{
				if (data_x < 0 || data_y < 0)
					s->guncon_reload_auto = 1;
			}
			else
			{
				s->guncon_reload_auto++;


				if (s->guncon_reload_auto < 1 + 3)
				{
					pad_out[0] |= BUTTON_TRIGGER;
					reload = 0;
				}

				else if (s->guncon_reload_auto < 1 + 6)
				{
					pad_out[0] &= ~BUTTON_TRIGGER;
					reload = 1;
				}

				// wait time
				else if (s->guncon_reload_auto > 30)
				{
					s->guncon_reload_auto = 0;

					reload = 0;
				}
			}
		}

		// ==================================
		// ==================================
		// ==================================

		// offscreen reload - manual trigger
		if (data_x == 0 || data_y == 0)
		{
			// trigger pulled
			if (~pad_out[0] & BUTTON_TRIGGER)
				reload = 1;
		}

		// ==================================
		// ==================================
		// ==================================

		// analog x-y
		pad_out[1] = (s16)data_x;
		pad_out[2] = (s16)data_y;


		// mouse only!!
		//
		// black screen flash detection (calibration hack)
		if (s->guncon_calibrate)
		{
			if (s->f.latch_trigger_delay > 0)
			{
				// still force trigger down - calibration timing
				pad_out[0] &= ~BUTTON_TRIGGER;


				// check screen flash
				//MessageBox(0,0,0,0);


				// use trigger latch data - calibration timing
				pad_out[1] = (s16)s->f.latch_trigger_x;
				pad_out[2] = (s16)s->f.latch_trigger_y;


				s->f.latch_trigger_delay--;
				if (!s->f.latch_trigger_delay)
				{
					// black screen = no data
					pad_out[1] = 0;
					pad_out[2] = 0;
				}
			}
		} // end calibration hack


		// special handling
		if (reload == 1)
		{
			pad_out[1] = 0;
			pad_out[2] = 0;
		}
	}

	static void gcon2_handle_data(USBDevice* dev, USBPacket* p)
	{
		Guncon2State* s = (Guncon2State*)dev;
		uint8_t data[8];

		uint8_t devep = p->ep->nr;

		switch (p->pid)
		{
			case USB_TOKEN_IN:
				if (devep == 1)
				{
					// 					s->pad->TokenIn(data, p->iov.size);
					copy_data(s, data); //, s->pad->GetData());
					usb_packet_copy(p, data, MIN(p->iov.size, 6));
				}
				else
				{
					goto fail;
				}
				break;
			case USB_TOKEN_OUT:
				//usb_packet_copy(p, data, MIN(p->iov.size, sizeof(data)));
				//ret = s->pad->TokenOut(data, p->iov.size);
				p->status = USB_RET_SUCCESS;
				break;
			default:
			fail:
				p->status = USB_RET_STALL;
				break;
		}
	}

	static void gcon2_handle_reset(USBDevice* dev)
	{
		/* XXX: do it */
		Guncon2State* s = (Guncon2State*)dev;
		// 		s->pad->Reset();
		hid_reset(&s->hid_mouse);
		return;
	}

	static void gcon2_handle_control(USBDevice* dev, USBPacket* p, int request, int value,
		int index, int length, uint8_t* data)
	{
		Guncon2State* s = reinterpret_cast<Guncon2State*>(dev);
		int ret = 0;

		switch (request)
		{
			case ClassInterfaceRequest | GUNCON2_SET_PARAM:
				p->status = USB_RET_SUCCESS;
				break;
			case ClassInterfaceOutRequest | GUNCON2_SET_PARAM:
				// expect 6 bytes - y / x / mode
				s->f.param_x = data[0] | data[1] << 8;
				s->f.param_y = data[2] | data[3] << 8;
				s->f.param_mode = data[4] | data[5] << 8;
				Console.WriteLn("GUNCON2 set param: x: %d, y: %d, mode: %d",
					s->f.param_x,
					s->f.param_y,
					s->f.param_mode);
				break;
			default:
				ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
				if (ret >= 0)
				{
					return;
				}
				p->status = USB_RET_STALL;
				break;
		}
	}

	static void gcon2_handle_destroy(USBDevice* dev)
	{
		Guncon2State* s = (Guncon2State*)dev;
		delete s;
	}

	static int gcon2_open(USBDevice* dev)
	{
		Guncon2State* s = (Guncon2State*)dev;
		if (s)
		{
			if (s->usbhid_ms)
				s->usbhid_ms->Open();
			if (s->usbhid_kbd)
				s->usbhid_kbd->Open();
			return 0;
			// 			return s->pad->Open();
		}
		return 1;
	}

	static void gcon2_close(USBDevice* dev)
	{
		Guncon2State* s = (Guncon2State*)dev;
		if (s)
		{
			// 			s->pad->Close();
			if (s->usbhid_ms)
				s->usbhid_ms->Close();
			if (s->usbhid_kbd)
				s->usbhid_kbd->Close();
		}
	}

	USBDevice* Guncon2Device::CreateDevice(int port)
	{
		// Find better place
		const_cast<PadConfig&>(Config).Load(port);
		auto presets = GetGuncon2Presets(port);
		std::string varApi;
		Guncon2State* s = new Guncon2State();
		const auto& cfg = Config.Port[port].Guncon2;

		bool found = false;
		for (const auto& p : presets)
		{
			if (cfg.Preset == p.id)
			{
				s->preset.scale_x = p.scale_x;
				s->preset.scale_y = p.scale_y;

				s->preset.width = p.width;
				s->preset.height = p.height;
				s->preset.screen_center_x = p.center_x;
				s->preset.screen_center_y = p.center_y;
				found = true;
				break;
			}
		}

		if (!found)
		{
			s->preset.scale_x = cfg.Aiming_scale_X.ToFloat();
			s->preset.scale_y = cfg.Aiming_scale_Y.ToFloat();
		}
		s->guncon_sensitivity = cfg.Sensitivity.ToFloat();
		s->guncon_threshold = cfg.Threshold;
		s->guncon_deadzone = cfg.Deadzone;
		s->btn_modes[0] = cfg.Left;
		s->btn_modes[1] = cfg.Right;
		s->btn_modes[2] = cfg.Middle;

		s->btn_modes[3] = cfg.Aux_1;
		s->btn_modes[4] = cfg.Aux_2;
		s->btn_modes[5] = cfg.Wheel_up;
		s->btn_modes[6] = cfg.Wheel_dn;

		s->guncon_keyboard_dpad = cfg.Keyboard_Dpad;
		s->guncon_start_hotkey = cfg.Start_hotkey;

		s->guncon_reload = cfg.Reload;
		s->guncon_calibrate = cfg.Calibration;
		s->abs_coords2window = cfg.Abs2Window;

		s->guncon_cursor = cfg.Cursor;

		s->guncon_lightgun_left = cfg.Lightgun_left;
		s->guncon_lightgun_top = cfg.Lightgun_top;
		s->guncon_lightgun_right = cfg.Lightgun_right;
		s->guncon_lightgun_bottom = cfg.Lightgun_bottom;

#ifdef _WIN32
		std::wstring tmp;
		LoadSetting(nullptr, port, TypeName(), N_DEVICE_API, tmp);
		varApi = wstr_to_str(tmp);
#else
		LoadSetting(nullptr, port, TypeName(), N_DEVICE_API, varApi);
#endif
		auto hid_proxy = usb_hid::RegisterUsbHID::instance().Proxy(varApi);
		if (!hid_proxy)
		{
			Console.WriteLn("Invalid HID API: %s \n", varApi.c_str());
		}
		else
		{
			auto ms_type = std::string(TypeName()) + "_ms";
			auto kbd_type = std::string(TypeName()) + "_kbd";
			s->usbhid_ms = hid_proxy->CreateObject(port, ms_type.c_str());
			s->usbhid_kbd = hid_proxy->CreateObject(port, kbd_type.c_str());
		}

		//if (!s->usbhid)
		//	return nullptr;

		s->desc.full = &s->desc_dev;
		s->desc.str = desc_strings;

		if (usb_desc_parse_dev(dev_descriptor, sizeof(dev_descriptor), s->desc, s->desc_dev) < 0)
			goto fail;
		if (usb_desc_parse_config(config_descriptor, sizeof(config_descriptor), s->desc_dev) < 0)
			goto fail;

		// 		s->pad = pad;
		s->dev.speed = USB_SPEED_FULL;
		s->dev.klass.handle_attach = usb_desc_attach;
		s->dev.klass.handle_reset = gcon2_handle_reset;
		s->dev.klass.handle_control = gcon2_handle_control;
		s->dev.klass.handle_data = gcon2_handle_data;
		s->dev.klass.unrealize = gcon2_handle_destroy;
		s->dev.klass.open = gcon2_open;
		s->dev.klass.close = gcon2_close;
		s->dev.klass.usb_desc = &s->desc;
		s->port = port;

		usb_desc_init(&s->dev);
		usb_ep_init(&s->dev);

		s->intr_ms = usb_ep_get(&s->dev, USB_TOKEN_IN, 1);
		hid_init(&s->hid_mouse, HID_MOUSE, usb_hid_changed);
		if (s->usbhid_ms)
		{
			s->hid_mouse.ptr.eh_sync = guncon2_pointer_sync;
			s->usbhid_ms->SetHIDState(&s->hid_mouse);
		}

		if (s->usbhid_kbd)
		{
			s->hid_kbd.kind = HID_KEYBOARD;
			s->hid_kbd.kbd.eh_entry = guncon2_keyboard_event;
			s->usbhid_kbd->SetHIDState(&s->hid_kbd);
		}

		gcon2_handle_reset((USBDevice*)s);

		return (USBDevice*)s;

	fail:
		gcon2_handle_destroy((USBDevice*)s);
		return nullptr;
	}

	int Guncon2Device::Configure(int port, const std::string& api, void* data)
	{
		//auto proxy = RegisterPad::instance().Proxy(api);
		//if (proxy)
		//	return proxy->Configure(port, TypeName(), data);
		Dialog GC2SettingsDialog(port, api);
		if (GC2SettingsDialog.ShowModal() == wxID_OK)
			return RESULT_OK;
		return RESULT_CANCELED;
	}

	int Guncon2Device::Freeze(FreezeAction mode, USBDevice* dev, void* data)
	{
		Guncon2State* s = (Guncon2State*)dev;
		switch (mode)
		{
			case FreezeAction::Load:
				if (!s)
					return -1;
				s->f = *(Guncon2State::freeze*)data;
				return sizeof(Guncon2State::freeze);
			case FreezeAction::Save:
				if (!s)
					return -1;
				return sizeof(Guncon2State::freeze);
			case FreezeAction::Size:
				return sizeof(Guncon2State::freeze);
			default:
				break;
		}
		return 0;
	}

} // namespace usb_pad

/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021 PCSX2 Dev Team
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

#include "GSMTLShaderCommon.h"

using namespace metal;


// Weave shader
fragment float4 ps_interlace0(ConvertShaderData data [[stage_in]], ConvertPSRes res,
	constant GSMTLInterlacePSUniform& uniform [[buffer(GSMTLBufferIndexUniforms)]])
{
	const int idx   = int(uniform.ZrH.x); // buffer index passed from CPU
	const int field = idx & 1;            // current field
	const int vpos  = int(data.p.y);      // vertical position of destination texture

	if ((vpos & 1) == field)
		return res.sample(data.t);
	else
		discard_fragment();

	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}


// Bob shader
fragment float4 ps_interlace1(ConvertShaderData data [[stage_in]], ConvertPSRes res)
{
	return res.sample(data.t);
}


// Blend shader
fragment float4 ps_interlace2(ConvertShaderData data [[stage_in]], ConvertPSRes res,
	constant GSMTLInterlacePSUniform& uniform [[buffer(GSMTLBufferIndexUniforms)]])
{
	float2 vstep = float2(0.0f, uniform.ZrH.y);
	float4 c0 = res.sample(data.t - vstep);
	float4 c1 = res.sample(data.t);
	float4 c2 = res.sample(data.t + vstep);
	return (c0 + c1 * 2.f + c2) / 4.f;
}


// MAD shader - buffering
fragment float4 ps_interlace3(ConvertShaderData data [[stage_in]], ConvertPSRes res,
	constant GSMTLInterlacePSUniform& uniform [[buffer(GSMTLBufferIndexUniforms)]])
{
	// We take half the lines from the current frame and stores them in the MAD frame buffer.
	// the MAD frame buffer is split in 2 consecutive banks of 2 fields each, the fields in each bank
	// are interleaved (top field at even lines and bottom field at odd lines). 
	// When the source texture has an odd vres, the first line of bank 1 would be an odd index
	// causing the wrong lines to be discarded, so a vertical offset (lofs) is added to the vertical
	// position of the destination texture to force the proper field alignment

	const int    idx      = int(uniform.ZrH.x);                       // buffer index passed from CPU
	const int    bank     = idx >> 1;                                 // current bank
	const int    field    = idx & 1;                                  // current field
	const int    vres     = int(uniform.ZrH.z) >> 1;                  // vertical resolution of source texture
	const int    lofs     = ((((vres + 1) >> 1) << 1) - vres) & bank; // line alignment offset for bank 1
	const int    vpos     = int(data.p.y) + lofs;                     // vertical position of destination texture
	const float2 bofs     = float2(0.0f, 0.5f * bank);                // vertical offset of the current bank relative to source texture size
	const float2 vscale   = float2(1.0f, 2.0f);                       // scaling factor from source to destination texture
	const float2 optr     = data.t - bofs;                            // used to check if the current destination line is within the current bank
	const float2 iptr     = optr * vscale;                            // pointer to the current pixel in the source texture

	// if the index of current destination line belongs to the current fiels we update it, otherwise
	// we leave the old line in the destination buffer
	if ((optr.y >= 0.0f) && (optr.y < 0.5f) && ((vpos & 1) == field))
		return res.sample(iptr);
	else
		discard_fragment();

	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}


// MAD shader - reconstruction
fragment float4 ps_interlace4(ConvertShaderData data [[stage_in]], ConvertPSRes res,
	constant GSMTLInterlacePSUniform& uniform [[buffer(GSMTLBufferIndexUniforms)]])
{
	const int    idx         = int(uniform.ZrH.x);                   // buffer index passed from CPU
	const int    field       = idx & 1;                              // current field
	const int    vpos        = int(data.p.y);                        // vertical position of destination texture
	const float  sensitivity = uniform.ZrH.w;                        // passed from CPU, higher values mean more likely to use weave
	const float3 motion_thr  = float3(1.0, 1.0, 1.0) * sensitivity;  //
	const float2 bofs        = float2(0.0f, 0.5f);                   // position of the bank 1 relative to source texture size
	const float2 vscale      = float2(1.0f, 0.5f);                   // scaling factor from source to destination texture
	const float2 lofs        = float2(0.0f, uniform.ZrH.y) * vscale; // distance between two adjacent lines relative to source texture size
	const float2 iptr        = data.t * vscale;                      // pointer to the current pixel in the source texture


	float2 p_t0; // pointer to current pixel (missing or not) from most recent frame
	float2 p_t1; // pointer to current pixel (missing or not) from one frame back
	float2 p_t2; // pointer to current pixel (missing or not) from two frames back
	float2 p_t3; // pointer to current pixel (missing or not) from three frames back

	switch (idx)
	{
		case 1:
			p_t0 = iptr;
			p_t1 = iptr;
			p_t2 = iptr + bofs;
			p_t3 = iptr + bofs;
			break;
		case 2:
			p_t0 = iptr + bofs;
			p_t1 = iptr;
			p_t2 = iptr;
			p_t3 = iptr + bofs;
			break;
		case 3:
			p_t0 = iptr + bofs;
			p_t1 = iptr + bofs;
			p_t2 = iptr;
			p_t3 = iptr;
			break;
		default:
			p_t0 = iptr;
			p_t1 = iptr + bofs;
			p_t2 = iptr + bofs;
			p_t3 = iptr;
			break;
	}

	// calculating motion, only relevant for missing lines where the "center line" is pointed by p_t1

	float4 hn = res.sample(p_t0 - lofs); // new high pixel
	float4 cn = res.sample(p_t1);        // new center pixel
	float4 ln = res.sample(p_t0 + lofs); // new low pixel

	float4 ho = res.sample(p_t2 - lofs); // old high pixel
	float4 co = res.sample(p_t3);        // old center pixel
	float4 lo = res.sample(p_t2 + lofs); // old low pixel

	float3 mh = hn.rgb - ho.rgb;
	float3 mc = cn.rgb - co.rgb;
	float3 ml = ln.rgb - lo.rgb;

	mh = max(mh, -mh) - motion_thr;
	mc = max(mc, -mc) - motion_thr;
	ml = max(ml, -ml) - motion_thr;

	#if 1 // use this code to evaluate each color motion separately
		float mh_max = max(max(mh.x, mh.y), mh.z);
		float mc_max = max(max(mc.x, mc.y), mc.z);
		float ml_max = max(max(ml.x, ml.y), ml.z);
	#else // use this code to evaluate average color motion
		float mh_max = mh.x + mh.y + mh.z;
		float mc_max = mc.x + mc.y + mc.z;
		float ml_max = ml.x + ml.y + ml.z;
	#endif

	// selecting deinterlacing output

	if ((vpos & 1) == field)
	{
		// output coordinate present on current field
		return res.sample(p_t0);
	}
	else if ((iptr.y > 0.5f - lofs.y) || (iptr.y < 0.0 + lofs.y))
	{
		// top and bottom lines are always weaved
		return cn;
	}
	else
	{
		// missing line needs to be reconstructed
		if (((mh_max > 0.0f) || (ml_max > 0.0f)) || (mc_max > 0.0f))
			// high motion -> interpolate pixels above and below
			return (hn + ln) / 2.0f;
		else
			// low motion -> weave
			return cn;
	}

	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

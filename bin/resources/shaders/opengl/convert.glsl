//#version 420 // Keep it for editor detection


#ifdef VERTEX_SHADER

layout(location = 0) in vec2 POSITION;
layout(location = 1) in vec2 TEXCOORD0;
layout(location = 7) in vec4 COLOR;

// FIXME set the interpolation (don't know what dx do)
// flat means that there is no interpolation. The value given to the fragment shader is based on the provoking vertex conventions.
//
// noperspective means that there will be linear interpolation in window-space. This is usually not what you want, but it can have its uses.
//
// smooth, the default, means to do perspective-correct interpolation.
//
// The centroid qualifier only matters when multisampling. If this qualifier is not present, then the value is interpolated to the pixel's center, anywhere in the pixel, or to one of the pixel's samples. This sample may lie outside of the actual primitive being rendered, since a primitive can cover only part of a pixel's area. The centroid qualifier is used to prevent this; the interpolation point must fall within both the pixel's area and the primitive's area.
out vec4 PSin_p;
out vec2 PSin_t;
out vec4 PSin_c;

void vs_main()
{
    PSin_p = vec4(POSITION, 0.5f, 1.0f);
    PSin_t = TEXCOORD0;
    PSin_c = COLOR;
    gl_Position = vec4(POSITION, 0.5f, 1.0f); // NOTE I don't know if it is possible to merge POSITION_OUT and gl_Position
}

#endif

#ifdef FRAGMENT_SHADER

in vec4 PSin_p;
in vec2 PSin_t;
in vec4 PSin_c;

// Give a different name so I remember there is a special case!
#if defined(ps_convert_rgba8_16bits) || defined(ps_convert_float32_32bits)
layout(location = 0) out uint SV_Target1;
#else
layout(location = 0) out vec4 SV_Target0;
#endif

vec4 sample_c()
{
    return texture(TextureSampler, PSin_t);
}

#ifdef ps_copy
void ps_copy()
{
    SV_Target0 = sample_c();
}
#endif

#ifdef ps_depth_copy
void ps_depth_copy()
{
  gl_FragDepth = sample_c().r;
}
#endif

#ifdef ps_convert_rgba8_16bits
// Need to be careful with precision here, it can break games like Spider-Man 3 and Dogs Life
void ps_convert_rgba8_16bits()
{
    highp uvec4 i = uvec4(sample_c() * vec4(255.5f, 255.5f, 255.5f, 255.5f));

    SV_Target1 = ((i.x & 0x00F8u) >> 3) | ((i.y & 0x00F8u) << 2) | ((i.z & 0x00f8u) << 7) | ((i.w & 0x80u) << 8);
}
#endif

#ifdef ps_convert_float32_32bits
void ps_convert_float32_32bits()
{
    // Convert a GL_FLOAT32 depth texture into a 32 bits UINT texture
    SV_Target1 = uint(exp2(32.0f) * sample_c().r);
}
#endif

#ifdef ps_convert_float32_rgba8
void ps_convert_float32_rgba8()
{
    // Convert a GL_FLOAT32 depth texture into a RGBA color texture
    uint d = uint(sample_c().r * exp2(32.0f));
    SV_Target0 = vec4(uvec4((d & 0xFFu), ((d >> 8) & 0xFFu), ((d >> 16) & 0xFFu), (d >> 24))) / vec4(255.0);
}
#endif

#ifdef ps_convert_float16_rgb5a1
void ps_convert_float16_rgb5a1()
{
    // Convert a GL_FLOAT32 (only 16 lsb) depth into a RGB5A1 color texture
    uint d = uint(sample_c().r * exp2(32.0f));
    SV_Target0 = vec4(uvec4((d & 0x1Fu), ((d >> 5) & 0x1Fu), ((d >> 10) & 0x1Fu), (d >> 15) & 0x01u)) / vec4(32.0f, 32.0f, 32.0f, 1.0f);
}
#endif

#ifdef ps_convert_rgba8_float32
void ps_convert_rgba8_float32()
{
    // Convert a RRGBA texture into a float depth texture
    uvec4 c = uvec4(sample_c() * vec4(255.0f) + vec4(0.5f));
    gl_FragDepth = float(c.r | (c.g << 8) | (c.b << 16) | (c.a << 24)) * exp2(-32.0f);
}
#endif

#ifdef ps_convert_rgba8_float24
void ps_convert_rgba8_float24()
{
    // Same as above but without the alpha channel (24 bits Z)

    // Convert a RRGBA texture into a float depth texture
    uvec3 c = uvec3(sample_c().rgb * vec3(255.0f) + vec3(0.5f));
    gl_FragDepth = float(c.r | (c.g << 8) | (c.b << 16)) * exp2(-32.0f);
}
#endif

#ifdef ps_convert_rgba8_float16
void ps_convert_rgba8_float16()
{
    // Same as above but without the A/B channels (16 bits Z)

    // Convert a RRGBA texture into a float depth texture
    uvec2 c = uvec2(sample_c().rg * vec2(255.0f) + vec2(0.5f));
    gl_FragDepth = float(c.r | (c.g << 8)) * exp2(-32.0f);
}
#endif

#ifdef ps_convert_rgb5a1_float16
void ps_convert_rgb5a1_float16()
{
    // Convert a RGB5A1 (saved as RGBA8) color to a 16 bit Z
    uvec4 c = uvec4(sample_c() * vec4(255.0f) + vec4(0.5f));
    gl_FragDepth = float(((c.r & 0xF8u) >> 3) | ((c.g & 0xF8u) << 2) | ((c.b & 0xF8u) << 7) | ((c.a & 0x80u) << 8)) * exp2(-32.0f);
}
#endif

#ifdef ps_convert_rgba_8i
void ps_convert_rgba_8i()
{

    // Potential speed optimization. There is a high probability that
    // game only want to extract a single channel (blue). It will allow
    // to remove most of the conditional operation and yield a +2/3 fps
    // boost on MGS3
    //
    // Hypothesis wrong in Prince of Persia ... Seriously WTF !
    //#define ONLY_BLUE;

    // Convert a RGBA texture into a 8 bits packed texture
    // Input column: 8x2 RGBA pixels
    // 0: 8 RGBA
    // 1: 8 RGBA
    // Output column: 16x4 Index pixels
    // 0: 8 R | 8 B
    // 1: 8 R | 8 B
    // 2: 8 G | 8 A
    // 3: 8 G | 8 A
    float c;

    uvec2 sel = uvec2(gl_FragCoord.xy) % uvec2(16u, 16u);
    ivec2 tb  = ((ivec2(gl_FragCoord.xy) & ~ivec2(15, 3)) >> 1);

    int ty   = tb.y | (int(gl_FragCoord.y) & 1);
    int txN  = tb.x | (int(gl_FragCoord.x) & 7);
    int txH  = tb.x | ((int(gl_FragCoord.x) + 4) & 7);

    txN *= PS_SCALE_FACTOR;
    txH *= PS_SCALE_FACTOR;
    ty  *= PS_SCALE_FACTOR;

    // TODO investigate texture gather
    vec4 cN = texelFetch(TextureSampler, ivec2(txN, ty), 0);
    vec4 cH = texelFetch(TextureSampler, ivec2(txH, ty), 0);


    if ((sel.y & 4u) == 0u) {
        // Column 0 and 2
#ifdef ONLY_BLUE
        c = cN.b;
#else
        if ((sel.y & 3u) < 2u) {
            // first 2 lines of the col
            if (sel.x < 8u)
                c = cN.r;
            else
                c = cN.b;
        } else {
            if (sel.x < 8u)
                c = cH.g;
            else
                c = cH.a;
        }
#endif
    } else {
#ifdef ONLY_BLUE
        c = cH.b;
#else
        // Column 1 and 3
        if ((sel.y & 3u) < 2u) {
            // first 2 lines of the col
            if (sel.x < 8u)
                c = cH.r;
            else
                c = cH.b;
        } else {
            if (sel.x < 8u)
                c = cN.g;
            else
                c = cN.a;
        }
#endif
    }


    SV_Target0 = vec4(c);
}
#endif

#ifdef ps_filter_transparency
void ps_filter_transparency()
{
    vec4 c = sample_c();

    c.a = dot(c.rgb, vec3(0.299, 0.587, 0.114));

    SV_Target0 = c;
}
#endif

// Used for DATE (stencil)
// DATM == 1
#ifdef ps_datm1
void ps_datm1()
{
    if(sample_c().a < (127.5f / 255.0f)) // >= 0x80 pass
        discard;
}
#endif

// Used for DATE (stencil)
// DATM == 0
#ifdef ps_datm0
void ps_datm0()
{
    if((127.5f / 255.0f) < sample_c().a) // < 0x80 pass (== 0x80 should not pass)
        discard;
}
#endif

#ifdef ps_mod256
void ps_mod256()
{
    SV_Target0 = mod(round(sample_c() * 255.0f), 256.0f) / 255.0f;
}
#endif

#ifdef ps_yuv
uniform ivec2 EMOD;

void ps_yuv()
{
    vec4 i = sample_c();
    vec4 o;

    mat3 rgb2yuv; // Value from GS manual
    rgb2yuv[0] = vec3(0.587, -0.311, -0.419);
    rgb2yuv[1] = vec3(0.114, 0.500, -0.081);
    rgb2yuv[2] = vec3(0.299, -0.169, 0.500);

    vec3 yuv = rgb2yuv * i.gbr;

    float Y = float(0xDB)/255.0f * yuv.x + float(0x10)/255.0f;
    float Cr = float(0xE0)/255.0f * yuv.y + float(0x80)/255.0f;
    float Cb = float(0xE0)/255.0f * yuv.z + float(0x80)/255.0f;

    switch(EMOD.x) {
        case 0:
            o.a = i.a;
            break;
        case 1:
            o.a = Y;
            break;
        case 2:
            o.a = Y/2.0f;
            break;
        case 3:
            o.a = 0.0f;
            break;
    }

    switch(EMOD.y) {
        case 0:
            o.rgb = i.rgb;
            break;
        case 1:
            o.rgb = vec3(Y);
            break;
        case 2:
            o.rgb = vec3(Y, Cb, Cr);
            break;
        case 3:
            o.rgb = vec3(i.a);
            break;
    }

    SV_Target0 = o;
}
#endif

#endif

#pragma once

#include "../file.h"
#include "../storage/pixel_buffer.h"
#include "gtype.h"

#if OS == WINDOWS
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11SamplerState;
struct ID3D11RenderTargetView;
#endif

LSTD_BEGIN_NAMESPACE

enum class texture_wrap { None = 0, Repeat, Clamp, Mirrored_Repeat, Clamp_To_Border };
enum class texture_filter { Linear, Nearest };

struct graphics;

struct texture_2D {
#if OS == WINDOWS
    struct {
        ID3D11Texture2D *Texture = null;
        ID3D11ShaderResourceView *ResourceView = null;
        ID3D11SamplerState *SamplerState = null;
        ID3D11RenderTargetView *RenderTargetView = null;
    } D3D{};
#endif

    struct impl {
        void (*Init)(texture_2D *t);
        void (*SetData)(texture_2D *t, pixel_buffer data) = null;

        void (*Bind)(texture_2D *t) = null;
        void (*Unbind)(texture_2D *t) = null;
        void (*Release)(texture_2D *t) = null;
    } Impl{};

    graphics *Graphics;

    string Name;  // @TODO: _texture_ should be an asset, move this field when we have a catalog system

    s32 Width = 0;
    s32 Height = 0;

    u32 BoundSlot = (u32) -1;

    texture_wrap Wrap;
    texture_filter Filter;
    bool RenderTarget = false;  // When true, the texture can be used as a framebuffer

    texture_2D() = default;
    ~texture_2D() { release(); }

    void init(graphics *g, string name, s32 width, s32 height, texture_filter filter = texture_filter::Linear,
              texture_wrap wrap = texture_wrap::Clamp);
    void init_as_render_target(graphics *g, string name, s32 width, s32 height,
                               texture_filter filter = texture_filter::Linear, texture_wrap wrap = texture_wrap::Clamp);

    void set_data(pixel_buffer data) { Impl.SetData(this, data); }

    void bind(u32 slot) {
        BoundSlot = slot;
        Impl.Bind(this);
    }
    void unbind() {
        Impl.Unbind(this);
        BoundSlot = (u32) -1;
    }

    void release() {
        if (Impl.Release) Impl.Release(this);
    }
};

LSTD_END_NAMESPACE
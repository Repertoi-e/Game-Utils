#include "imgui_renderer.h"

#include "../../dx_graphics.h"

#include <d3d11.h>  // @TODO: Provide an API for clipping

LSTD_BEGIN_NAMESPACE

void imgui_renderer::init(graphics *g) {
    assert(!Graphics);
    Graphics = g;

    ImGuiIO &io = ImGui::GetIO();
    io.BackendRendererName = "lstd";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    ImGuiPlatformIO &platformIO = ImGui::GetPlatformIO();
    platformIO.Renderer_RenderWindow = [](auto *viewport, void *context) {
        auto *renderer = (imgui_renderer *) context;
        if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear)) {
            renderer->Graphics->clear_color(vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        renderer->draw(viewport->DrawData);
    };

    g->create_shader(&Shader, "UI Shader", file::path("data/UI.hlsl"));
    Shader.bind();

    g->create_buffer(&UB, buffer::type::SHADER_UNIFORM_BUFFER, buffer::usage::DYNAMIC, sizeof(mat4));

    s32 width, height;
    u8 *pixels = null;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    g->create_texture_2D(&FontTexture, "UI Font Texture", width, height);
    FontTexture.set_data(pixels);
    size_t VBSize = 5000, IBSize = 10000;
}

void imgui_renderer::draw(ImDrawData *drawData) {
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f) return;

    if (VBSize < drawData->TotalVtxCount) {
        VB.release();

        VBSize = drawData->TotalVtxCount + 5000;
        Graphics->create_buffer(&VB, buffer::type::VERTEX_BUFFER, buffer::usage::DYNAMIC, VBSize * sizeof(ImDrawVert));

        Shader.bind();
        buffer_layout layout;
        layout.add("POSITION", gtype::F32_2);
        layout.add("TEXCOORD", gtype::F32_2);
        layout.add("COLOR", gtype::U32, 1, true);
        VB.set_input_layout(layout);
    }

    if (IBSize < drawData->TotalIdxCount) {
        IBSize = drawData->TotalIdxCount + 10000;
        Graphics->create_buffer(&IB, buffer::type::INDEX_BUFFER, buffer::usage::DYNAMIC, IBSize * sizeof(u32));
    }

    auto *vb = (ImDrawVert *) VB.map(buffer::map_access::WRITE_DISCARD_PREVIOUS);
    auto *ib = (u32 *) IB.map(buffer::map_access::WRITE_DISCARD_PREVIOUS);

    For_as(it_index, range(drawData->CmdListsCount)) {
        auto *it = drawData->CmdLists[it_index];
        copy_memory(vb, it->VtxBuffer.Data, it->VtxBuffer.Size * sizeof(ImDrawVert));
        copy_memory(ib, it->IdxBuffer.Data, it->IdxBuffer.Size * sizeof(u32));
        vb += it->VtxBuffer.Size;
        ib += it->IdxBuffer.Size;
    }
    VB.unmap();
    IB.unmap();

    auto *ub = UB.map(buffer::map_access::WRITE_DISCARD_PREVIOUS);
    f32 L = drawData->DisplayPos.x;
    f32 R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    f32 T = drawData->DisplayPos.y;
    f32 B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    f32 mvp[4][4] = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        {0.0f, 0.0f, 0.5f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f},
    };
    copy_memory(ub, &mvp, sizeof(mvp));
    UB.unmap();

    u32 clipRectCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_RECT oldScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ((dx_graphics *) Graphics)->D3DDeviceContext->RSGetScissorRects(&clipRectCount, oldScissorRects);

    set_render_state();

    s32 vtxOffset = 0, idxOffset = 0;

    For_as(cmdListIndex, range(drawData->CmdListsCount)) {
        auto *cmdList = drawData->CmdLists[cmdListIndex];
        For(cmdList->CmdBuffer) {
            if (it.UserCallback != null) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer
                // to reset render state.)
                if (it.UserCallback == ImDrawCallback_ResetRenderState) {
                    set_render_state();
                } else {
                    it.UserCallback(cmdList, &it);
                }
            } else {
                rect clipRect = {
                    (s32)(it.ClipRect.x - drawData->DisplayPos.x), (s32)(it.ClipRect.y - drawData->DisplayPos.y),
                    (s32)(it.ClipRect.z - drawData->DisplayPos.x), (s32)(it.ClipRect.w - drawData->DisplayPos.y)};
                ((dx_graphics *) Graphics)->D3DDeviceContext->RSSetScissorRects(1, (D3D11_RECT *) &clipRect);

                FontTexture.bind(0);
                Graphics->draw_indexed(it.ElemCount, it.IdxOffset + idxOffset, it.VtxOffset + vtxOffset);
            }
        }
        idxOffset += cmdList->IdxBuffer.Size;
        vtxOffset += cmdList->VtxBuffer.Size;
    }
    ((dx_graphics *) Graphics)->D3DDeviceContext->RSSetScissorRects(clipRectCount, oldScissorRects);
}

void imgui_renderer::release() {
    VB.release();
    IB.release();
    UB.release();
    FontTexture.release();
    Shader.release();
}

void imgui_renderer::set_render_state() {
    Shader.bind();

    buffer::bind_data data;
    data.Topology = primitive_topology::TriangleList;
    VB.bind(data);
    IB.bind({});

    data.ShaderType = shader::type::VERTEX_SHADER;
    data.Position = Shader.UniformBuffers[0].Position;  // @Hack
    UB.bind(data);
}

LSTD_END_NAMESPACE

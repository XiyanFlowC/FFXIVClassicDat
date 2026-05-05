#include "TextureView.h"
#include <algorithm>
#include <array>
#include <cstring>

#include "../FFXIVClassicDat/GTex.h"

#pragma comment(lib, "d3dcompiler.lib")

struct Vertex
{
    float x, y;
    float u, v;
};

namespace
{
    struct ViewConstants
    {
        float quadScale[2];
        float quadOffset[2];
    };

    uint32_t ReadBE32(const uint8_t* p)
    {
        return (static_cast<uint32_t>(p[0]) << 24) |
            (static_cast<uint32_t>(p[1]) << 16) |
            (static_cast<uint32_t>(p[2]) << 8) |
            static_cast<uint32_t>(p[3]);
    }

    bool GetMip0DataRange(const uint8_t* data, size_t size, const GTexHeader* header, size_t& outOffset, size_t& outAvailable)
    {
        if (!data || !header || size < sizeof(GTexHeader))
            return false;

        if (header->HasDirectData() != 0)
        {
            outOffset = sizeof(GTexHeader);
            outAvailable = size - outOffset;
            return true;
        }

        const uint32_t tableOffset = header->MipOffsetTable();
        if (tableOffset == 0 || tableOffset + 4 > size)
            return false;

        const uint32_t mip0Offset = ReadBE32(data + tableOffset);
        if (mip0Offset >= size)
            return false;

        outOffset = mip0Offset;
        outAvailable = size - outOffset;
        return true;
    }
}

bool TextureView::s_registered = false;

const wchar_t* TextureView::ClassName()
{
    return L"FFXIVClassicDatExplorer_TextureView";
}

TextureView::TextureView() = default;

TextureView::~TextureView()
{
    CleanupD3D();
}

bool TextureView::Create(HWND parent, int x, int y, int w, int h)
{
    m_parent = parent;

    if (!s_registered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = ClassName();
        RegisterClassExW(&wc);
        s_registered = true;
    }

    m_hwnd = CreateWindowExW(0, ClassName(), L"TextureView",
        WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent, nullptr, GetModuleHandle(nullptr), this);

    return m_hwnd != nullptr;
}

void TextureView::LoadTexture(const uint8_t* data, size_t size)
{
    const GTexHeader* header = GTEX_ValidateHeader(data, static_cast<uint32_t>(size));
    if (!header)
    {
        ClearTexture();
        return;
    }

    const int width = static_cast<int>(header->Width());
    const int height = static_cast<int>(header->Height());
    if (width <= 0 || height <= 0)
    {
        ClearTexture();
        return;
    }

    size_t mip0Offset = 0;
    size_t availableSize = 0;
    if (!GetMip0DataRange(data, size, header, mip0Offset, availableSize))
    {
        ClearTexture();
        return;
    }

    if (!InitD3D())
        return;

    const uint8_t formatType = header->format_type;
    const uint8_t* mip0Data = data + mip0Offset;

    m_texture.Reset();
    m_srv.Reset();

    if (formatType == static_cast<uint8_t>(GTexFormatType::A8R8G8B8) ||
        formatType == static_cast<uint8_t>(GTexFormatType::X8R8G8B8))
    {
        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        const size_t need = pixelCount * 4;
        if (availableSize < need)
        {
            ClearTexture();
            return;
        }

        std::vector<uint8_t> decoded(need);
        for (size_t i = 0; i < pixelCount; ++i)
        {
            // D3D9 A8R8G8B8/X8R8G8B8 in memory is BGRA/BGRX byte order.
            const uint8_t b = mip0Data[i * 4 + 0];
            const uint8_t g = mip0Data[i * 4 + 1];
            const uint8_t r = mip0Data[i * 4 + 2];
            const uint8_t a = (formatType == static_cast<uint8_t>(GTexFormatType::A8R8G8B8))
                ? mip0Data[i * 4 + 3]
                : 255;

            decoded[i * 4 + 0] = r;
            decoded[i * 4 + 1] = g;
            decoded[i * 4 + 2] = b;
            decoded[i * 4 + 3] = a;
        }

        CreateTextureResource(width, height, decoded.data());
    }
    else if (formatType == static_cast<uint8_t>(GTexFormatType::A8B8G8R8) ||
             formatType == static_cast<uint8_t>(GTexFormatType::A8R8G8B8LE) ||
             formatType == static_cast<uint8_t>(GTexFormatType::X8B8G8R8) ||
             formatType == static_cast<uint8_t>(GTexFormatType::X8R8G8B8LE))
    {
        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        const size_t need = pixelCount * 4;
        if (availableSize < need)
        {
            ClearTexture();
            return;
        }

        std::vector<uint8_t> decoded(need);
        for (size_t i = 0; i < pixelCount; ++i)
        {
            // A8B8G8R8/X8B8G8R8 family stores bytes as RGBA/RGBX.
            decoded[i * 4 + 0] = mip0Data[i * 4 + 0];
            decoded[i * 4 + 1] = mip0Data[i * 4 + 1];
            decoded[i * 4 + 2] = mip0Data[i * 4 + 2];
            decoded[i * 4 + 3] = (formatType == static_cast<uint8_t>(GTexFormatType::A8B8G8R8) ||
                                  formatType == static_cast<uint8_t>(GTexFormatType::A8R8G8B8LE))
                ? mip0Data[i * 4 + 3]
                : 255;
        }

        CreateTextureResource(width, height, decoded.data());
    }
    else if (formatType == static_cast<uint8_t>(GTexFormatType::A4R4G4B4))
    {
        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        const size_t need = pixelCount * 2;
        if (availableSize < need)
        {
            ClearTexture();
            return;
        }

        std::vector<uint8_t> decoded(pixelCount * 4);
        for (size_t i = 0; i < pixelCount; ++i)
        {
            const uint16_t p = (static_cast<uint16_t>(mip0Data[i * 2]) << 8) |
                static_cast<uint16_t>(mip0Data[i * 2 + 1]);
            decoded[i * 4 + 0] = static_cast<uint8_t>(((p >> 8) & 0xF) * 17);
            decoded[i * 4 + 1] = static_cast<uint8_t>(((p >> 4) & 0xF) * 17);
            decoded[i * 4 + 2] = static_cast<uint8_t>((p & 0xF) * 17);
            decoded[i * 4 + 3] = static_cast<uint8_t>(((p >> 12) & 0xF) * 17);
        }

        CreateTextureResource(width, height, decoded.data());
    }
    else
    {
        DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
        uint32_t blockBytes = 0;

        if (formatType == static_cast<uint8_t>(GTexFormatType::DXT1))
        {
            dxgiFormat = DXGI_FORMAT_BC1_UNORM;
            blockBytes = 8;
        }
        else if (formatType == static_cast<uint8_t>(GTexFormatType::DXT3))
        {
            dxgiFormat = DXGI_FORMAT_BC2_UNORM;
            blockBytes = 16;
        }
        else if (formatType == static_cast<uint8_t>(GTexFormatType::DXT5))
        {
            dxgiFormat = DXGI_FORMAT_BC3_UNORM;
            blockBytes = 16;
        }

        if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
        {
            ClearTexture();
            return;
        }

        const uint32_t blocksWide = std::max(1u, (static_cast<uint32_t>(width) + 3u) / 4u);
        const uint32_t blocksHigh = std::max(1u, (static_cast<uint32_t>(height) + 3u) / 4u);
        const size_t requiredSize = static_cast<size_t>(blocksWide) * blocksHigh * blockBytes;

        if (availableSize < requiredSize)
        {
            ClearTexture();
            return;
        }

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = static_cast<UINT>(width);
        td.Height = static_cast<UINT>(height);
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = dxgiFormat;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = mip0Data;
        initData.SysMemPitch = blocksWide * blockBytes;

        HRESULT hr = m_device->CreateTexture2D(&td, &initData, m_texture.GetAddressOf());
        if (FAILED(hr))
        {
            ClearTexture();
            return;
        }

        hr = m_device->CreateShaderResourceView(m_texture.Get(), nullptr, m_srv.GetAddressOf());
        if (FAILED(hr))
        {
            ClearTexture();
            return;
        }
    }

    m_texWidth = width;
    m_texHeight = height;
    m_zoom = 1.0f;
    m_panX = 0.0f;
    m_panY = 0.0f;
    m_hasTexture = (m_srv != nullptr);
    ClampPan();

    Render();
}

void TextureView::LoadDecoded(const uint8_t* rgbaData, int width, int height)
{
    if (!InitD3D()) return;

    m_texWidth = width;
    m_texHeight = height;
    m_zoom = 1.0f;
    m_panX = 0.0f;
    m_panY = 0.0f;

    CreateTextureResource(width, height, rgbaData);
    m_hasTexture = (m_srv != nullptr);
    ClampPan();

    Render();
}

void TextureView::ClearTexture()
{
    m_hasTexture = false;
    m_srv.Reset();
    m_texture.Reset();
    m_panX = 0.0f;
    m_panY = 0.0f;
    if (m_hwnd)
        InvalidateRect(m_hwnd, nullptr, TRUE);
}

void TextureView::SetBackgroundColor(float r, float g, float b)
{
    m_backgroundColor[0] = std::clamp(r, 0.0f, 1.0f);
    m_backgroundColor[1] = std::clamp(g, 0.0f, 1.0f);
    m_backgroundColor[2] = std::clamp(b, 0.0f, 1.0f);
    m_backgroundColor[3] = 1.0f;

    Render();
}

void TextureView::Show()
{
    if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW);
}

void TextureView::Hide()
{
    if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

void TextureView::Resize(int w, int h)
{
    if (m_hwnd)
    {
        SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
        m_viewWidth = w;
        m_viewHeight = h;
        if (m_swapChain)
        {
            m_rtv.Reset();
            m_swapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
            UpdateViewport();
        }
        ClampPan();
        Render();
    }
}

void TextureView::ClampPan()
{
    const float viewW = static_cast<float>(std::max(1, m_viewWidth));
    const float viewH = static_cast<float>(std::max(1, m_viewHeight));
    const float texW = static_cast<float>(m_hasTexture ? std::max(1, m_texWidth) : 64);
    const float texH = static_cast<float>(m_hasTexture ? std::max(1, m_texHeight) : 64);

    float fitScaleX = 1.0f;
    float fitScaleY = 1.0f;

    const float texAspect = texW / texH;
    const float viewAspect = viewW / viewH;

    if (viewAspect > texAspect)
    {
        fitScaleX = texAspect / viewAspect;
    }
    else
    {
        fitScaleY = viewAspect / texAspect;
    }

    const float halfTexW = 0.5f * viewW * fitScaleX * m_zoom;
    const float halfTexH = 0.5f * viewH * fitScaleY * m_zoom;
    const float halfViewW = 0.5f * viewW;
    const float halfViewH = 0.5f * viewH;

    constexpr float minVisiblePixels = 16.0f;
    const float maxPanX = std::max(0.0f, halfTexW + halfViewW - minVisiblePixels);
    const float maxPanY = std::max(0.0f, halfTexH + halfViewH - minVisiblePixels);

    m_panX = std::clamp(m_panX, -maxPanX, maxPanX);
    m_panY = std::clamp(m_panY, -maxPanY, maxPanY);
}

bool TextureView::CreateShaders()
{
    static constexpr const char* vsSource = R"(
cbuffer ViewCB : register(b0)
{
    float2 gQuadScale;
    float2 gQuadOffset;
};

struct VSInput
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput o;
    float2 p = input.pos * gQuadScale + gQuadOffset;
    o.pos = float4(p, 0.0f, 1.0f);
    o.uv = input.uv;
    return o;
}
)";

    static constexpr const char* psSource = R"(
Texture2D tex : register(t0);
SamplerState samp : register(s0);

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
    return tex.Sample(samp, uv);
}
)";

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompile(vsSource, std::strlen(vsSource), nullptr, nullptr, nullptr,
        "main", "vs_4_0", compileFlags, 0, vsBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
        return false;

    errorBlob.Reset();
    hr = D3DCompile(psSource, std::strlen(psSource), nullptr, nullptr, nullptr,
        "main", "ps_4_0", compileFlags, 0, psBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
        return false;

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vs.GetAddressOf());
    if (FAILED(hr))
        return false;

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_ps.GetAddressOf());
    if (FAILED(hr))
        return false;

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = m_device->CreateInputLayout(layout, static_cast<UINT>(std::size(layout)),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_inputLayout.GetAddressOf());

    return SUCCEEDED(hr);
}

void TextureView::CreatePlaceholderTexture()
{
    if (!m_device || m_placeholderSrv)
        return;

    constexpr int kSize = 64;
    std::array<uint8_t, kSize * kSize * 4> checker = {};

    for (int y = 0; y < kSize; ++y)
    {
        for (int x = 0; x < kSize; ++x)
        {
            const bool dark = (((x / 8) + (y / 8)) % 2) == 0;
            const uint8_t c = dark ? 90 : 130;
            const size_t i = static_cast<size_t>((y * kSize + x) * 4);
            checker[i + 0] = c;
            checker[i + 1] = c;
            checker[i + 2] = c;
            checker[i + 3] = 255;
        }
    }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = kSize;
    td.Height = kSize;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = checker.data();
    initData.SysMemPitch = kSize * 4;

    if (FAILED(m_device->CreateTexture2D(&td, &initData, m_placeholderTexture.GetAddressOf())))
        return;

    m_device->CreateShaderResourceView(m_placeholderTexture.Get(), nullptr, m_placeholderSrv.GetAddressOf());
}

bool TextureView::InitD3D()
{
    if (m_device) return true;

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    m_viewWidth = rc.right;
    m_viewHeight = rc.bottom;

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = m_viewWidth;
    scd.BufferDesc.Height = m_viewHeight;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = m_hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        nullptr, 0, D3D11_SDK_VERSION,
        &scd, m_swapChain.GetAddressOf(), m_device.GetAddressOf(),
        nullptr, m_context.GetAddressOf());

    if (FAILED(hr))
    {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
            nullptr, 0, D3D11_SDK_VERSION,
            &scd, m_swapChain.GetAddressOf(), m_device.GetAddressOf(),
            nullptr, m_context.GetAddressOf());
    }

    if (FAILED(hr))
        return false;

    if (!CreateShaders())
        return false;

    Vertex vertices[] = {
        { -1.0f, -1.0f, 0.0f, 1.0f },
        { -1.0f,  1.0f, 0.0f, 0.0f },
        {  1.0f, -1.0f, 1.0f, 1.0f },
        {  1.0f,  1.0f, 1.0f, 0.0f },
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initDataVb = {};
    initDataVb.pSysMem = vertices;

    hr = m_device->CreateBuffer(&bd, &initDataVb, m_vb.GetAddressOf());
    if (FAILED(hr))
        return false;

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    hr = m_device->CreateSamplerState(&sd, m_sampler.GetAddressOf());
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(ViewConstants);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = m_device->CreateBuffer(&cbDesc, nullptr, m_viewCBuffer.GetAddressOf());
    if (FAILED(hr))
        return false;

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_device->CreateBlendState(&blendDesc, m_alphaBlendState.GetAddressOf());
    if (FAILED(hr))
        return false;

    CreatePlaceholderTexture();
    UpdateViewport();
    return m_rtv != nullptr;
}

void TextureView::CleanupD3D()
{
    m_alphaBlendState.Reset();
    m_viewCBuffer.Reset();
    m_sampler.Reset();
    m_vb.Reset();
    m_inputLayout.Reset();
    m_ps.Reset();
    m_vs.Reset();
    m_srv.Reset();
    m_texture.Reset();
    m_placeholderSrv.Reset();
    m_placeholderTexture.Reset();
    m_rtv.Reset();
    m_swapChain.Reset();
    m_context.Reset();
    m_device.Reset();
}

void TextureView::CreateTextureResource(int width, int height, const uint8_t* rgbaData)
{
    if (!m_device || !rgbaData || width <= 0 || height <= 0)
        return;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = static_cast<UINT>(width);
    td.Height = static_cast<UINT>(height);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = rgbaData;
    initData.SysMemPitch = width * 4;

    m_texture.Reset();
    m_srv.Reset();

    if (FAILED(m_device->CreateTexture2D(&td, &initData, m_texture.GetAddressOf())))
        return;

    m_device->CreateShaderResourceView(m_texture.Get(), nullptr, m_srv.GetAddressOf());
}

void TextureView::UpdateViewport()
{
    if (!m_swapChain || !m_device) return;

    m_rtv.Reset();

    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));

    if (FAILED(hr) || !backBuffer)
    {
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            CleanupD3D();
            InitD3D();
        }
        return;
    }

    m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_rtv.GetAddressOf());
}

void TextureView::Render()
{
    if (!m_device || !m_context || !m_rtv || !m_swapChain) return;

    m_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), nullptr);

    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(m_viewWidth);
    vp.Height = static_cast<float>(m_viewHeight);
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    m_context->ClearRenderTargetView(m_rtv.Get(), m_backgroundColor);

    ID3D11ShaderResourceView* activeSrv = (m_hasTexture && m_srv) ? m_srv.Get() : m_placeholderSrv.Get();

    if (activeSrv && m_inputLayout && m_vb && m_vs && m_ps && m_sampler && m_viewCBuffer)
    {
        ClampPan();

        float texW = static_cast<float>(m_hasTexture ? m_texWidth : 64);
        float texH = static_cast<float>(m_hasTexture ? m_texHeight : 64);
        float viewW = static_cast<float>(std::max(1, m_viewWidth));
        float viewH = static_cast<float>(std::max(1, m_viewHeight));

        float fitScaleX = 1.0f;
        float fitScaleY = 1.0f;

        if (texW > 0.0f && texH > 0.0f)
        {
            const float texAspect = texW / texH;
            const float viewAspect = viewW / viewH;

            if (viewAspect > texAspect)
            {
                fitScaleX = texAspect / viewAspect;
            }
            else
            {
                fitScaleY = viewAspect / texAspect;
            }
        }

        ViewConstants vc{};
        vc.quadScale[0] = fitScaleX * m_zoom;
        vc.quadScale[1] = fitScaleY * m_zoom;
        vc.quadOffset[0] = (2.0f * m_panX) / viewW;
        vc.quadOffset[1] = (-2.0f * m_panY) / viewH;

        m_context->UpdateSubresource(m_viewCBuffer.Get(), 0, nullptr, &vc, 0, 0);

        m_context->IASetInputLayout(m_inputLayout.Get());
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_vb.GetAddressOf(), &stride, &offset);

        m_context->VSSetShader(m_vs.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, m_viewCBuffer.GetAddressOf());
        m_context->PSSetShader(m_ps.Get(), nullptr, 0);
        m_context->PSSetShaderResources(0, 1, &activeSrv);
        m_context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

        const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_context->OMSetBlendState(m_alphaBlendState.Get(), blendFactor, 0xFFFFFFFF);

        m_context->Draw(4, 0);

        m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        ID3D11Buffer* nullCb = nullptr;
        m_context->VSSetConstantBuffers(0, 1, &nullCb);
        ID3D11ShaderResourceView* nullSrv = nullptr;
        m_context->PSSetShaderResources(0, 1, &nullSrv);
    }

    HRESULT hr = m_swapChain->Present(1, 0);

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        CleanupD3D();
    }
}

LRESULT CALLBACK TextureView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TextureView* view = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        view = static_cast<TextureView*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
        view->m_hwnd = hwnd;
    }
    else
    {
        view = reinterpret_cast<TextureView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (view)
        return view->HandleMessage(msg, wParam, lParam);

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT TextureView::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        m_viewWidth = LOWORD(lParam);
        m_viewHeight = HIWORD(lParam);

        if (m_viewWidth == 0 || m_viewHeight == 0)
            break;

        if (m_swapChain)
        {
            m_context->OMSetRenderTargets(0, nullptr, nullptr);
            m_rtv.Reset();

            HRESULT hr = m_swapChain->ResizeBuffers(0, m_viewWidth, m_viewHeight, DXGI_FORMAT_UNKNOWN, 0);
            if (SUCCEEDED(hr))
            {
                UpdateViewport();
                ClampPan();
                Render();
            }
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);
        Render();
        EndPaint(m_hwnd, &ps);
    }
    break;

    case WM_MOUSEWHEEL:
    {
        float delta = GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
        m_zoom *= (1.0f + delta * 0.1f);
        if (m_zoom < 0.1f) m_zoom = 0.1f;
        if (m_zoom > 20.0f) m_zoom = 20.0f;
        ClampPan();
        Render();
    }
    break;

    case WM_LBUTTONDOWN:
        m_dragging = true;
        m_dragStartX = LOWORD(lParam);
        m_dragStartY = HIWORD(lParam);
        m_dragPanX = m_panX;
        m_dragPanY = m_panY;
        SetCapture(m_hwnd);
        break;

    case WM_LBUTTONUP:
        m_dragging = false;
        ReleaseCapture();
        break;

    case WM_MOUSEMOVE:
        if (m_dragging)
        {
            m_panX = m_dragPanX + (LOWORD(lParam) - m_dragStartX) / m_zoom;
            m_panY = m_dragPanY + (HIWORD(lParam) - m_dragStartY) / m_zoom;
            ClampPan();
            Render();
        }
        break;

    case WM_DESTROY:
        CleanupD3D();
        break;

    default:
        return DefWindowProc(m_hwnd, msg, wParam, lParam);
    }
    return 0;
}

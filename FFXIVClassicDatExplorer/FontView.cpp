#include "FontView.h"

#include <algorithm>
#include <sstream>
#include <cstdarg>

#include "../FFXIVClassicDat/FdtFile.h"
#include "../FFXIVClassicDat/FdtFont.h"
#include "../FFXIVClassicDat/GTex.h"

#define IDC_SECTION_COMBO   200
#define IDC_INPUT_EDIT      201

bool FontView::s_registered = false;

const wchar_t* FontView::ClassName()
{
    return L"FFXIVClassicDatExplorer_FontView";
}

FontView::FontView() = default;

FontView::~FontView()
{
    if (m_font) DeleteObject(m_font);
}

bool FontView::Create(HWND parent, int x, int y, int w, int h)
{
    if (!s_registered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = ClassName();
        RegisterClassExW(&wc);
        s_registered = true;
    }

    m_hwnd = CreateWindowExW(0, ClassName(), L"FontView",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        x, y, w, h, parent, nullptr, GetModuleHandle(nullptr), this);

    if (!m_hwnd) return false;

    NONCLIENTMETRICS ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    m_font = CreateFontIndirectW(&ncm.lfMessageFont);

    HFONT hGuiFont = m_font ? m_font : (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    int topY = 4;

    m_hwndSection = CreateWindowExW(0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        4, topY, 160, 200, m_hwnd, (HMENU)(INT_PTR)IDC_SECTION_COMBO,
        GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndSection, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

    m_hwndChannelGrp = CreateWindowExW(0, WC_BUTTONW, L"Channel",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        172, topY - 2, 200, 36, m_hwnd, nullptr,
        GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndChannelGrp, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

    m_hwndChanR = CreateWindowExW(0, WC_BUTTONW, L"R",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        180, topY + 14, 28, 16, m_hwnd, (HMENU)(INT_PTR)IDC_CHANNEL_R,
        GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndChanR, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

    m_hwndChanG = CreateWindowExW(0, WC_BUTTONW, L"G",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        212, topY + 14, 28, 16, m_hwnd, (HMENU)(INT_PTR)IDC_CHANNEL_G,
        GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndChanG, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

    m_hwndChanB = CreateWindowExW(0, WC_BUTTONW, L"B",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        244, topY + 14, 28, 16, m_hwnd, (HMENU)(INT_PTR)IDC_CHANNEL_B,
        GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndChanB, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

    m_hwndChanA = CreateWindowExW(0, WC_BUTTONW, L"A",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        276, topY + 14, 28, 16, m_hwnd, (HMENU)(INT_PTR)IDC_CHANNEL_A,
        GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndChanA, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

    m_hwndChanAll = CreateWindowExW(0, WC_BUTTONW, L"All",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        308, topY + 14, 40, 16, m_hwnd, (HMENU)(INT_PTR)IDC_CHANNEL_ALL,
        GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndChanAll, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

    SendMessage(m_hwndChanAll, BM_SETCHECK, BST_CHECKED, 0);

    m_hwndDetails = CreateWindowExW(0, WC_BUTTONW, L"Details",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        380, topY + 14, 60, 16, m_hwnd, (HMENU)400,
        GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndDetails, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
    SendMessage(m_hwndDetails, BM_SETCHECK, BST_CHECKED, 0);

    topY = 40;

    m_hwndInput = CreateWindowExW(0, WC_EDITW, L"Preview text...",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        4, topY, w - 12, 22, m_hwnd, (HMENU)(INT_PTR)IDC_INPUT_EDIT,
        GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hwndInput, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

    return true;
}

void FontView::LoadFdt(const uint8_t* data, size_t size)
{
    Clear();

    m_fdt = std::make_unique<FdtFile>();
    if (!m_fdt->Parse(data, static_cast<uint32_t>(size)))
    {
        m_fdt.reset();
        return;
    }

    m_fontData = std::make_unique<FdtFont>();
    if (!m_fontData->Load(*m_fdt))
    {
        m_fontData.reset();
        return;
    }

    const auto& gtexSurfaces = m_fontData->GetGtexSurfaces();
    for (size_t i = 0; i < gtexSurfaces.size(); ++i)
    {
        SurfaceInfo si;
        std::ostringstream ss;
        ss << "GTEX Surface " << i;
        si.name = ss.str();
        si.type = 0;
        si.data = gtexSurfaces[i].data();
        si.size = gtexSurfaces[i].size();

        if (si.size >= 32 && memcmp(si.data, "GTEX", 4) == 0)
        {
            const GTexHeader* hdr = reinterpret_cast<const GTexHeader*>(si.data);
            si.width = static_cast<int>(hdr->Width());
            si.height = static_cast<int>(hdr->Height());
        }
        m_surfaces.push_back(si);
    }

    const auto& ddsSurfaces = m_fontData->GetDdsSurfaces();
    for (size_t i = 0; i < ddsSurfaces.size(); ++i)
    {
        SurfaceInfo si;
        std::ostringstream ss;
        ss << "DDS Surface " << i;
        si.name = ss.str();
        si.type = 1;
        si.data = ddsSurfaces[i].data();
        si.size = ddsSurfaces[i].size();

        if (si.size >= 20 && memcmp(si.data, "DDS ", 4) == 0)
        {
            const uint8_t* d = si.data;
            si.height = static_cast<int>(d[12]) | (static_cast<int>(d[13]) << 8) |
                        (static_cast<int>(d[14]) << 16) | (static_cast<int>(d[15]) << 24);
            si.width  = static_cast<int>(d[16]) | (static_cast<int>(d[17]) << 8) |
                        (static_cast<int>(d[18]) << 16) | (static_cast<int>(d[19]) << 24);
        }
        m_surfaces.push_back(si);
    }

    RefreshSectionList();

    if (!m_surfaces.empty())
        LoadSection(0);
}

void FontView::Clear()
{
    m_fdt.reset();
    m_fontData.reset();
    m_surfaces.clear();
    m_fullAtlasRgba.clear();
    m_displayAtlasRgba.clear();
    m_atlasWidth = 0;
    m_atlasHeight = 0;
    m_currentSection = -1;
    m_previewGlyphs.clear();
    m_previewRects.clear();
    m_channel = Channel::All;

    if (m_hwndSection) SendMessage(m_hwndSection, CB_RESETCONTENT, 0, 0);
    if (m_hwndInput) SetWindowTextW(m_hwndInput, L"");
    if (m_hwndChanAll) SendMessage(m_hwndChanAll, BM_SETCHECK, BST_CHECKED, 0);
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
}

void FontView::Show() { if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW); }
void FontView::Hide() { if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE); }

void FontView::Resize(int w, int h)
{
    if (m_hwnd) { SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE); OnSize(w, h); }
}

void FontView::SetChannel(Channel ch)
{
    m_channel = ch;
    ExtractChannelRGBA(m_fullAtlasRgba, ch, m_displayAtlasRgba);
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
}

void FontView::RefreshSectionList()
{
    if (!m_hwndSection) return;
    SendMessage(m_hwndSection, CB_RESETCONTENT, 0, 0);
    for (auto& si : m_surfaces)
    {
        std::wstring wname(si.name.begin(), si.name.end());
        SendMessageW(m_hwndSection, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wname.c_str()));
    }
    if (!m_surfaces.empty())
        SendMessage(m_hwndSection, CB_SETCURSEL, 0, 0);
}

static uint32_t ReadBE32(const uint8_t* p)
{
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8)  | static_cast<uint32_t>(p[3]);
}

void FontView::LoadSection(int index)
{
    if (index < 0 || index >= static_cast<int>(m_surfaces.size())) return;
    m_currentSection = index;
    auto& si = m_surfaces[index];
    if (si.type == 0)
    {
        DecodeGtexSurface(si.data, si.size, si.width, si.height, m_fullAtlasRgba);
        m_atlasWidth = si.width;
        m_atlasHeight = si.height;
        ExtractChannelRGBA(m_fullAtlasRgba, m_channel, m_displayAtlasRgba);
    }
    else
    {
        m_fullAtlasRgba.clear();
        m_displayAtlasRgba.clear();
        m_atlasWidth = si.width;
        m_atlasHeight = si.height;
    }
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
}

void FontView::DecodeGtexSurface(const uint8_t* data, size_t size, int width, int height,
                                  std::vector<uint8_t>& outRgba)
{
    outRgba.clear();
    if (!data || size < 32 || width <= 0 || height <= 0) return;
    const GTexHeader* header = reinterpret_cast<const GTexHeader*>(data);
    if (memcmp(header->magic, "GTEX", 4) != 0) return;
    size_t pixelOffset = 32;
    if (header->HasDirectData() == 0)
    {
        uint32_t tableOff = ReadBE32(header->mip_off_table);
        if (tableOff < size) pixelOffset = tableOff;
    }
    const uint8_t* pixelData = data + pixelOffset;
    size_t avail = size > pixelOffset ? size - pixelOffset : 0;
    size_t pixelCount = static_cast<size_t>(width) * height;
    outRgba.assign(pixelCount * 4, 0);
    uint8_t formatType = header->format_type;
    if (formatType == 0x16)
    {
        for (size_t i = 0; i < pixelCount && i * 2 + 1 < avail; ++i)
        {
            uint16_t p = (static_cast<uint16_t>(pixelData[i * 2]) << 8) |
                         static_cast<uint16_t>(pixelData[i * 2 + 1]);
			uint8_t r = ((p >> 12) & 0xF) * 17;
			uint8_t a = ((p >> 8) & 0xF) * 17;
			uint8_t b = ((p >> 4) & 0xF) * 17;
			uint8_t g = (p & 0xF) * 17;
            outRgba[i * 4 + 0] = r;
            outRgba[i * 4 + 1] = g;
            outRgba[i * 4 + 2] = b;
            outRgba[i * 4 + 3] = a;
        }
    }
    else if (formatType == 0x04)
    {
        for (size_t i = 0; i < pixelCount && i * 4 + 3 < avail; ++i)
        {
            outRgba[i * 4 + 0] = pixelData[i * 4 + 2];
            outRgba[i * 4 + 1] = pixelData[i * 4 + 1];
            outRgba[i * 4 + 2] = pixelData[i * 4 + 0];
            outRgba[i * 4 + 3] = pixelData[i * 4 + 3];
        }
    }
}

void FontView::ExtractChannelRGBA(const std::vector<uint8_t>& src, Channel ch,
                                   std::vector<uint8_t>& dst)
{
    dst.clear();
    if (src.empty()) return;
    size_t pixelCount = src.size() / 4;
    dst.resize(pixelCount * 4);
    if (ch == Channel::All) { dst = src; return; }

    int chOff = 0;
    if (ch == Channel::R) chOff = 0;
    else if (ch == Channel::G) chOff = 1;
    else if (ch == Channel::B) chOff = 2;
    else if (ch == Channel::A) chOff = 3;

    for (size_t i = 0; i < pixelCount; ++i)
    {
        uint8_t gray = src[i * 4 + chOff];
        dst[i * 4 + 0] = gray;
        dst[i * 4 + 1] = gray;
        dst[i * 4 + 2] = gray;
        dst[i * 4 + 3] = 255;
    }
}

void FontView::RenderPreviewString(const std::wstring& text)
{
    m_previewGlyphs.clear();
    m_previewRects.clear();
    if (!m_fontData || !m_hwndInput) return;
    wchar_t inputBuf[256];
    GetWindowTextW(m_hwndInput, inputBuf, 256);
    std::wstring preview(inputBuf);
    if (preview.empty() || preview == L"Preview text...") return;
    if (m_atlasWidth <= 0 || m_atlasHeight <= 0) return;
    for (wchar_t ch : preview)
        m_previewGlyphs.push_back(std::wstring(1, ch));
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
}

void FontView::Paint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);
    if (!hdc) return;
    RECT client;
    GetClientRect(m_hwnd, &client);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
    HBRUSH bgBrush = (HBRUSH)(COLOR_BTNFACE + 1);
    FillRect(memDC, &client, bgBrush);
    if (m_font) SelectObject(memDC, m_font);

    int controlsBottom = 68;
    int previewH = 56;
    int detailsW = m_showDetails ? 260 : 0;

    RECT atlasRc = { 4, controlsBottom, client.right - 4 - detailsW, client.bottom - previewH };
    if (atlasRc.right > atlasRc.left && atlasRc.bottom > atlasRc.top)
        OnPaintAtlas(memDC, atlasRc);

    if (m_showDetails)
    {
        RECT detailRc = { client.right - detailsW, controlsBottom, client.right - 4, client.bottom - previewH };
        if (detailRc.right > detailRc.left && detailRc.bottom > detailRc.top)
            OnPaintMetadata(memDC, detailRc);
    }

    RECT previewRc = { 4, client.bottom - previewH, client.right - 4, client.bottom - 4 };
    if (previewRc.bottom > previewRc.top)
        OnPaintPreview(memDC, previewRc);

    BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
    EndPaint(m_hwnd, &ps);
}

void FontView::OnPaintAtlas(HDC hdc, RECT& rc)
{
    HBRUSH blackBrush = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &rc, blackBrush);
    DeleteObject(blackBrush);
    HPEN grayPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT));
    HPEN oldPen = (HPEN)SelectObject(hdc, grayPen);
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(grayPen);

    if (m_fullAtlasRgba.empty() || m_atlasWidth <= 0 || m_atlasHeight <= 0)
    {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(140, 140, 150));
        DrawTextW(hdc, L"No atlas loaded", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    int atlasAreaW = rc.right - rc.left;
    int atlasAreaH = rc.bottom - rc.top;
    float scale = std::min(static_cast<float>(atlasAreaW) / m_atlasWidth,
                           static_cast<float>(atlasAreaH) / m_atlasHeight);
    int drawW = static_cast<int>(m_atlasWidth * scale);
    int drawH = static_cast<int>(m_atlasHeight * scale);
    int drawX = rc.left + (atlasAreaW - drawW) / 2;
    int drawY = rc.top + (atlasAreaH - drawH) / 2;

    const auto& pixels = m_displayAtlasRgba.empty() ? m_fullAtlasRgba : m_displayAtlasRgba;
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = m_atlasWidth;
    bmi.bmiHeader.biHeight = -m_atlasHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(hdc, drawX, drawY, drawW, drawH, 0, 0, m_atlasWidth, m_atlasHeight,
        pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);

    const wchar_t* chName = L"All";
    if (m_channel == Channel::R) chName = L"R";
    else if (m_channel == Channel::G) chName = L"G";
    else if (m_channel == Channel::B) chName = L"B";
    else if (m_channel == Channel::A) chName = L"A";
    wchar_t info[64];
    swprintf_s(info, L" %dx%d [%s] ", m_atlasWidth, m_atlasHeight, chName);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(200, 200, 200));
    RECT infoRc = { rc.left + 4, rc.top + 4, rc.right - 4, rc.top + 22 };
    DrawTextW(hdc, info, -1, &infoRc, DT_LEFT | DT_TOP | DT_SINGLELINE);
}

void FontView::OnPaintPreview(HDC hdc, RECT& rc)
{
    HBRUSH blackBrush = CreateSolidBrush(RGB(18, 18, 22));
    FillRect(hdc, &rc, blackBrush);
    DeleteObject(blackBrush);
    HPEN grayPen = CreatePen(PS_SOLID, 1, RGB(55, 55, 60));
    HPEN oldPen = (HPEN)SelectObject(hdc, grayPen);
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(grayPen);

    const auto& atlas = m_displayAtlasRgba.empty() ? m_fullAtlasRgba : m_displayAtlasRgba;

    bool canRender = m_fontData && !atlas.empty() && m_atlasWidth > 0 && m_atlasHeight > 0 &&
        !m_previewGlyphs.empty();

    if (canRender)
    {
        uint32_t glyphCount = m_fontData->GetGlyphCount();
        uint32_t fontSize = m_fontData->GetMetrics().fontSize;
        if (fontSize == 0) fontSize = 32;

        // Scan atlas row 0 to find glyph slot width
        int slotW = static_cast<int>(fontSize);
        std::vector<int> runStarts;
        for (int x = 0; x < m_atlasWidth; ++x)
        {
            bool any = false;
            for (int y = 0; y < std::min(static_cast<int>(fontSize), m_atlasHeight); ++y)
            {
                size_t idx = (static_cast<size_t>(y) * m_atlasWidth + x) * 4;
                if (idx + 3 < atlas.size() &&
                    (atlas[idx] > 0x10 || atlas[idx+1] > 0x10 || atlas[idx+2] > 0x10 || atlas[idx+3] > 0x10))
                { any = true; break; }
            }
            if (any && (runStarts.empty() || x - runStarts.back() > 4))
                runStarts.push_back(x);
        }
        if (runStarts.size() >= 2) slotW = runStarts[1] - runStarts[0];

        int glyphsPerChannel = m_atlasWidth / slotW;
        if (glyphsPerChannel < 1) glyphsPerChannel = 1;

        int previewH = rc.bottom - rc.top - 8;
        float renderScale = 1.0f;
        if (fontSize > 0)
            renderScale = std::max(0.2f, std::min(8.0f, static_cast<float>(previewH) / static_cast<float>(fontSize)));

        float penX = static_cast<float>(rc.left + 8);
        float baselineY = static_cast<float>(rc.top + 4) +
            static_cast<float>(m_fontData->GetMetrics().baseline) * renderScale;

        for (size_t ci = 0; ci < m_previewGlyphs.size(); ++ci)
        {
            wchar_t ch = m_previewGlyphs[ci].empty() ? L' ' : m_previewGlyphs[ci][0];
            uint32_t glyphIdx = m_fontData->GetGlyphIndexForChar(static_cast<uint32_t>(ch));
            if (glyphIdx >= glyphCount) { penX += slotW * renderScale; continue; }

            uint32_t perChan = static_cast<uint32_t>(glyphsPerChannel);
            uint32_t fullRow = glyphIdx / (perChan * 4);
            uint32_t rest = glyphIdx % (perChan * 4);
            uint32_t chIdxLocal = rest / perChan;
            uint32_t colIdx = rest % perChan;

            int srcX = static_cast<int>(colIdx) * slotW;
            int srcY = static_cast<int>(fullRow) * static_cast<int>(fontSize);
            int gw = std::min(slotW, m_atlasWidth - srcX);
            int gh = std::min(static_cast<int>(fontSize), m_atlasHeight - srcY);
            if (gw <= 0 || gh <= 0) { penX += slotW * renderScale; continue; }

            int dw = std::max(1, static_cast<int>(gw * renderScale));
            int dh = std::max(1, static_cast<int>(gh * renderScale));
            int dx = static_cast<int>(penX);
            int dy = static_cast<int>(baselineY - static_cast<float>(gh) * renderScale);

            BITMAPINFO glyphBmi = {};
            glyphBmi.bmiHeader.biSize = sizeof(glyphBmi.bmiHeader);
            glyphBmi.bmiHeader.biWidth = gw;
            glyphBmi.bmiHeader.biHeight = gh;
            glyphBmi.bmiHeader.biPlanes = 1;
            glyphBmi.bmiHeader.biBitCount = 32;
            glyphBmi.bmiHeader.biCompression = BI_RGB;

            void* glyphBits = nullptr;
            HBITMAP hGlyphBmp = CreateDIBSection(hdc, &glyphBmi, DIB_RGB_COLORS, &glyphBits, nullptr, 0);
            if (!hGlyphBmp || !glyphBits) { penX += slotW * renderScale; continue; }

            uint8_t* gdst = static_cast<uint8_t*>(glyphBits);
            for (int gy = 0; gy < gh; ++gy)
            {
                for (int gx = 0; gx < gw; ++gx)
                {
                    size_t atlasOff = (static_cast<size_t>(srcY + gy) * m_atlasWidth + srcX + gx) * 4;
                    uint8_t chVal = 0;
                    if (atlasOff + 3 < atlas.size())
                    {
                        static const int kOrder[4] = { 3, 0, 1, 2 }; // font packing: A,R,G,B
                        chVal = atlas[atlasOff + kOrder[chIdxLocal]];
                    }
                    size_t dstOff = (static_cast<size_t>(gh - 1 - gy) * gw + gx) * 4;
                    gdst[dstOff + 0] = chVal;
                    gdst[dstOff + 1] = chVal;
                    gdst[dstOff + 2] = chVal;
                    gdst[dstOff + 3] = chVal;
                }
            }

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP oldMbmp = (HBITMAP)SelectObject(memDC, hGlyphBmp);

            BLENDFUNCTION bf = {};
            bf.BlendOp = AC_SRC_OVER;
            bf.SourceConstantAlpha = 255;
            bf.AlphaFormat = AC_SRC_ALPHA;
            AlphaBlend(hdc, dx, dy, dw, dh, memDC, 0, 0, gw, gh, bf);

            SelectObject(memDC, oldMbmp);
            DeleteDC(memDC);
            DeleteObject(hGlyphBmp);

            penX += static_cast<float>(slotW) * renderScale;
        }
    }
    else
    {
        RECT textRc = { rc.left + 12, rc.top + 4, rc.right - 12, rc.bottom - 4 };
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(130, 130, 140));
        DrawTextW(hdc, L"Type text above to preview glyph rendering", -1, &textRc,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    if (m_fontData)
    {
        wchar_t info[128];
        auto& met = m_fontData->GetMetrics();
        swprintf_s(info, L"[v%d, %ux%u, glyphs:%u]",
            met.version, met.fontSize, met.fontSizeMax, m_fontData->GetGlyphCount());
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(150, 150, 160));
        RECT infoRc = { rc.left + 12, rc.top + 4, rc.right - 12, rc.bottom - 4 };
        DrawTextW(hdc, info, -1, &infoRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }
}

void FontView::OnPaintMetadata(HDC hdc, RECT& rc)
{
    HBRUSH bg = CreateSolidBrush(RGB(35, 35, 40));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(70, 70, 75));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    SetBkMode(hdc, TRANSPARENT);
    int y = rc.top + 8;
    int x = rc.left + 10;
    int lineH = 20;

    if (m_fontData)
    {
        auto& m = m_fontData->GetMetrics();

        auto drawLine = [&](const wchar_t* label, const wchar_t* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            wchar_t tmp[128];
            wvsprintfW(tmp, fmt, args);
            va_end(args);
            SetTextColor(hdc, RGB(180, 180, 190));
            RECT lr = { x, y, x + 120, y + lineH };
            DrawTextW(hdc, label, -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SetTextColor(hdc, RGB(240, 240, 255));
            RECT vr = { x + 125, y, rc.right - 10, y + lineH };
            DrawTextW(hdc, tmp, -1, &vr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            y += lineH;
        };

        SetTextColor(hdc, RGB(255, 200, 60));
        RECT tr = { x, y, rc.right - 10, y + lineH };
        DrawTextW(hdc, L"Font Metrics", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        y += lineH + 4;

        drawLine(L"Version:", L"%u", m.version);
        drawLine(L"Font Size:", L"%u px", m.fontSize);
        drawLine(L"Max Size:", L"%u px", m.fontSizeMax);
        drawLine(L"Baseline:", L"%u", m.baseline);
        drawLine(L"Hang Margin:", L"%u", m.hangMargin);
        drawLine(L"Texture Count:", L"%u", m.textureCount);

        y += 8;
        SetTextColor(hdc, RGB(255, 200, 60));
        RECT tr2 = { x, y, rc.right - 10, y + lineH };
        DrawTextW(hdc, L"Glyph Data", -1, &tr2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        y += lineH + 4;

        drawLine(L"Glyphs:", L"%u", m_fontData->GetGlyphCount());
        drawLine(L"Lookup Bytes:", L"%zu", m_fontData->GetLookupTable().size());
        drawLine(L"Height Bytes:", L"%zu", m_fontData->GetHeightTable().size());
        drawLine(L"GTEX Surfaces:", L"%zu", m_fontData->GetGtexSurfaces().size());
        drawLine(L"DDS Surfaces:", L"%zu", m_fontData->GetDdsSurfaces().size());

        if (m_currentSection >= 0 && m_currentSection < static_cast<int>(m_surfaces.size()))
        {
            y += 8;
            SetTextColor(hdc, RGB(255, 200, 60));
            RECT tr3 = { x, y, rc.right - 10, y + lineH };
            DrawTextW(hdc, L"Current Section", -1, &tr3, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            y += lineH + 4;
            auto& si = m_surfaces[m_currentSection];
            drawLine(L"Name:", L"%S", si.name.c_str());
            drawLine(L"Type:", L"%s", si.type == 0 ? L"GTEX" : L"DDS");
            drawLine(L"Size:", L"%dx%d", si.width, si.height);
        }
    }
    else
    {
        SetTextColor(hdc, RGB(160, 160, 170));
        RECT nr = { x, y, rc.right - 10, y + lineH };
        DrawTextW(hdc, L"No font data loaded", -1, &nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

void FontView::OnSize(int w, int h)
{
    if (m_hwndInput)
        SetWindowPos(m_hwndInput, nullptr, 4, 40, w - 12, 22, SWP_NOZORDER);
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
}

void FontView::OnCommand(int id)
{
    if (id == IDC_SECTION_COMBO)
    {
        int sel = static_cast<int>(SendMessage(m_hwndSection, CB_GETCURSEL, 0, 0));
        if (sel >= 0 && sel < static_cast<int>(m_surfaces.size()))
            LoadSection(sel);
    }
    else if (id == IDC_INPUT_EDIT)
    {
        wchar_t buf[256];
        if (m_hwndInput) GetWindowTextW(m_hwndInput, buf, 256);
        RenderPreviewString(buf);
    }
    else if (id == IDC_CHANNEL_R)   { if (SendMessage(m_hwndChanR, BM_GETCHECK, 0, 0) == BST_CHECKED) SetChannel(Channel::R); }
    else if (id == IDC_CHANNEL_G)   { if (SendMessage(m_hwndChanG, BM_GETCHECK, 0, 0) == BST_CHECKED) SetChannel(Channel::G); }
    else if (id == IDC_CHANNEL_B)   { if (SendMessage(m_hwndChanB, BM_GETCHECK, 0, 0) == BST_CHECKED) SetChannel(Channel::B); }
    else if (id == IDC_CHANNEL_A)   { if (SendMessage(m_hwndChanA, BM_GETCHECK, 0, 0) == BST_CHECKED) SetChannel(Channel::A); }
    else if (id == IDC_CHANNEL_ALL) { if (SendMessage(m_hwndChanAll, BM_GETCHECK, 0, 0) == BST_CHECKED) SetChannel(Channel::All); }
    else if (id == 400)
    {
        m_showDetails = (SendMessage(m_hwndDetails, BM_GETCHECK, 0, 0) == BST_CHECKED);
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

LRESULT CALLBACK FontView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    FontView* view = nullptr;
    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        view = static_cast<FontView*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
        view->m_hwnd = hwnd;
    }
    else
        view = reinterpret_cast<FontView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (view) return view->HandleMessage(msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT FontView::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE: OnSize(LOWORD(lParam), HIWORD(lParam)); break;
    case WM_COMMAND: OnCommand(LOWORD(wParam)); break;
    case WM_PAINT: Paint(); break;
    case WM_CTLCOLORSTATIC: return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    default: return DefWindowProc(m_hwnd, msg, wParam, lParam);
    }
    return 0;
}

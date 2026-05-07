#pragma once

#include <windows.h>
#undef ERROR
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <commctrl.h>

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

class FdtFile;
class FdtFont;
struct FdtGlyph;

class FontView
{
public:
    FontView();
    ~FontView();

    bool Create(HWND parent, int x, int y, int w, int h);
    HWND GetHwnd() const { return m_hwnd; }

    void LoadFdt(const uint8_t* data, size_t size);
    void Clear();

    void Show();
    void Hide();
    void Resize(int w, int h);

    enum class Channel { R, G, B, A, All };

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnSize(int w, int h);
    void OnCommand(int id);
    void OnPaintAtlas(HDC hdc, RECT& rc);
    void OnPaintPreview(HDC hdc, RECT& rc);
    void OnPaintMetadata(HDC hdc, RECT& rc);
    void Paint();

    void RefreshSectionList();
    void LoadSection(int index);
    void SetChannel(Channel ch);
    void RenderPreviewString(const std::wstring& text);

    void DecodeGtexSurface(const uint8_t* data, size_t size, int width, int height,
                           std::vector<uint8_t>& outRgba);
    void ExtractChannelRGBA(const std::vector<uint8_t>& src, Channel ch,
                            std::vector<uint8_t>& dst);

    HWND m_hwnd = nullptr;
    HWND m_hwndSection = nullptr;
    HWND m_hwndInput = nullptr;
    HWND m_hwndChannelGrp = nullptr;
    HWND m_hwndChanR = nullptr;
    HWND m_hwndChanG = nullptr;
    HWND m_hwndChanB = nullptr;
    HWND m_hwndChanA = nullptr;
    HWND m_hwndChanAll = nullptr;
    HWND m_hwndDetails = nullptr;
    HFONT m_font = nullptr;

    std::unique_ptr<FdtFile> m_fdt;
    std::unique_ptr<FdtFont> m_fontData;

    struct SurfaceInfo
    {
        std::string name;
        int type = 0;
        int width = 0;
        int height = 0;
        const uint8_t* data = nullptr;
        size_t size = 0;
    };
    std::vector<SurfaceInfo> m_surfaces;

    std::vector<uint8_t> m_fullAtlasRgba;
    std::vector<uint8_t> m_displayAtlasRgba;
    int m_atlasWidth = 0;
    int m_atlasHeight = 0;
    int m_currentSection = -1;
    Channel m_channel = Channel::All;

    std::vector<RECT> m_previewRects;
    std::vector<std::wstring> m_previewGlyphs;

    bool m_showDetails = true;

    static bool s_registered;
    static const wchar_t* ClassName();

    static constexpr int IDC_CHANNEL_R    = 300;
    static constexpr int IDC_CHANNEL_G    = 301;
    static constexpr int IDC_CHANNEL_B    = 302;
    static constexpr int IDC_CHANNEL_A    = 303;
    static constexpr int IDC_CHANNEL_ALL  = 304;
};

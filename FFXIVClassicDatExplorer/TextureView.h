#pragma once

#include <windows.h>
#undef ERROR
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <commctrl.h>
#include <cstdint>
#include <vector>
#include <string>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct GTexHeader;

class TextureView
{
public:
	TextureView();
	~TextureView();

	bool Create(HWND parent, int x, int y, int w, int h);
	HWND GetHwnd() const { return m_hwnd; }

	void LoadTexture(const uint8_t* data, size_t size);
	void LoadDecoded(const uint8_t* rgbaData, int width, int height);
    void ClearTexture();
    void SetBackgroundColor(float r, float g, float b);
    bool SaveTextureToFile(const std::wstring& path);
    bool SaveTextureToPng(const std::wstring& path);

    void Show();
	void Hide();
	void Resize(int w, int h);

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	bool InitD3D();
	bool CreateShaders();
	void CleanupD3D();
	void Render();
	void CreateTextureResource(int width, int height, const uint8_t* rgbaData);
	void UpdateViewport();
    void CreatePlaceholderTexture();
    void ClampPan();
    void UpdateZoomLabel();

    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    HWND m_hwndZoomLabel = nullptr;

	// D3D11
	ComPtr<ID3D11Device> m_device;
	ComPtr<ID3D11DeviceContext> m_context;
	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D11RenderTargetView> m_rtv;
	ComPtr<ID3D11Texture2D> m_texture;
	ComPtr<ID3D11ShaderResourceView> m_srv;
	ComPtr<ID3D11Texture2D> m_placeholderTexture;
	ComPtr<ID3D11ShaderResourceView> m_placeholderSrv;

	// Shader resources
	ComPtr<ID3D11VertexShader> m_vs;
	ComPtr<ID3D11PixelShader> m_ps;
	ComPtr<ID3D11InputLayout> m_inputLayout;
	ComPtr<ID3D11Buffer> m_vb;
	ComPtr<ID3D11SamplerState> m_sampler;
	ComPtr<ID3D11Buffer> m_viewCBuffer;
	ComPtr<ID3D11BlendState> m_alphaBlendState;

	int m_texWidth = 256;
	int m_texHeight = 256;
	bool m_hasTexture = false;

	float m_zoom = 1.0f;
	float m_panX = 0.0f;
	float m_panY = 0.0f;
	bool m_dragging = false;
	int m_dragStartX = 0;
	int m_dragStartY = 0;
	float m_dragPanX = 0.0f;
	float m_dragPanY = 0.0f;

	int m_viewWidth = 0;
	int m_viewHeight = 0;
	float m_backgroundColor[4] = { 0.15f, 0.15f, 0.15f, 1.0f };

	static bool s_registered;
	static const wchar_t* ClassName();
};

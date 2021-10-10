#pragma once

#include <memory>

#include <d3d11.h>
#include <dxgi1_2.h>
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "DXGI.lib")

#include "Utility\Logger.h"
#include "Utility\GdiplusUtil.h"

class DesktopDuplication
{
public:

	std::unique_ptr<Gdiplus::Bitmap>	ScreenShot(HWND hwndTarget, const CRect& rcAdjustClient)
	{
		_UpdateTargtOutputDuplication(hwndTarget);
		ATLASSERT(m_duplication);
		if (!m_duplication) {
			return nullptr;
		}
		HRESULT hr = S_OK;
		const UINT timeout_ms = 500;
		DXGI_OUTDUPL_FRAME_INFO frame_info = {};
		CComPtr<IDXGIResource> resource;

		enum { kMaxAcquireNextFrameCount = 4 };
		for (int i = 0; i < kMaxAcquireNextFrameCount; ++i) {
			HRESULT hr = m_duplication->AcquireNextFrame(timeout_ms, &frame_info, &resource);
			ATLASSERT(SUCCEEDED(hr));
			if (hr == DXGI_ERROR_ACCESS_LOST) {
				m_duplication.Release();
				return nullptr;
			}
			if (frame_info.LastPresentTime.QuadPart != 0) {
				break;	// success
			}
			resource.Release();
			m_duplication->ReleaseFrame();
		}
		if (!resource) {
			ERROR_LOG << L"AcquireNextFrame retry over";
			return nullptr;
		}


		CComQIPtr<ID3D11Texture2D> surface = resource;
		ATLASSERT(surface);
		D3D11_TEXTURE2D_DESC desc = {};
		surface->GetDesc(&desc);

		// Change flags
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		ATLASSERT(desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM);
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;	// 一応上書きしておく

		CComPtr<ID3D11Texture2D>	cpuReadableTexture;
		hr = m_device->CreateTexture2D(&desc, NULL, &cpuReadableTexture);

		// Use GPU to copy GPU-readable texture to CPU-readable texture
		m_deviceContext->CopyResource(cpuReadableTexture, surface);

		// Use CPU to read from CPU-readable texture
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		hr = m_deviceContext->Map(cpuReadableTexture, 0, D3D11_MAP_READ, 0, &mappedResource);

		// VRAM to RAM
		auto ssMonitorBmp32 = std::make_unique<Gdiplus::Bitmap>(desc.Width, desc.Height, mappedResource.RowPitch, PixelFormat32bppARGB, (BYTE*)mappedResource.pData);

		m_deviceContext->Unmap(cpuReadableTexture, 0);

		// 24bit colorへ変換 & ウィンドウの範囲に切り抜き
		auto ssBmp24 = std::make_unique<Gdiplus::Bitmap>(
			rcAdjustClient.Width(), rcAdjustClient.Height(), PixelFormat24bppRGB);
		Gdiplus::Graphics graphics(ssBmp24.get());

		switch (m_last_outputDesc.Rotation) {
		case DXGI_MODE_ROTATION_ROTATE90:
			ssMonitorBmp32->RotateFlip(Gdiplus::Rotate90FlipNone);
			break;
		case DXGI_MODE_ROTATION_ROTATE180:
			ssMonitorBmp32->RotateFlip(Gdiplus::Rotate180FlipNone);
			break;
		case DXGI_MODE_ROTATION_ROTATE270:
			ssMonitorBmp32->RotateFlip(Gdiplus::Rotate270FlipNone);
			break;
		}

		graphics.DrawImage(ssMonitorBmp32.get(), 0, 0,
			rcAdjustClient.left - m_last_outputDesc.DesktopCoordinates.left,
			rcAdjustClient.top - m_last_outputDesc.DesktopCoordinates.top,
			rcAdjustClient.Width(), rcAdjustClient.Height(), Gdiplus::UnitPixel);

		hr = m_duplication->ReleaseFrame();
		return ssBmp24;
	}

private:
	void	_UpdateTargtOutputDuplication(HWND hwndTarget)
	{
		HMONITOR hMonitor = ::MonitorFromWindow(hwndTarget, MONITOR_DEFAULTTONEAREST);
		ATLASSERT(hMonitor);
		MONITORINFOEX monitorInfo = {};
		monitorInfo.cbSize = sizeof(monitorInfo);
		::GetMonitorInfo(hMonitor, &monitorInfo);

		if (!m_duplication ||
			m_lastHwndTarget != hwndTarget ||
			m_lastTargetHWNDMonitorName != monitorInfo.szDevice)
		{
			m_duplication.Release();
			m_lastHwndTarget = hwndTarget;
			m_lastTargetHWNDMonitorName = monitorInfo.szDevice;

			CComPtr<IDXGIFactory1> factory;
			CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));

			// 全ディスプレイアダプタを調べる
			CComPtr<IDXGIAdapter1> adapter;
			for (int i = 0; (factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND); ++i)
			{
				// アウトプットを一通り調べてメインモニタを探す
				CComPtr<IDXGIOutput> output;
				for (int j = 0; (adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND); j++)
				{
					output->GetDesc(&m_last_outputDesc);

					// hwndTarget のモニターを発見
					if (::wcscmp(monitorInfo.szDevice, m_last_outputDesc.DeviceName) == 0) {

						// デバイス作成
						m_device.Release();
						m_deviceContext.Release();
						HRESULT hr = ::D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &m_device, nullptr, &m_deviceContext);
						if (FAILED(hr)) {
							ERROR_LOG << L"D3D11CreateDevice failed";
							//throw std::runtime_error("D3D11CreateDevice");
							return;	// failed
						}

						CComQIPtr<IDXGIOutput1> output1 = output;
						output1->DuplicateOutput(m_device, &m_duplication);
						ATLASSERT(m_duplication);
						return;	// update!
					}
					output.Release();
				}
				adapter.Release();
			}
		}
	}

	CComPtr<ID3D11Device> m_device;
	CComPtr<ID3D11DeviceContext> m_deviceContext;
	CComPtr<IDXGIOutputDuplication> m_duplication;

	HWND	m_lastHwndTarget;
	std::wstring m_lastTargetHWNDMonitorName;
	DXGI_OUTPUT_DESC m_last_outputDesc;
};


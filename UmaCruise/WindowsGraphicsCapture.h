#pragma once

#include "winrt/base.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#pragma comment(lib, "windowsapp")

#include <shcore.h>
#include <dwmapi.h>
#include <DispatcherQueue.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <d2d1_3helper.h>
#include <windows.ui.composition.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <Windows.Graphics.Capture.Interop.h>

#include <VersionHelpers.h>

#include "Utility\Logger.h"
#include "Utility\GdiplusUtil.h"

#pragma comment(lib, "Dwmapi.lib")	// for DwmGetWindowAttribute

#include "IScreenShotWindow.h"

namespace winrt
{
	using namespace winrt::Windows::System;
	using namespace winrt::Windows::Foundation;
	using namespace winrt::Windows::Graphics;
	using namespace winrt::Windows::Graphics::DirectX;
	using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
	using namespace winrt::Windows::Graphics::Capture;
}

template <typename T>
auto GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object)
{
	auto access = object.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
	winrt::com_ptr<T> result;
	winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
	return result;
}


class WindowsGraphicsCapture : public IScreenShotWindow
{
public:
	~WindowsGraphicsCapture()
	{
		_Release();
	}

	static bool IsSupported()
	{
		using fnRtlGetNtVersionNumbers = void (WINAPI*)(LPDWORD major, LPDWORD minor, LPDWORD build);
		auto RtlGetNtVersionNumbers = 
			reinterpret_cast<fnRtlGetNtVersionNumbers>(
								GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
		ATLASSERT(RtlGetNtVersionNumbers);
		DWORD major, minor, buildNumber;
		RtlGetNtVersionNumbers(&major, &minor, &buildNumber);
		buildNumber &= ~0xF0000000;
		if (major >= 10 && buildNumber >= 19041) {
			bool b = winrt::GraphicsCaptureSession::IsSupported();
			return b;
		}
		return false;
	}

	std::unique_ptr<Gdiplus::Bitmap>	ScreenShot(HWND hwndTarget, const CRect& rcAdjustClient) override
	{
		_UpdateTargt(hwndTarget);
		if (!IsCaptureStart()) {
			return nullptr;
		}

		HRESULT hr = S_OK;
		auto surfaceTexture = TakeAsync();
		if (!surfaceTexture) {
			return nullptr;
		}

		CComQIPtr<ID3D11Texture2D> surface = surfaceTexture.get();
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

		// 
		CRect dwmWindowRect;
		DwmGetWindowAttribute(hwndTarget, DWMWA_EXTENDED_FRAME_BOUNDS, (PVOID)&dwmWindowRect, sizeof(dwmWindowRect));

		const int borderWidth = (dwmWindowRect.Width() - rcAdjustClient.Width()) / 2;
		// title bar + border 分ずらす
		const int top = dwmWindowRect.Height() - (rcAdjustClient.Height() + borderWidth);
		const int left = borderWidth;

		graphics.DrawImage(ssMonitorBmp32.get(), 0, 0,
			left,
			top, 
			rcAdjustClient.Width(), rcAdjustClient.Height(), Gdiplus::UnitPixel);

		return ssBmp24;
	}

	bool	IsCaptureStart() {
		return m_framePool != nullptr;
	}


private:
	void	_Release()
	{
		if (m_session) {
			m_session.Close();
			m_session = nullptr;
		}
		if (m_framePool) {
			m_framePool.Close();
			m_framePool = nullptr;
		}
		if (m_rtDevice) {
			m_rtDevice.Close();
			m_rtDevice = nullptr;
		}
		m_deviceContext.Release();
		m_device.Release();
	}

	winrt::GraphicsCaptureItem CreateCaptureItemForWindow(HWND hwnd)
	{
		namespace abi = ABI::Windows::Graphics::Capture;

		auto factory = winrt::get_activation_factory<winrt::GraphicsCaptureItem>();
		auto interop = factory.as<IGraphicsCaptureItemInterop>();
		winrt::GraphicsCaptureItem item{ nullptr };
		winrt::check_hresult(interop->CreateForWindow(hwnd, winrt::guid_of<abi::IGraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(item))));
		return item;
	}


	void	_UpdateTargt(HWND hwndTarget)
	{
		static bool s_bInit = false;
		if (!s_bInit) {
			winrt::init_apartment(/*winrt::apartment_type::single_threaded*/);
			s_bInit = true;
		}

		if (hwndTarget == NULL) {
			if (m_framePool) {
				ATLASSERT(m_session);
				m_session.Close();
				m_session = nullptr;
				m_framePool.Close();
				m_framePool = nullptr;
			}

			if (m_rtDevice) {
				m_rtDevice.Close();
				m_rtDevice = nullptr;
			}

			m_deviceContext.Release();
			m_device.Release();
			return;	// release!
		}

		HMONITOR hMonitor = ::MonitorFromWindow(hwndTarget, MONITOR_DEFAULTTONEAREST);
		ATLASSERT(hMonitor);
		MONITORINFOEX monitorInfo = {};
		monitorInfo.cbSize = sizeof(monitorInfo);
		::GetMonitorInfo(hMonitor, &monitorInfo);

		if (!m_framePool  ||
			m_lastHwndTarget != hwndTarget ||
			m_lastTargetHWNDMonitorName != monitorInfo.szDevice)
		{
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
						_CreateDeviceAndCaptureStart(adapter, hwndTarget);
						
						return;	// update!
					}
					output.Release();
				}
				adapter.Release();
			}
			// 見つからなかった...
			// ので、デフォルトのデバイスで初期化する
			_CreateDeviceAndCaptureStart(nullptr, hwndTarget);
			return;

		} else {
			// 更新なし
		}
	}

	void	_CreateDeviceAndCaptureStart(IDXGIAdapter1* adapter, HWND hwndTarget)
	{
		_Release();

		// デバイス作成
		HRESULT hr = S_OK;
		if (!adapter) {
			hr = ::D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &m_device, nullptr, &m_deviceContext);
		} else {
			hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &m_device, nullptr, &m_deviceContext);
		}
		if (FAILED(hr)) {
			ERROR_LOG << L"D3D11CreateDevice failed";
			//throw std::runtime_error("D3D11CreateDevice");
			return;	// failed
		}

		// ID3D11Device -> winrt::IDirect3DDevice
		CComQIPtr<IDXGIDevice> dxgiDevice = m_device;
		ATLASSERT(dxgiDevice);
		winrt::com_ptr<::IInspectable> device;
		hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice, device.put());
		if (FAILED(hr) || !device) {
			ERROR_LOG << L"CreateDirect3D11DeviceFromDXGIDevice failed";
			return;
		}
		m_rtDevice = device.as<winrt::IDirect3DDevice>();
		ATLASSERT(m_rtDevice);

		// ==
		auto item = CreateCaptureItemForWindow(hwndTarget);
		if (!item) {
			ERROR_LOG << L"CreateCaptureItemForWindow failed";
			return;
		}

		m_lastSize = item.Size();
		m_framePool = winrt::Direct3D11CaptureFramePool::Create(
			m_rtDevice,
			winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
			1,
			item.Size());
		if (!m_framePool) {
			ERROR_LOG << L"Direct3D11CaptureFramePool::Create failed";
			return;
		}

		m_session = m_framePool.CreateCaptureSession(item);
		if (!m_session) {
			ERROR_LOG << L"m_framePool.CreateCaptureSession failed";
			return;
		}
		m_session.IsCursorCaptureEnabled(false);

		m_session.StartCapture();
	}


	winrt::com_ptr<ID3D11Texture2D> TakeAsync()
	{
		ATLASSERT(m_framePool);
		enum { kMaxRetryCount = 1000 };
		for (int retryCount = 0; retryCount < kMaxRetryCount; ++retryCount) {
			auto frame = m_framePool.TryGetNextFrame();
			if (!frame) {
				::Sleep(1);
				//WARN_LOG << L"TryGetNextFrame frame empty - retryCount: " << retryCount;
				continue;
			}

			// ウィンドウサイズが変更されたのでフレームバッファサイズを変更させる
			auto frameDesc = frame.Surface().Description();			
			winrt::SizeInt32 size = frame.ContentSize();// { frameDesc.Width, frameDesc.Height };
			if (size != m_lastSize) {
				m_lastSize = size;
				m_framePool.Recreate(m_rtDevice, winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized, 1, size);
				continue;
			}

			auto frameTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
			ATLASSERT(frameTexture);
			return frameTexture;
		}
		ATLASSERT(FALSE);
		return nullptr;
	}

	CComPtr<ID3D11Device> m_device;
	CComPtr<ID3D11DeviceContext> m_deviceContext;

	HWND	m_lastHwndTarget;
	std::wstring m_lastTargetHWNDMonitorName;
	DXGI_OUTPUT_DESC m_last_outputDesc;

	winrt::IDirect3DDevice	m_rtDevice;
	winrt::Direct3D11CaptureFramePool	m_framePool{ nullptr };
	winrt::GraphicsCaptureSession	m_session{ nullptr };
	winrt::SizeInt32	m_lastSize;

};
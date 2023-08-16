#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <cassert>

static bool g_window_resized = false;
static bool g_device_lost = false;

///////////////////////////////////////////////////////////////////////////////////////////
// Prototypes
///////////////////////////////////////////////////////////////////////////////////////////

void create_device_independent_resources(ID2D1Factory **d2d1_factory,
                                         IDWriteFactory **dwrite_factory,
                                         IDWriteTextFormat **dwrite_text_format);

HRESULT create_d3d_device(D3D_DRIVER_TYPE const kD3DDriverType,
                          ID3D11Device1 **d3d11_device,
                          ID3D11DeviceContext1 **d3d11_device_context);

void create_device_dependent_resources(HWND hwnd,
                                       ID3D11Device1 **d3d11_device,
                                       ID3D11DeviceContext1 **d3d11_device_context,
                                       IDXGISwapChain1 **dxgi_swap_chain);

void discard_device_dependent_resources(ID3D11Device1 **d3d11_device,
                                        ID3D11DeviceContext1 **d3d11_device_context,
                                        ID3D11RenderTargetView **d3d11_frame_buffer_view,
                                        ID3D11VertexShader **vertex_shader,
                                        ID3D11PixelShader **pixel_shader,
                                        IDXGISwapChain1 **dxgi_swap_chain,
                                        ID2D1RenderTarget **d2d1_back_buffer_render_target,
                                        ID2D1SolidColorBrush **d2d1_text_brush);

void create_render_target_resources(HWND hwnd,
                                    ID3D11Device1 *d3d11_device,
                                    IDXGISwapChain1 *dxgi_swap_chain,
                                    ID2D1Factory *d2d1_factory,
                                    ID3D11RenderTargetView **d3d11_frame_buffer_view,
                                    ID2D1RenderTarget **d2d1_back_buffer_render_target,
                                    ID2D1SolidColorBrush **d2d1_text_brush);

void discard_render_target_resources(ID3D11DeviceContext1 **d3d11_device_context,
                                     ID3D11RenderTargetView **d3d11_frame_buffer_view,
                                     ID2D1RenderTarget **d2d1_back_buffer_render_target,
                                     ID2D1SolidColorBrush **d2d1_text_brush);

void create_vertex_shader(ID3D11Device *d3d11_device,
                          ID3DBlob **vsBlob,
                          ID3D11VertexShader **vertex_shader);

void create_pixel_shader(ID3D11Device *d3d11_device,
                         ID3DBlob **ps_blob,
                         ID3D11PixelShader **pixel_shader);

bool shader_compiler_succeeded(HRESULT hr, ID3DBlob *shader_compiler_error_blob);

void render(HWND hwnd,
            ID3D11DeviceContext1 *d3d11_device_context,
            ID3D11RenderTargetView **d3d11_frame_buffer_view,
            ID3D11InputLayout *input_layout,
            ID3D11VertexShader *vertex_shader,
            ID3D11PixelShader *pixel_shader,
            ID3D11Buffer **vertex_buffer,
            UINT *stride,
            UINT *offset,
            UINT nVertices,
            IDXGISwapChain1 *dxgi_swap_chain,
            ID2D1RenderTarget *d2d1_back_buffer_render_target,
            IDWriteTextFormat *dwrite_text_format,
            ID2D1SolidColorBrush *d2d1_text_brush);

void resize(HWND hwnd,
            ID3D11Device1 **d3d11_device,
            ID3D11DeviceContext1 **d3d11_device_context,
            IDXGISwapChain1 **dxgi_swap_chain,
            ID2D1Factory **d2d1_factory,
            ID3D11RenderTargetView **d3d11_frame_buffer_view,
            ID2D1RenderTarget **d2d1_back_buffer_render_target,
            ID2D1SolidColorBrush **d2d1_text_brush);

HWND create_window(HINSTANCE hinstance, LONG surface_width, LONG surface_height);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

template <typename T>
inline void safe_release(T** pointer_to_object);

///////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline void safe_release(T** pointer_to_object)
{
    if (*pointer_to_object != nullptr)
    {
        (*pointer_to_object)->Release();
        (*pointer_to_object) = nullptr;
    }
}

void create_device_independent_resources(ID2D1Factory **d2d1_factory,
                                         IDWriteFactory **dwrite_factory,
                                         IDWriteTextFormat **dwrite_text_format)
{
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                     __uuidof(IDWriteFactory),
                                     reinterpret_cast<IUnknown**>(dwrite_factory));
    assert(SUCCEEDED(hr));

    static const WCHAR kFontName[] = L"Verdana";
    static const FLOAT kFontSize = 50;
    hr = (*dwrite_factory)->CreateTextFormat(kFontName,
                                             NULL,
                                             DWRITE_FONT_WEIGHT_NORMAL,
                                             DWRITE_FONT_STYLE_NORMAL,
                                             DWRITE_FONT_STRETCH_NORMAL,
                                             kFontSize,
                                             L"",
                                             dwrite_text_format);
    assert(SUCCEEDED(hr));

    (*dwrite_text_format)->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    (*dwrite_text_format)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d1_factory);
    assert(SUCCEEDED(hr));
}

HRESULT create_d3d_device(D3D_DRIVER_TYPE const kD3DDriverType,
                          ID3D11Device1 **d3d11_device,
                          ID3D11DeviceContext1 **d3d11_device_context)
{
    ID3D11Device *base_device = nullptr;
    ID3D11DeviceContext *base_device_context = nullptr;

    D3D_FEATURE_LEVEL kFeatureLevels[]{ D3D_FEATURE_LEVEL_11_0 };

    UINT kCreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG_BUILD)
    kCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(0, kD3DDriverType,
                                   0, kCreationFlags,
                                   kFeatureLevels, ARRAYSIZE(kFeatureLevels),
                                   D3D11_SDK_VERSION, &base_device,
                                   0, &base_device_context);
    assert(SUCCEEDED(hr));
    hr = base_device->QueryInterface(__uuidof(ID3D11Device1),
                                     reinterpret_cast<void**>(d3d11_device));
    assert(SUCCEEDED(hr));
    base_device->Release();
    hr = base_device_context->QueryInterface(__uuidof(ID3D11DeviceContext1),
                                             reinterpret_cast<void**>(d3d11_device_context));
    assert(SUCCEEDED(hr));
    base_device_context->Release();

#ifdef DEBUG_BUILD
    ID3D11Debug *d3d_debug = nullptr;
    (*d3d11_device)->QueryInterface(__uuidof(ID3D11Debug),
                                    reinterpret_cast<void**>(&d3d_debug));
    if (d3d_debug)
    {
        ID3D11InfoQueue *d3dInfoQueue = nullptr;
        if (SUCCEEDED(d3d_debug->QueryInterface(__uuidof(ID3D11InfoQueue),
                                                reinterpret_cast<void**>(&d3dInfoQueue))))
        {
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
            d3dInfoQueue->Release();
        }
        d3d_debug->Release();
    }
#endif

    return hr;
}

void create_device_dependent_resources(HWND hwnd,
                                       ID3D11Device1 **d3d11_device,
                                       ID3D11DeviceContext1 **d3d11_device_context,
                                       IDXGISwapChain1 **dxgi_swap_chain)
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the D3D device and context.
    ///////////////////////////////////////////////////////////////////////////////////////////

    HRESULT hr = create_d3d_device(D3D_DRIVER_TYPE_HARDWARE, d3d11_device, d3d11_device_context);
    if (hr == DXGI_ERROR_UNSUPPORTED)
        hr = create_d3d_device(D3D_DRIVER_TYPE_WARP, d3d11_device, d3d11_device_context);
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the DXGI swap chain.
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    IDXGIFactory2 *dxgi_factory = nullptr;
    {
        IDXGIDevice1 *dxgi_device = nullptr;
        hr = (*d3d11_device)->QueryInterface(__uuidof(IDXGIDevice1),
                                             reinterpret_cast<void**>(&dxgi_device));
        assert(SUCCEEDED(hr));

        IDXGIAdapter *dxgi_adapter = nullptr;
        hr = dxgi_device->GetAdapter(&dxgi_adapter);
        assert(SUCCEEDED(hr));
        dxgi_device->Release();

        DXGI_ADAPTER_DESC adapter_desc;
        dxgi_adapter->GetDesc(&adapter_desc);
        OutputDebugStringA("DXGI adapter: ");
        OutputDebugStringW(adapter_desc.Description);

        hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory2),
                                     reinterpret_cast<void**>(&dxgi_factory));
        assert(SUCCEEDED(hr));
        dxgi_adapter->Release();
    }

    DXGI_SWAP_CHAIN_DESC1 dxgi_swap_chain_desc{};
    dxgi_swap_chain_desc.Width = 0;
    dxgi_swap_chain_desc.Height = 0;
    dxgi_swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    dxgi_swap_chain_desc.SampleDesc.Count = 1;
    dxgi_swap_chain_desc.SampleDesc.Quality = 0;
    dxgi_swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgi_swap_chain_desc.BufferCount = 2;
    dxgi_swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    dxgi_swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    dxgi_swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    dxgi_swap_chain_desc.Flags = 0;

    hr = dxgi_factory->CreateSwapChainForHwnd(*d3d11_device,
                                              hwnd,
                                              &dxgi_swap_chain_desc,
                                              0, 0,
                                              dxgi_swap_chain);
    assert(SUCCEEDED(hr));
    dxgi_factory->Release();
}

void create_render_target_resources(HWND hwnd,
                                    ID3D11Device1 *d3d11_device,
                                    IDXGISwapChain1 *dxgi_swap_chain,
                                    ID2D1Factory *d2d1_factory,
                                    ID3D11RenderTargetView **d3d11_frame_buffer_view,
                                    ID2D1RenderTarget **d2d1_back_buffer_render_target,
                                    ID2D1SolidColorBrush **d2d1_text_brush)
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the D3D render targets.
    ///////////////////////////////////////////////////////////////////////////////////////////
    assert(d3d11_device != nullptr);
    assert(dxgi_swap_chain != nullptr);
    
    ID3D11Texture2D *d3d11_frame_buffer;
    HRESULT hr = dxgi_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                            reinterpret_cast<void**>(&d3d11_frame_buffer));
    assert(SUCCEEDED(hr));
    hr = d3d11_device->CreateRenderTargetView(d3d11_frame_buffer, nullptr,
                                              d3d11_frame_buffer_view);
    assert(SUCCEEDED(hr));
    d3d11_frame_buffer->Release();

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the D2D render targets.
    ///////////////////////////////////////////////////////////////////////////////////////////
    assert(hwnd);
    assert(d2d1_factory != nullptr);

    IDXGISurface *dxgi_surface = nullptr;
    hr = dxgi_swap_chain->GetBuffer(0, IID_PPV_ARGS(&dxgi_surface));
    assert(SUCCEEDED(hr));
    UINT dpi = GetDpiForWindow(hwnd);
    D2D1_RENDER_TARGET_PROPERTIES d2d1_render_target_properties =
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                                     D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN,
                                                       D2D1_ALPHA_MODE_PREMULTIPLIED),
                                     (FLOAT)dpi, (FLOAT)dpi);
    hr = d2d1_factory->CreateDxgiSurfaceRenderTarget(dxgi_surface,
                                                     &d2d1_render_target_properties,
                                                     d2d1_back_buffer_render_target);
    assert(SUCCEEDED(hr));
    dxgi_surface->Release();

    hr = (*d2d1_back_buffer_render_target)->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                                                  d2d1_text_brush);
    assert(SUCCEEDED(hr));
}

void discard_device_dependent_resources(ID3D11Device1 **d3d11_device,
                                        ID3D11DeviceContext1 **d3d11_device_context,
                                        ID3D11RenderTargetView **d3d11_frame_buffer_view,
                                        ID3D11VertexShader **vertex_shader,
                                        ID3D11PixelShader **pixel_shader,
                                        IDXGISwapChain1 **dxgi_swap_chain,
                                        ID2D1RenderTarget **d2d1_back_buffer_render_target,
                                        ID2D1SolidColorBrush **d2d1_text_brush)
{
    safe_release(d2d1_text_brush);
    safe_release(d2d1_back_buffer_render_target);
    safe_release(d3d11_frame_buffer_view);
    safe_release(dxgi_swap_chain);
    safe_release(pixel_shader);
    safe_release(vertex_shader);
    safe_release(d3d11_device_context);
    safe_release(d3d11_device);
}

void discard_render_target_resources(ID3D11DeviceContext1 **d3d11_device_context,
                                     ID3D11RenderTargetView **d3d11_frame_buffer_view,
                                     ID2D1RenderTarget **d2d1_back_buffer_render_target,
                                     ID2D1SolidColorBrush **d2d1_text_brush)
{
    (*d3d11_device_context)->OMSetRenderTargets(0, 0, 0);
    safe_release(d3d11_frame_buffer_view);
    safe_release(d2d1_back_buffer_render_target);
    safe_release(d2d1_text_brush);
}

void render(HWND hwnd,
            ID3D11DeviceContext1 *d3d11_device_context,
            ID3D11RenderTargetView **d3d11_frame_buffer_view,
            ID3D11InputLayout *input_layout,
            ID3D11VertexShader *vertex_shader,
            ID3D11PixelShader *pixel_shader,
            ID3D11Buffer **vertex_buffer,
            UINT *stride,
            UINT *offset,
            UINT nvertex,
            IDXGISwapChain1 *dxgi_swap_chain,
            ID2D1RenderTarget *d2d1_back_buffer_render_target,
            IDWriteTextFormat *dwrite_text_format,
            ID2D1SolidColorBrush *d2d1_text_brush)
{
    HRESULT hr = S_OK;    
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Render D3D content.
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    FLOAT kBackgroundColor[4]{ 0.1f, 0.2f, 0.6f, 1.0f };
    d3d11_device_context->ClearRenderTargetView(*d3d11_frame_buffer_view, kBackgroundColor);

    RECT window_rect{};
    GetClientRect(hwnd, &window_rect);
    D3D11_VIEWPORT d3d11_viewport{
        0.0f, 0.0f,
        (FLOAT)(window_rect.right - window_rect.left),
        (FLOAT)(window_rect.bottom - window_rect.top),
        0.0f, 1.0f
    };

    d3d11_device_context->RSSetViewports(1, &d3d11_viewport);
    d3d11_device_context->OMSetRenderTargets(1, d3d11_frame_buffer_view, nullptr);
    d3d11_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3d11_device_context->IASetInputLayout(input_layout);
    d3d11_device_context->VSSetShader(vertex_shader, nullptr, 0);
    d3d11_device_context->PSSetShader(pixel_shader, nullptr, 0);
    d3d11_device_context->IASetVertexBuffers(0, 1, vertex_buffer, stride, offset);
    d3d11_device_context->Draw(nvertex, 0);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Render D2D content.
    ///////////////////////////////////////////////////////////////////////////////////////////

    static const D2D1_RECT_F kTextRect{0, 0, 50, 50};
    static const wchar_t *kText = L"S";
    assert(d2d1_back_buffer_render_target != nullptr);
    d2d1_back_buffer_render_target->BeginDraw();
    d2d1_back_buffer_render_target->SetTransform(D2D1::Matrix3x2F::Identity());
    D2D1_SIZE_F surfaceSize = d2d1_back_buffer_render_target->GetSize();
    d2d1_back_buffer_render_target->DrawText(kText,
                                             (UINT32)wcslen(kText),
                                             dwrite_text_format,
                                             &kTextRect,
                                             d2d1_text_brush);
    hr = d2d1_back_buffer_render_target->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        g_device_lost = true;
        return;
    }
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////

    hr = dxgi_swap_chain->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        g_device_lost = true;
        return;
    }
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////}
}

void resize(HWND hwnd,
            ID3D11Device1 **d3d11_device,
            ID3D11DeviceContext1 **d3d11_device_context,
            IDXGISwapChain1 **dxgi_swap_chain,
            ID2D1Factory **d2d1_factory,
            ID3D11RenderTargetView **d3d11_frame_buffer_view,
            ID2D1RenderTarget **d2d1_back_buffer_render_target,
            ID2D1SolidColorBrush **d2d1_text_brush)
{
    discard_render_target_resources(d3d11_device_context,
                                    d3d11_frame_buffer_view,
                                    d2d1_back_buffer_render_target,
                                    d2d1_text_brush);
    HRESULT hr = (*dxgi_swap_chain)->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    assert(SUCCEEDED(hr));
    create_render_target_resources(hwnd,
                                   *d3d11_device,
                                   *dxgi_swap_chain,
                                   *d2d1_factory,
                                   d3d11_frame_buffer_view,
                                   d2d1_back_buffer_render_target,
                                   d2d1_text_brush);
}

bool shader_compiler_succeeded(HRESULT hr, ID3DBlob *shader_compiler_error_blob)
{
    if (FAILED(hr))
    {
        const char *error_string = nullptr;
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            error_string = "Could not compile shader; file not found.";
        }
        else if (shader_compiler_error_blob)
        {
            error_string = (const char*)shader_compiler_error_blob->GetBufferPointer();
            shader_compiler_error_blob->Release();
        }
        MessageBoxA(0, error_string, "Shader compiler error", MB_ICONERROR | MB_OK);
        return false;
    }
    return true;;
}

void create_vertex_shader(ID3D11Device *d3d11_device,
                          ID3DBlob **vs_blob,
                          ID3D11VertexShader **vertex_shader)
{
    ID3DBlob *shader_compiler_error_blob = nullptr;
    HRESULT hr = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr,
                                    "vs_main", "vs_5_0", 0, 0,
                                    vs_blob, &shader_compiler_error_blob);
    assert(shader_compiler_succeeded(hr, shader_compiler_error_blob));
    hr = d3d11_device->CreateVertexShader((*vs_blob)->GetBufferPointer(),
                                          (*vs_blob)->GetBufferSize(),
                                          nullptr, vertex_shader);
    assert(SUCCEEDED(hr));
}

void create_pixel_shader(ID3D11Device *d3d11_device,
                         ID3DBlob **ps_blob,
                         ID3D11PixelShader **pixel_shader)
{
    ID3DBlob *shader_compiler_error_blob = nullptr;
    HRESULT hr = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr,
                                    "ps_main", "ps_5_0", 0, 0,
                                    ps_blob, &shader_compiler_error_blob);
    assert(shader_compiler_succeeded(hr, shader_compiler_error_blob));
    hr = d3d11_device->CreatePixelShader((*ps_blob)->GetBufferPointer(),
                                         (*ps_blob)->GetBufferSize(),
                                         nullptr, pixel_shader);
    assert(SUCCEEDED(hr));
}

HWND create_window(HINSTANCE hinstance, LONG surface_width = 400, LONG surface_height = 300)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &WndProc;
    wc.hInstance = hinstance;
    wc.hIcon = LoadIconW(0, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(0, IDC_ARROW);
    wc.lpszClassName = L"WorldWindowClass";
    wc.hIconSm = LoadIconW(0, IDI_APPLICATION);
    if (!RegisterClassExW(&wc))
    {
        MessageBoxA(0, "RegisterClassEx failed", "ERROR", MB_OK);
        return nullptr;
    }
    RECT rect{0, 0, surface_width, surface_height};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
    LONG window_width = rect.right - rect.left;
    LONG window_height = rect.bottom - rect.top;
    HWND hwnd = nullptr;
    hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
                          wc.lpszClassName,
                          L"world",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          window_width, window_height,
                          0, 0, hinstance, 0);
    assert(hwnd != nullptr);
    return hwnd;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT lr = 0;
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
            DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        g_window_resized = true;
        break;
    default:
        lr = DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return lr;
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int)
{
    HWND hwnd = create_window(hinstance, 1024, 768);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create device-independent resources.
    ///////////////////////////////////////////////////////////////////////////////////////////

    ID2D1Factory *d2d1_factory = nullptr;
    IDWriteFactory *dwrite_factory = nullptr;
    IDWriteTextFormat *dwrite_text_format = nullptr;

    create_device_independent_resources(&d2d1_factory, &dwrite_factory, &dwrite_text_format);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the D3D device, context and swap chain.
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    ID3D11Device1 *d3d11_device = nullptr;
    ID3D11DeviceContext1* d3d11_device_context = nullptr;
    IDXGISwapChain1 *dxgi_swap_chain = nullptr;

    create_device_dependent_resources(hwnd, &d3d11_device, &d3d11_device_context, &dxgi_swap_chain);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the D3D and D2D render targets.
    ///////////////////////////////////////////////////////////////////////////////////////////

    ID3D11RenderTargetView *d3d11_frame_buffer_view = nullptr;
    ID2D1RenderTarget *d2d1_back_buffer_render_target = nullptr;
    ID2D1SolidColorBrush *d2d1_text_brush = nullptr;
    create_render_target_resources(hwnd,
                                   d3d11_device,
                                   dxgi_swap_chain,
                                   d2d1_factory,
                                   &d3d11_frame_buffer_view,
                                   &d2d1_back_buffer_render_target,
                                   &d2d1_text_brush);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create a vertex shader.
    ///////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob *vs_blob = nullptr;
    ID3D11VertexShader *vertex_shader = nullptr;
    create_vertex_shader(d3d11_device, &vs_blob, &vertex_shader);
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create a pixel shader.
    ///////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob *ps_blob = nullptr;
    ID3D11PixelShader *pixel_shader = nullptr;
    create_pixel_shader(d3d11_device, &ps_blob, &pixel_shader);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create input layout.
    ///////////////////////////////////////////////////////////////////////////////////////////

    ID3D11InputLayout *input_layout = nullptr;
    D3D11_INPUT_ELEMENT_DESC kInputElementDesc[] =
        {
            { "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
    HRESULT hr = d3d11_device->CreateInputLayout(kInputElementDesc, ARRAYSIZE(kInputElementDesc),
                                                 vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
                                                 &input_layout);
    assert(SUCCEEDED(hr));

    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create vertex buffer.
    ///////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Buffer *vertex_buffer = nullptr;
    UINT nvertex, stride, offset;
    {
        float kVertexData[]{ // x, y, r, g, b, a
            0.0f,  0.5f, 0.f, 1.f, 0.f, 1.f,
            0.5f, -0.5f, 1.f, 0.f, 0.f, 1.f,
            -0.5f, -0.5f, 0.f, 0.f, 1.f, 1.f
        };
        stride = 6 * sizeof(float);
        nvertex = sizeof(kVertexData) / stride;
        offset = 0;

        D3D11_BUFFER_DESC d3d11_vertex_buffer_desc{};
        d3d11_vertex_buffer_desc.ByteWidth = sizeof(kVertexData);
        d3d11_vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
        d3d11_vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA d3d11_vertex_subresource_data{kVertexData};

        hr = d3d11_device->CreateBuffer(&d3d11_vertex_buffer_desc,
                                        &d3d11_vertex_subresource_data,
                                        &vertex_buffer);
        assert(SUCCEEDED(hr));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Main loop.
    ///////////////////////////////////////////////////////////////////////////////////////////

    bool running = true;
    while (running)
    {
        MSG msg{};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (g_window_resized)
        {
            resize(hwnd,
                   &d3d11_device,
                   &d3d11_device_context,
                   &dxgi_swap_chain,
                   &d2d1_factory,
                   &d3d11_frame_buffer_view,
                   &d2d1_back_buffer_render_target,
                   &d2d1_text_brush);
            g_window_resized = false;
        }

        if (g_device_lost)
        {
            discard_device_dependent_resources(&d3d11_device,
                                               &d3d11_device_context,
                                               &d3d11_frame_buffer_view,
                                               &vertex_shader,
                                               &pixel_shader,
                                               &dxgi_swap_chain,
                                               &d2d1_back_buffer_render_target,
                                               &d2d1_text_brush);

            create_device_dependent_resources(hwnd,
                                              &d3d11_device,
                                              &d3d11_device_context,
                                              &dxgi_swap_chain);

            create_render_target_resources(hwnd,
                                           d3d11_device,
                                           dxgi_swap_chain,
                                           d2d1_factory,
                                           &d3d11_frame_buffer_view,
                                           &d2d1_back_buffer_render_target,
                                           &d2d1_text_brush);

            hr = d3d11_device->CreateVertexShader(vs_blob->GetBufferPointer(),
                                                  vs_blob->GetBufferSize(),
                                                  nullptr, &vertex_shader);
            assert(SUCCEEDED(hr));

            hr = d3d11_device->CreatePixelShader(ps_blob->GetBufferPointer(),
                                                 ps_blob->GetBufferSize(),
                                                 nullptr, &pixel_shader);
            assert(SUCCEEDED(hr));

            hr = d3d11_device->CreateInputLayout(kInputElementDesc, ARRAYSIZE(kInputElementDesc),
                                                 vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
                                                 &input_layout);
            assert(SUCCEEDED(hr));

            g_device_lost = false;
        }

        render(hwnd,
               d3d11_device_context,
               &d3d11_frame_buffer_view,
               input_layout,
               vertex_shader,
               pixel_shader,
               &vertex_buffer,
               &stride,
               &offset,
               nvertex,
               dxgi_swap_chain,
               d2d1_back_buffer_render_target,
               dwrite_text_format,
               d2d1_text_brush);
    }
    
    return 0;
}

﻿//
// DirectText.cpp
// Implementation of the DirectText class.
//

#include "pch.h"
#include "DWriteFallbackFont.h"

using namespace CharacterMapCX;
using namespace CharacterMapCX::Controls;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Documents;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Microsoft::Graphics::Canvas::UI;
using namespace Microsoft::Graphics::Canvas::UI::Xaml;
using namespace Windows::Graphics;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Microsoft::Graphics::Canvas::UI::Composition;

DependencyProperty^ DirectText::_FallbackFontProperty = nullptr;
DependencyProperty^ DirectText::_IsColorFontEnabledProperty = nullptr;
DependencyProperty^ DirectText::_AxisProperty = nullptr;
DependencyProperty^ DirectText::_UnicodeIndexProperty = nullptr;
DependencyProperty^ DirectText::_TextProperty = nullptr;
DependencyProperty^ DirectText::_FontFaceProperty = nullptr;
DependencyProperty^ DirectText::_TypographyProperty = nullptr;

DirectText::DirectText()
{
	DefaultStyleKey = "CharacterMapCX.Controls.DirectText";
    m_isStale = true;
}

void DirectText::OnApplyTemplate()
{
   /* if (gd == nullptr)
    {
        dpi = Display::DisplayInformation::GetForCurrentView()->LogicalDpi;
        auto device = CanvasDevice::GetSharedDevice();
        auto v = Windows::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(this);
        gd = CanvasComposition::CreateCompositionGraphicsDevice(
            v->Compositor, device);
            
        auto size = SizeInt32();
        size.Width = 2;
        size.Height = 2;
        surface = gd->CreateDrawingSurface2(
            size,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            DirectXAlphaMode::Premultiplied);
    }*/

    if (m_canvas == nullptr)
    {
        m_canvas = (CanvasControl^)GetTemplateChild("TextCanvas");
        if (m_canvas != nullptr)
        {
            m_drawToken = m_canvas->Draw +=
                ref new TypedEventHandler<CanvasControl^, CanvasDrawEventArgs^>(this, &DirectText::OnDraw);

            m_canvas->CreateResources +=
                ref new TypedEventHandler<CanvasControl^, CanvasCreateResourcesEventArgs^>(this, &DirectText::OnCreateResources);
        }
    }

    Update();
}

Windows::Foundation::Size CharacterMapCX::Controls::DirectText::MeasureOverride(Windows::Foundation::Size size)
{
    bool hasText = UnicodeIndex > 0 || FontFace != nullptr;

    if (!hasText || Typography == nullptr || m_canvas == nullptr || !m_canvas->ReadyToDraw)
        return Size(this->MinWidth, this->MinHeight);

    if (m_layout == nullptr || m_isStale)
    {
        m_isStale = false;

        if (m_layout != nullptr)
            delete m_layout;

        auto fontFace = FontFace;

        Platform::String^ text = Text;

        /* CREATE FORMAT */
        auto format = ref new CanvasTextFormat();
        format->FontFamily = FontFamily->Source;
        format->FontSize = FontSize;
        format->FontWeight = FontWeight;
        format->FontStyle = FontStyle;
        format->FontStretch = FontStretch;

        if (IsColorFontEnabled)
            format->Options = CanvasDrawTextOptions::EnableColorFont | CanvasDrawTextOptions::Clip;
        else
            format->Options = CanvasDrawTextOptions::Clip;

        /* SET FALLBACK */
        ComPtr<IDWriteTextFormat3> dformat = GetWrappedResource<IDWriteTextFormat3>(format);
        if (FallbackFont != nullptr)
            dformat->SetFontFallback(FallbackFont->Fallback.Get());

        /* SET AXIS */
        if (Axis->Size > 0)
        {
            DWRITE_FONT_AXIS_VALUE* values = new DWRITE_FONT_AXIS_VALUE[Axis->Size];
            for (int i = 0; i < Axis->Size; i++)
            {
                values[i] = Axis->GetAt(i)->GetDWriteValue();
            }

            dformat->SetFontAxisValues(values, Axis->Size);
        }

        dformat = nullptr;

        /* CREATE LAYOUT */
        auto typography = ref new CanvasTypography();
        if (Typography->Feature != CanvasTypographyFeatureName::None)
            typography->AddFeature(Typography->Feature, 1);

        auto device = m_canvas->Device;
        auto layout = ref new CanvasTextLayout(device, text, format, device->MaximumBitmapSizeInPixels, device->MaximumBitmapSizeInPixels);
        layout->SetTypography(0, layout->LineMetrics[0].CharacterCount, typography);
        if(IsColorFontEnabled)
            layout->Options = CanvasDrawTextOptions::EnableColorFont | CanvasDrawTextOptions::Clip;
        else
            layout->Options = CanvasDrawTextOptions::Clip;


        m_layout = layout;
        m_render = true;

        m_canvas->Invalidate();

        delete format;
    }

    auto dpis = m_canvas->DpiScale;
    auto dpi = m_canvas->Dpi / 96.0f;

    auto m = m_layout->Device->MaximumBitmapSizeInPixels / dpi;

    auto minh = min(m_layout->DrawBounds.Top, m_layout->LayoutBounds.Top);
    auto maxh = max(m_layout->DrawBounds.Bottom, m_layout->LayoutBounds.Bottom);

    auto minw = min(m_layout->DrawBounds.Left, m_layout->LayoutBounds.Left);
    auto maxw = max(m_layout->DrawBounds.Right, m_layout->LayoutBounds.Right);

    auto targetsize = Size(min(m, ceil(maxw - minw)), min(m, ceil(maxh - minh)));
    return targetsize;
}

void CharacterMapCX::Controls::DirectText::Render(CanvasDrawingSession^ ds)
{
    if (m_layout == nullptr)
        return;
  
    auto left = -min(m_layout->DrawBounds.Left, m_layout->LayoutBounds.Left);
    ds->DrawTextLayout(m_layout, float2(left, 0), Windows::UI::Colors::Green);
}

void DirectText::OnDraw(CanvasControl^ sender, CanvasDrawEventArgs^ args)
{
    //Render(args->DrawingSession);

    if (m_layout == nullptr)
        return;

    auto left = -min(m_layout->DrawBounds.Left, m_layout->LayoutBounds.Left);
    args->DrawingSession->DrawTextLayout(m_layout, float2(left, 0), ((SolidColorBrush^)this->Foreground)->Color);
}

void DirectText::OnCreateResources(CanvasControl^ sender, CanvasCreateResourcesEventArgs^ args)
{
    Update();
};

void CharacterMapCX::Controls::DirectText::OnLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    Update();
}

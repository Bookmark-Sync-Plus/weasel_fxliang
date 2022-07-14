﻿#include "stdafx.h"
#include "WeaselPanel.h"
#include <WeaselCommon.h>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

#include "VerticalLayout.h"
#include "HorizontalLayout.h"
#include "FullScreenLayout.h"

// for IDI_ZH, IDI_EN
#include <resource.h>

using namespace Gdiplus;
using namespace weasel;
using namespace std;
using namespace boost::algorithm;

/* start image gauss blur functions from https://github.com/kenjinote/DropShadow/  */
#define myround(x) (int)((x)+0.5)

inline void boxesForGauss(double sigma, int* sizes, int n)
{
	double wIdeal = sqrt((12 * sigma * sigma / n) + 1);
	int wl = (int)floor(wIdeal);
	if (wl % 2 == 0) --wl;

	const double wu = (double)wl + 2;

	const double mIdeal = (12 * sigma * sigma - n * (LONGLONG)wl * wl - 4 *(LONGLONG)n * wl - 3 * (LONGLONG)n) / (-4 * (LONGLONG)wl - 4);
	const int m = myround(mIdeal);

	for (int i = 0; i < n; ++i)
		sizes[i] = int(i < m ? wl : wu);
}

inline void boxBlurH_4(BYTE* scl, BYTE* tcl, int w, int h, int r, int bpp, int stride)
{
	float iarr = (float)(1. / ((LONGLONG)r + r + 1));
	for (int i = 0; i < h; ++i) {
		int ti1 = i * stride;
		int ti2 = i * stride + 1;
		int ti3 = i * stride + 2;
		int ti4 = i * stride + 3;

		int li1 = ti1;
		int li2 = ti2;
		int li3 = ti3;
		int li4 = ti4;

		int ri1 = ti1 + r * bpp;
		int ri2 = ti2 + r * bpp;
		int ri3 = ti3 + r * bpp;
		int ri4 = ti4 + r * bpp;

		int fv1 = scl[ti1];
		int fv2 = scl[ti2];
		int fv3 = scl[ti3];
		int fv4 = scl[ti4];

		int lv1 = scl[ti1 + (w - 1) * bpp];
		int lv2 = scl[ti2 + (w - 1) * bpp];
		int lv3 = scl[ti3 + (w - 1) * bpp];
		int lv4 = scl[ti4 + (w - 1) * bpp];

		int val1 = (r + 1) * fv1;
		int val2 = (r + 1) * fv2;
		int val3 = (r + 1) * fv3;
		int val4 = (r + 1) * fv4;

		for (int j = 0; j < r; ++j) {
			val1 += scl[ti1 + j * bpp];
			val2 += scl[ti2 + j * bpp];
			val3 += scl[ti3 + j * bpp];
			val4 += scl[ti4 + j * bpp];
		}

		for (int j = 0; j <= r; ++j) {
			val1 += scl[ri1] - fv1;
			val2 += scl[ri2] - fv2;
			val3 += scl[ri3] - fv3;
			val4 += scl[ri4] - fv4;

			tcl[ti1] = myround(val1 * iarr);
			tcl[ti2] = myround(val2 * iarr);
			tcl[ti3] = myround(val3 * iarr);
			tcl[ti4] = myround(val4 * iarr);

			ri1 += bpp;
			ri2 += bpp;
			ri3 += bpp;
			ri4 += bpp;

			ti1 += bpp;
			ti2 += bpp;
			ti3 += bpp;
			ti4 += bpp;
		}

		for (int j = r + 1; j < w - r; ++j) {
			val1 += scl[ri1] - scl[li1];
			val2 += scl[ri2] - scl[li2];
			val3 += scl[ri3] - scl[li3];
			val4 += scl[ri4] - scl[li4];

			tcl[ti1] = myround(val1 * iarr);
			tcl[ti2] = myround(val2 * iarr);
			tcl[ti3] = myround(val3 * iarr);
			tcl[ti4] = myround(val4 * iarr);

			ri1 += bpp;
			ri2 += bpp;
			ri3 += bpp;
			ri4 += bpp;

			li1 += bpp;
			li2 += bpp;
			li3 += bpp;
			li4 += bpp;

			ti1 += bpp;
			ti2 += bpp;
			ti3 += bpp;
			ti4 += bpp;
		}

		for (int j = w - r; j < w; ++j) {
			val1 += lv1 - scl[li1];
			val2 += lv2 - scl[li2];
			val3 += lv3 - scl[li3];
			val4 += lv4 - scl[li4];

			tcl[ti1] = myround(val1 * iarr);
			tcl[ti2] = myround(val2 * iarr);
			tcl[ti3] = myround(val3 * iarr);
			tcl[ti4] = myround(val4 * iarr);

			li1 += bpp;
			li2 += bpp;
			li3 += bpp;
			li4 += bpp;

			ti1 += bpp;
			ti2 += bpp;
			ti3 += bpp;
			ti4 += bpp;
		}
	}
}

inline void boxBlurT_4(BYTE* scl, BYTE* tcl, int w, int h, int r, int bpp, int stride)
{
	float iarr = (float)(1.0f / (r + r + 1.0f));
	for (int i = 0; i < w; ++i) {
		int ti1 = i * bpp;
		int ti2 = i * bpp + 1;
		int ti3 = i * bpp + 2;
		int ti4 = i * bpp + 3;

		int li1 = ti1;
		int li2 = ti2;
		int li3 = ti3;
		int li4 = ti4;

		int ri1 = ti1 + r * stride;
		int ri2 = ti2 + r * stride;
		int ri3 = ti3 + r * stride;
		int ri4 = ti4 + r * stride;

		int fv1 = scl[ti1];
		int fv2 = scl[ti2];
		int fv3 = scl[ti3];
		int fv4 = scl[ti4];

		int lv1 = scl[ti1 + stride * (h - 1)];
		int lv2 = scl[ti2 + stride * (h - 1)];
		int lv3 = scl[ti3 + stride * (h - 1)];
		int lv4 = scl[ti4 + stride * (h - 1)];

		int val1 = (r + 1) * fv1;
		int val2 = (r + 1) * fv2;
		int val3 = (r + 1) * fv3;
		int val4 = (r + 1) * fv4;

		for (int j = 0; j < r; ++j) {
			val1 += scl[ti1 + j * stride];
			val2 += scl[ti2 + j * stride];
			val3 += scl[ti3 + j * stride];
			val4 += scl[ti4 + j * stride];
		}

		for (int j = 0; j <= r; ++j) {
			val1 += scl[ri1] - fv1;
			val2 += scl[ri2] - fv2;
			val3 += scl[ri3] - fv3;
			val4 += scl[ri4] - fv4;

			tcl[ti1] = myround(val1 * iarr);
			tcl[ti2] = myround(val2 * iarr);
			tcl[ti3] = myround(val3 * iarr);
			tcl[ti4] = myround(val4 * iarr);

			ri1 += stride;
			ri2 += stride;
			ri3 += stride;
			ri4 += stride;

			ti1 += stride;
			ti2 += stride;
			ti3 += stride;
			ti4 += stride;
		}

		for (int j = r + 1; j < h - r; ++j) {
			val1 += scl[ri1] - scl[li1];
			val2 += scl[ri2] - scl[li2];
			val3 += scl[ri3] - scl[li3];
			val4 += scl[ri4] - scl[li4];

			tcl[ti1] = myround(val1 * iarr);
			tcl[ti2] = myround(val2 * iarr);
			tcl[ti3] = myround(val3 * iarr);
			tcl[ti4] = myround(val4 * iarr);

			li1 += stride;
			li2 += stride;
			li3 += stride;
			li4 += stride;

			ri1 += stride;
			ri2 += stride;
			ri3 += stride;
			ri4 += stride;

			ti1 += stride;
			ti2 += stride;
			ti3 += stride;
			ti4 += stride;
		}

		for (int j = h - r; j < h; ++j) {
			val1 += lv1 - scl[li1];
			val2 += lv2 - scl[li2];
			val3 += lv3 - scl[li3];
			val4 += lv4 - scl[li4];

			tcl[ti1] = myround(val1 * iarr);
			tcl[ti2] = myround(val2 * iarr);
			tcl[ti3] = myround(val3 * iarr);
			tcl[ti4] = myround(val4 * iarr);

			li1 += stride;
			li2 += stride;
			li3 += stride;
			li4 += stride;

			ti1 += stride;
			ti2 += stride;
			ti3 += stride;
			ti4 += stride;
		}
	}
}

inline void boxBlur_4(BYTE* scl, BYTE* tcl, int w, int h, int rx, int ry, int bpp, int stride)
{
	memcpy(tcl, scl, stride * h);
	boxBlurH_4(tcl, scl, w, h, rx, bpp, stride);
	boxBlurT_4(scl, tcl, w, h, ry, bpp, stride);
}

inline void gaussBlur_4(BYTE* scl, BYTE* tcl, int w, int h, float rx, float ry, int bpp, int stride)
{
	int bxsX[4];
	boxesForGauss(rx, bxsX, 4);

	int bxsY[4];
	boxesForGauss(ry, bxsY, 4);

	boxBlur_4(scl, tcl, w, h, (bxsX[0] - 1) / 2, (bxsY[0] - 1) / 2, bpp, stride);
	boxBlur_4(tcl, scl, w, h, (bxsX[1] - 1) / 2, (bxsY[1] - 1) / 2, bpp, stride);
	boxBlur_4(scl, tcl, w, h, (bxsX[2] - 1) / 2, (bxsY[2] - 1) / 2, bpp, stride);
	boxBlur_4(scl, tcl, w, h, (bxsX[3] - 1) / 2, (bxsY[3] - 1) / 2, bpp, stride);
}

void DoGaussianBlur(Gdiplus::Bitmap* img, float radiusX, float radiusY)
{
	if (img == 0 || (radiusX == 0.0f && radiusY == 0.0f)) return;

	const int w = img->GetWidth();
	const int h = img->GetHeight();

	if (radiusX > w / 2) {
		radiusX = (float)(w / 2);
	}

	if (radiusY > h / 2) {
		radiusY = (float)(h / 2);
	}

	Gdiplus::Bitmap* temp = new Gdiplus::Bitmap(img->GetWidth(), img->GetHeight(), img->GetPixelFormat());

	Gdiplus::BitmapData bitmapData1;
	Gdiplus::BitmapData bitmapData2;
	Gdiplus::Rect rect(0, 0, img->GetWidth(), img->GetHeight());

	if (Gdiplus::Ok == img->LockBits(
		&rect,
		Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
		img->GetPixelFormat(),
		&bitmapData1
	)
		&&
		Gdiplus::Ok == temp->LockBits(
			&rect,
			Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
			temp->GetPixelFormat(),
			&bitmapData2
		)) {
		BYTE* src = (BYTE*)bitmapData1.Scan0;
		BYTE* dst = (BYTE*)bitmapData2.Scan0;

		const int bpp = 4;
		const int stride = bitmapData1.Stride;

		gaussBlur_4(src, dst, w, h, radiusX, radiusY, bpp, stride);

		img->UnlockBits(&bitmapData1);
		temp->UnlockBits(&bitmapData2);
	}

	delete temp;
}

void DoGaussianBlurPower(Gdiplus::Bitmap* img, float radiusX, float radiusY, int nPower)
{
	Gdiplus::Bitmap* pBitmap = img->Clone(0, 0, img->GetWidth(), img->GetHeight(), PixelFormat32bppARGB);
	DoGaussianBlur(pBitmap, radiusX, radiusY);
	Gdiplus::Graphics g(pBitmap);
	for (int i = 0; i < 8; ++i) {
		g.DrawImage(pBitmap, 0, 0);
		if ((1 << i) & nPower) {
			Gdiplus::Graphics g(img);
			g.DrawImage(pBitmap, 0, 0);
		}
	}
	delete pBitmap;
}
/* end  image gauss blur functions from https://github.com/kenjinote/DropShadow/  */
static CRect OffsetRect(const CRect rc, int offsetx, int offsety)
{
	CRect res(rc.left + offsetx, rc.top + offsety, rc.right + offsetx, rc.bottom + offsety);
	return res;
}

 WeaselPanel::WeaselPanel(weasel::UI &ui)
	: m_layout(NULL), 
	  m_ctx(ui.ctx()), 
	  m_status(ui.status()), 
	  m_style(ui.style()),
	  _m_gdiplusToken(0)
{
	m_iconDisabled.LoadIconW(IDI_RELOAD, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconEnabled.LoadIconW(IDI_ZH, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconAlpha.LoadIconW(IDI_EN, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);

}

WeaselPanel::~WeaselPanel()
{
	if (m_layout != NULL)
		delete m_layout;
	if (pDWR != NULL)
		delete pDWR;
	if (pFonts != NULL)
		delete pFonts;
}

void WeaselPanel::_ResizeWindow()
{
	CDCHandle dc = GetDC();
	CSize size = m_layout->GetContentSize();
	int ox = 0;
	int oy = 0;
	if(m_style.shadow_color & 0xff000000 && m_style.shadow_radius != 0)
	{
		ox = abs(m_style.shadow_offset_x) + m_style.shadow_radius;
		oy = abs(m_style.shadow_offset_y) + m_style.shadow_radius;
		if((!m_style.shadow_offset_x) && (!m_style.shadow_offset_y))
		{
			ox *= 2;
			oy *= 2;
		}
	}
	size.cx += ox*2;
	size.cy += oy*2;
	SetWindowPos(NULL, 0, 0, size.cx, size.cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	ReleaseDC(dc);
}

void WeaselPanel::_CreateLayout()
{
	if (m_layout != NULL)
		delete m_layout;

	Layout* layout = NULL;
	if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL ||
		m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
	{
		layout = new VerticalLayout(m_style, m_ctx, m_status);
	}
	else if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL ||
		m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)
	{
		layout = new HorizontalLayout(m_style, m_ctx, m_status);
	}
	if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN ||
		m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)
	{
		layout = new FullScreenLayout(m_style, m_ctx, m_status, m_inputPos, layout);
	}
	m_layout = layout;
}

//更新界面
void WeaselPanel::Refresh()
{
	_CreateLayout();

	CDCHandle dc = GetDC();
	if (m_style.color_font)
		m_layout->DoLayout(dc, pDWR);
	else
		m_layout->DoLayout(dc, pFonts);
	ReleaseDC(dc);

	_ResizeWindow();
	_RepositionWindow();
	RedrawWindow();
}

void WeaselPanel::_HighlightTextEx(CDCHandle dc, CRect rc, COLORREF color, COLORREF shadowColor, int blurOffsetX, int blurOffsetY, int radius)
{
	Graphics gBack(dc);
	gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
	bool overleft = (rc.left <= bgRc.left);
	bool overright = (rc.right >= bgRc.right);
	bool overtop = (rc.top <= bgRc.top);
	bool overbottom = (rc.bottom >= bgRc.bottom);
	bool rtl = overleft && overtop;
	bool rtr = overright && overtop;
	bool rbr = overright && overbottom;
	bool rbl = overleft && overbottom;
	//	if (overleft)
	//		rc.left = bgRc.left;
	//	if (overright)
	//		rc.right = bgRc.right;
	//	if (overtop)
	//		rc.top = bgRc.top;
	//	if (overbottom)
	//		rc.bottom = bgRc.bottom;
	bool overborder = (overleft || overright || overtop || overbottom);
	// 必须shadow_color都是非完全透明色才做绘制, 全屏状态不绘制阴影保证响应速度
	if ((!overborder) && m_style.shadow_radius && (shadowColor & 0xff000000)
		&& m_style.layout_type != UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN
		&& m_style.layout_type != UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
	{
		BYTE r = GetRValue(shadowColor);
		BYTE g = GetGValue(shadowColor);
		BYTE b = GetBValue(shadowColor);
		Color brc = Color::MakeARGB((BYTE)(shadowColor >> 24), r, g, b);
		static Bitmap* pBitmapDropShadow;
		pBitmapDropShadow = new Gdiplus::Bitmap((INT)rc.Width() + blurOffsetX * 2, (INT)rc.Height() + blurOffsetY * 2, PixelFormat32bppARGB);
		Gdiplus::Graphics gg(pBitmapDropShadow);
		gg.SetSmoothingMode(SmoothingModeHighQuality);

		CRect rect(
			blurOffsetX + m_style.shadow_offset_x,
			blurOffsetY + m_style.shadow_offset_y,
			rc.Width() + blurOffsetX + m_style.shadow_offset_x,
			rc.Height() + blurOffsetY + m_style.shadow_offset_y);
		rect.InflateRect(m_style.border, m_style.border);
		if (m_style.shadow_offset_x != 0 || m_style.shadow_offset_y != 0)
		{
			GraphicsRoundRectPath path(rect, radius);
			SolidBrush br(brc);
			gg.FillPath(&br, &path);
		}
		else
		{
			int pensize = 1;
			int alpha = ((shadowColor >> 24) & 255);
			int step = alpha / m_style.shadow_radius;
			Color scolor = Color::MakeARGB(alpha, GetRValue(shadowColor), GetGValue(shadowColor), GetBValue(shadowColor));
			Pen penShadow(scolor, (Gdiplus::REAL)pensize);
			CRect rcShadowEx = rect;
			for (int i = 0; i < m_style.shadow_radius; i++)
			{
				GraphicsRoundRectPath path(rcShadowEx, radius + 1 + i);
				gg.DrawPath(&penShadow, &path);
				scolor = Color::MakeARGB(alpha - i * step, GetRValue(shadowColor), GetGValue(shadowColor), GetBValue(shadowColor));
				penShadow.SetColor(scolor);
				rcShadowEx.InflateRect(2, 2);
			}
		}
		DoGaussianBlur(pBitmapDropShadow, (float)m_style.shadow_radius, (float)m_style.shadow_radius);
		gBack.DrawImage(pBitmapDropShadow, rc.left - blurOffsetX, rc.top - blurOffsetY);
		delete pBitmapDropShadow;
	}
	if (color & 0xff000000)	// 必须back_color非完全透明才绘制
	{
		Color back_color = Color::MakeARGB((color >> 24), GetRValue(color), GetGValue(color), GetBValue(color));
		SolidBrush gBrBack(back_color);

		// shadow off, and any side out of backgroud border
		//if (((overborder && !isBgRc) || (!(shadowColor & 0xff000000))) && (rtl || rtr || rbr || rbl) )
		//if (overborder && !isBgRc)
		if ((rtl || rtr || rbr || rbl) && m_style.inline_preedit)
		{
			rc.DeflateRect(m_style.border / 2, m_style.border / 2);
			if (!overleft)
				rc.left -= m_style.border;
			if (!overright)
				rc.right += m_style.border;
			if (!overtop)
				rc.top -= m_style.border;
			if (!overbottom)
				rc.bottom += m_style.border;
			GraphicsRoundRectPath bgPath(rc, m_style.round_corner_ex, rtl, rtr, rbr, rbl);
			gBack.FillPath(&gBrBack, &bgPath);
		}
		else
		{
			//if(m_style.inline_preedit)
			{
				if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL || m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)
				{
					if (overtop)
						rc.top = bgRc.top + m_style.border / 2;
					if (overbottom)
						rc.bottom = bgRc.bottom - m_style.border / 2;
				}
				else if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL || m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
				{
					if (overleft)
						rc.left = bgRc.left + m_style.border / 2;
					if (overright)
						rc.right = bgRc.right - m_style.border / 2;
				}
			}
			GraphicsRoundRectPath bgPath(rc, radius);
			gBack.FillPath(&gBrBack, &bgPath);
		}
	}
}

void WeaselPanel::_HighlightTextBg(CDCHandle dc, CRect rc, COLORREF color, COLORREF shadowColor, int blurOffsetX, int blurOffsetY, int radius)
{
	Graphics gBack(dc);
	gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
	// 必须shadow_color都是非完全透明色才做绘制, 全屏状态不绘制阴影保证响应速度
	if (m_style.shadow_radius && (shadowColor & 0xff000000) 
		&& m_style.layout_type != UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN 
		&& m_style.layout_type != UIStyle::LAYOUT_VERTICAL_FULLSCREEN)	
	{
		BYTE r = GetRValue(shadowColor);
		BYTE g = GetGValue(shadowColor);
		BYTE b = GetBValue(shadowColor);
		Color brc = Color::MakeARGB((BYTE)(shadowColor >> 24), r, g, b);
		static Bitmap* pBitmapDropShadow;
		pBitmapDropShadow = new Gdiplus::Bitmap((INT)rc.Width() + blurOffsetX * 2, (INT)rc.Height() + blurOffsetY * 2, PixelFormat32bppARGB);
		Gdiplus::Graphics gg(pBitmapDropShadow);
		gg.SetSmoothingMode(SmoothingModeHighQuality);

		CRect rect(
				blurOffsetX + m_style.shadow_offset_x,
				blurOffsetY + m_style.shadow_offset_y, 
				rc.Width() + blurOffsetX + m_style.shadow_offset_x,
				rc.Height() + blurOffsetY + m_style.shadow_offset_y);
		if (m_style.shadow_offset_x != 0 || m_style.shadow_offset_y != 0)
		{
			GraphicsRoundRectPath path(rect, radius);
			SolidBrush br(brc);
			gg.FillPath(&br, &path);
		}
		else
		{
			int pensize = 1;
			int alpha = ((shadowColor >> 24) & 255);
			int step = alpha / m_style.shadow_radius;
			Color scolor = Color::MakeARGB(alpha, GetRValue(shadowColor), GetGValue(shadowColor), GetBValue(shadowColor));
			Pen penShadow(scolor, (Gdiplus::REAL)pensize);
			CRect rcShadowEx = rect;
			for (int i = 0; i < m_style.shadow_radius; i++)
			{
				GraphicsRoundRectPath path(rcShadowEx, radius + 1 + i);
				gg.DrawPath(&penShadow, &path);
				scolor = Color::MakeARGB(alpha - i * step, GetRValue(shadowColor), GetGValue(shadowColor), GetBValue(shadowColor));
				penShadow.SetColor(scolor);
				rcShadowEx.InflateRect(2, 2);
			}
		}
		DoGaussianBlur(pBitmapDropShadow, (float)m_style.shadow_radius, (float)m_style.shadow_radius);
		gBack.DrawImage(pBitmapDropShadow, rc.left - blurOffsetX, rc.top - blurOffsetY);
		delete pBitmapDropShadow;
	}
	if (color & 0xff000000)	// 必须back_color非完全透明才绘制
	{
		GraphicsRoundRectPath bgPath(rc, radius);
		Color back_color = Color::MakeARGB((color >> 24), GetRValue(color), GetGValue(color), GetBValue(color));
		SolidBrush gBrBack(back_color);
		gBack.FillPath(&gBrBack, &bgPath);
	}
}
bool WeaselPanel::_DrawPreedit(Text const& text, CDCHandle dc, CRect const& rc)
{
	bool drawn = false;
	std::wstring const& t = text.str;
	if (!t.empty())
	{
		weasel::TextRange range;
		std::vector<weasel::TextAttribute> const& attrs = text.attributes;
		for (size_t j = 0; j < attrs.size(); ++j)
			if (attrs[j].type == weasel::HIGHLIGHTED)
				range = attrs[j].range;

		if (range.start < range.end)
		{
			CSize selStart, selEnd;
			if (m_style.color_font)
			{
				m_layout->GetTextSizeDW(t, range.start, pDWR->pTextFormat, pDWR->pDWFactory, &selStart);
				m_layout->GetTextSizeDW(t, range.end, pDWR->pTextFormat, pDWR->pDWFactory, &selEnd);
			}
			else
			{
				long height = -MulDiv(pFonts->_TextFontPoint, dc.GetDeviceCaps(LOGPIXELSY), 72);
				CFont font;
				CFontHandle oldFont;
				font.CreateFontW(height, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, pFonts->_TextFontFace.c_str());
				oldFont = dc.SelectFont(font);
				m_layout->GetTextExtentDCMultiline(dc, t, range.start, &selStart);
				m_layout->GetTextExtentDCMultiline(dc, t, range.end, &selEnd);
				dc.SelectFont(oldFont);
				font.DeleteObject();
				oldFont.DeleteObject();
			}
			int x = rc.left;
			if (range.start > 0)
			{
				// zzz
				std::wstring str_before(t.substr(0, range.start));
				CRect rc_before(x, rc.top, rc.left + selStart.cx, rc.bottom);
				dc.SetTextColor(m_style.text_color);
				dc.SetBkColor(m_style.back_color);
				if(m_style.color_font)
					_TextOut(dc, x, rc.top, rc_before, str_before.c_str(), str_before.length(), pDWR->pTextFormat, pFonts->_TextFontPoint, pFonts->_TextFontFace);
				else
					_TextOut(dc, x, rc.top, rc_before, str_before.c_str(), str_before.length(), NULL, pFonts->_TextFontPoint, pFonts->_TextFontFace);
				x += selStart.cx + m_style.hilite_spacing;
			}
			{
				// zzz[yyy]
				std::wstring str_highlight(t.substr(range.start, range.end - range.start));
				CRect rc_hi(x, rc.top, x + (selEnd.cx - selStart.cx), rc.bottom);
				CRect rct = rc_hi;
				rc_hi.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
				OffsetRect(rc_hi, -m_style.hilite_padding, 0);
				_HighlightTextEx(dc, rc_hi, m_style.hilited_back_color, m_style.hilited_shadow_color, 
					(m_style.margin_x-m_style.hilite_padding), (m_style.margin_y - m_style.hilite_padding), m_style.round_corner);
				dc.SetTextColor(m_style.hilited_text_color);
				dc.SetBkColor(m_style.hilited_back_color);
				if(m_style.color_font) 
					_TextOut(dc, x, rc.top, rct, str_highlight.c_str(), str_highlight.length(), pDWR->pTextFormat, pFonts->_TextFontPoint, pFonts->_TextFontFace);
				else
					_TextOut(dc, x, rc.top, rct, str_highlight.c_str(), str_highlight.length(), NULL, pFonts->_TextFontPoint, pFonts->_TextFontFace);
				dc.SetTextColor(m_style.text_color);
				dc.SetBkColor(m_style.back_color);
				x += (selEnd.cx - selStart.cx);
			}
			if (range.end < static_cast<int>(t.length()))
			{
				// zzz[yyy]xxx
				x += m_style.hilite_spacing;
				std::wstring str_after(t.substr(range.end));
				CRect rc_after(x, rc.top, rc.right, rc.bottom);
				if(m_style.color_font) 
					_TextOut(dc, x, rc.top, rc_after, str_after.c_str(), str_after.length(), pDWR->pTextFormat, pFonts->_TextFontPoint, pFonts->_TextFontFace);
				else
					_TextOut(dc, x, rc.top, rc_after, str_after.c_str(), str_after.length(), NULL, pFonts->_TextFontPoint, pFonts->_TextFontFace);

			}
		}
		else
		{
			CRect rcText(rc.left, rc.top, rc.right, rc.bottom);
			if (m_style.color_font)
				_TextOut(dc, rc.left, rc.top, rcText, t.c_str(), t.length(), pDWR->pTextFormat, pFonts->_TextFontPoint, pFonts->_TextFontFace);
			else
				_TextOut(dc, rc.left, rc.top, rcText, t.c_str(), t.length(), NULL, pFonts->_TextFontPoint, pFonts->_TextFontFace);
		}
		drawn = true;
	}
	return drawn;
}

bool WeaselPanel::_DrawCandidates(CDCHandle dc)
{
	bool drawn = false;
	const std::vector<Text> &candidates(m_ctx.cinfo.candies);
	const std::vector<Text> &comments(m_ctx.cinfo.comments);
	const std::vector<Text> &labels(m_ctx.cinfo.labels);

	int ox = 0;
	int oy = 0;
	if(m_style.shadow_color & 0xff000000 && m_style.shadow_radius != 0)
	{
		ox = abs(m_style.shadow_offset_x) + m_style.shadow_radius;
		oy = abs(m_style.shadow_offset_y) + m_style.shadow_radius;
		if((!m_style.shadow_offset_x) && (!m_style.shadow_offset_y))
		{
			ox *= 2;
			oy *= 2;
		}
	}

	int bkx = abs((m_style.margin_x - m_style.hilite_padding)) + max(abs(m_style.shadow_offset_x), abs(m_style.shadow_offset_y));
	int bky = abs((m_style.margin_y - m_style.hilite_padding)) + max(abs(m_style.shadow_offset_x), abs(m_style.shadow_offset_y));

	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		CRect rect;
		if (i == m_ctx.cinfo.highlighted)
		{
			rect = OffsetRect(m_layout->GetHighlightRect(), ox, oy);
			rect.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
			_HighlightTextEx(dc, rect, m_style.hilited_candidate_back_color, m_style.hilited_candidate_shadow_color, bkx, bky, m_style.round_corner);
			dc.SetTextColor(m_style.hilited_label_text_color);
		}
		else
		{
			CRect candidateBackRect;
			if(m_style.layout_type == m_style.LAYOUT_HORIZONTAL_FULLSCREEN || m_style.layout_type == m_style.LAYOUT_HORIZONTAL)
			{
				candidateBackRect.left = m_layout->GetCandidateLabelRect(i).left;
				candidateBackRect.right = m_layout->GetCandidateCommentRect(i).right;
				candidateBackRect.top = m_layout->GetHighlightRect().top;
				candidateBackRect.bottom = m_layout->GetHighlightRect().bottom;
			}
			else
			{
				candidateBackRect.left = m_layout->GetHighlightRect().left;
				candidateBackRect.right = m_layout->GetHighlightRect().right;
				candidateBackRect.top =m_layout->GetCandidateTextRect(i).top;
				candidateBackRect.bottom = m_layout->GetCandidateTextRect(i).bottom;
			}
			candidateBackRect = OffsetRect(candidateBackRect, ox, oy);
			candidateBackRect.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
			_HighlightTextEx(dc, candidateBackRect, m_style.candidate_back_color, m_style.candidate_shadow_color, bkx, bky, m_style.round_corner);
			dc.SetTextColor(m_style.label_text_color);
		}

		// Draw label
		std::wstring label = m_layout->GetLabelText(labels, i, m_style.label_text_format.c_str());
		rect = m_layout->GetCandidateLabelRect(i);
		rect = OffsetRect(rect, ox, oy);

		if (m_style.color_font)
			_TextOut(dc, rect.left, rect.top, rect, label.c_str(), label.length(), pDWR->pLabelTextFormat, pFonts->_LabelFontPoint, pFonts->_LabelFontFace);
		else
			_TextOut(dc, rect.left, rect.top, rect, label.c_str(), label.length(), NULL, pFonts->_LabelFontPoint, pFonts->_LabelFontFace);


		// Draw text
		std::wstring text = candidates.at(i).str;
		if (i == m_ctx.cinfo.highlighted)
			dc.SetTextColor(m_style.hilited_candidate_text_color);
		else
			dc.SetTextColor(m_style.candidate_text_color);
		rect = m_layout->GetCandidateTextRect(i);
		rect = OffsetRect(rect, ox, oy);
		if (m_style.color_font)
			_TextOut(dc, rect.left, rect.top, rect, text.c_str(), text.length(), pDWR->pTextFormat, pFonts->_TextFontPoint, pFonts->_TextFontFace);
		else
			_TextOut(dc, rect.left, rect.top, rect, text.c_str(), text.length(), NULL, pFonts->_TextFontPoint, pFonts->_TextFontFace);

		
		// Draw comment
		std::wstring comment = comments.at(i).str;
		if (!comment.empty())
		{
			if (i == m_ctx.cinfo.highlighted)
				dc.SetTextColor(m_style.hilited_comment_text_color);
			else
				dc.SetTextColor(m_style.comment_text_color);
			rect = m_layout->GetCandidateCommentRect(i);
			rect = OffsetRect(rect, ox, oy);
			if(m_style.color_font)
				_TextOut(dc, rect.left, rect.top, rect, comment.c_str(), comment.length(), pDWR->pCommentTextFormat, pFonts->_CommentFontPoint, pFonts->_CommentFontFace);
			else
				_TextOut(dc, rect.left, rect.top, rect, comment.c_str(), comment.length(), NULL, pFonts->_CommentFontPoint, pFonts->_CommentFontFace);
		}
		drawn = true;
	}

	dc.SetTextColor(m_style.text_color);
	return drawn;
}

//draw client area
void WeaselPanel::DoPaint(CDCHandle dc)
{
	// background start
	CRect rc;
	GetClientRect(&rc);

	SIZE sz = { rc.right - rc.left, rc.bottom - rc.top };
	CDCHandle hdc = ::GetDC(m_hWnd);
	CDCHandle memDC = ::CreateCompatibleDC(hdc);
	HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, sz.cx, sz.cy);
	::SelectObject(memDC, memBitmap);
	
	int ox = 0;
	int oy = 0;
	if(m_style.shadow_color & 0xff000000 && m_style.shadow_radius != 0)
	{
		ox = abs(m_style.shadow_offset_x) + m_style.shadow_radius;
		oy = abs(m_style.shadow_offset_y) + m_style.shadow_radius;
		if((!m_style.shadow_offset_x) && (!m_style.shadow_offset_y))
		{
			ox *= 2;
			oy *= 2;
		}
	}

	/* inline_preedit and candidate size 1 and preedit_type preview, and hide_candidates_when_single is set */
	const std::vector<Text> &candidates(m_ctx.cinfo.candies);
	bool hide_candidates = false;
	if (m_style.hide_candidates_when_single == True 
		&& m_style.inline_preedit == True 
		&& candidates.size() == 1 )
		hide_candidates = True;

	CRect trc;
	/* (candidate not empty or (input not empty and not inline_preedit)) and not hide_candidates */
	if( (!(candidates.size()==0) 
			|| ((!m_ctx.aux.str.empty() || !m_ctx.preedit.str.empty()) && !m_style.inline_preedit)) 
		&& !hide_candidates)
	{
		Graphics gBack(memDC);
		gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
		trc = rc;
		trc.DeflateRect(ox+m_style.border, oy+m_style.border);
		bgRc = trc;
		GraphicsRoundRectPath bgPath(trc, m_style.round_corner_ex);
		int alpha = ((m_style.border_color >> 24) & 255);
		Color border_color = Color::MakeARGB(alpha, GetRValue(m_style.border_color), GetGValue(m_style.border_color), GetBValue(m_style.border_color));
		Pen gPenBorder(border_color, (Gdiplus::REAL)m_style.border);
		_HighlightTextBg(memDC, trc, m_style.back_color, m_style.shadow_color, ox * 2, oy * 2, m_style.round_corner_ex);
		if(m_style.border)
			gBack.DrawPath(&gPenBorder, &bgPath);
		//int deflate = m_style.border / 2;
		//bgRc.DeflateRect(deflate, deflate);
		gBack.ReleaseHDC(memDC);
	}
	// background end

	if (!m_style.color_font)
	{
		memDC.SetTextColor(m_style.text_color);
		memDC.SetBkColor(m_style.back_color);
		memDC.SetBkMode(TRANSPARENT);
	}
	
	bool drawn = false;

	// draw preedit string
	if (!m_layout->IsInlinePreedit() && !hide_candidates)
	{
		trc = OffsetRect(m_layout->GetPreeditRect(), ox, oy);
		drawn |= _DrawPreedit(m_ctx.preedit, memDC, trc);
	}
	
	// draw auxiliary string
	drawn |= _DrawPreedit(m_ctx.aux, memDC, m_layout->GetAuxiliaryRect());

	// status icon (I guess Metro IME stole my idea :)
	if (m_layout->ShouldDisplayStatusIcon())
	{
		const CRect iconRect(OffsetRect(m_layout->GetStatusIconRect(), ox, oy));
		CIcon& icon(m_status.disabled ? m_iconDisabled : m_status.ascii_mode ? m_iconAlpha : m_iconEnabled);
		memDC.DrawIconEx(iconRect.left, iconRect.top, icon, 0, 0);
		drawn = true;
	}

	// draw candidates
	if(!hide_candidates)
		drawn |= _DrawCandidates(memDC);

	/* Nothing drawn, hide candidate window */
	if (!drawn)
		ShowWindow(SW_HIDE);

	HDC screenDC = ::GetDC(NULL);
	CRect rect;
	GetWindowRect(&rect);
	POINT ptSrc = {
		rect.left,
		rect.top 
	};
	POINT ptDest = {
		rc.left,
		rc.top 
	};

	BLENDFUNCTION bf;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 255;
	::UpdateLayeredWindow(m_hWnd, screenDC, &ptSrc, &sz, memDC, &ptDest, RGB(0,0,0), &bf, ULW_ALPHA);
	::DeleteDC(memDC);
	::DeleteObject(memBitmap);
	ReleaseDC(screenDC);

}

LRESULT WeaselPanel::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LONG t = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
	t |= WS_EX_LAYERED;
	::SetWindowLong(m_hWnd, GWL_EXSTYLE, t);
	GdiplusStartup(&_m_gdiplusToken, &_m_gdiplusStartupInput, NULL);
	GetWindowRect(&m_inputPos);
	// windows version is depending on app running mode, if running mode lower then windows 8.1, disable color_font
	if(!IsWindows8Point1OrGreater())
		m_style.color_font = false;
	if (m_style.color_font)
	{
		pDWR = new DirectWriteResources();
		// prepare d2d1 resources
		pDWR->InitResources(m_style);
	}
	pFonts = new GDIFonts(m_style.label_font_face, m_style.label_font_point,
		m_style.font_face, m_style.font_point,
		m_style.comment_font_face, m_style.comment_font_point);
	Refresh();
	return TRUE;
}

LRESULT WeaselPanel::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	GdiplusShutdown(_m_gdiplusToken);
	return 0;
}

void WeaselPanel::CloseDialog(int nVal)
{
	
}

void WeaselPanel::MoveTo(RECT const& rc)
{
	const int distance = 6;
	m_inputPos = rc;
	m_inputPos.OffsetRect(0, distance);
	_RepositionWindow();
}

void WeaselPanel::_RepositionWindow()
{
	RECT rcWorkArea;
	//SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
	memset(&rcWorkArea, 0, sizeof(rcWorkArea));
	HMONITOR hMonitor = MonitorFromRect(m_inputPos, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &info))
		{
			rcWorkArea = info.rcWork;
		}
	}
	RECT rcWindow;
	GetWindowRect(&rcWindow);
	int width = (rcWindow.right - rcWindow.left);
	int height = (rcWindow.bottom - rcWindow.top);
	// keep panel visible
	rcWorkArea.right -= width;
	rcWorkArea.bottom -= height;
	int x = m_inputPos.left;
	int y = m_inputPos.bottom;
	if (x > rcWorkArea.right)
		x = rcWorkArea.right;
	if (x < rcWorkArea.left)
		x = rcWorkArea.left;
	// show panel above the input focus if we're around the bottom
	if (y > rcWorkArea.bottom)
		y = m_inputPos.top - height;
	if (y > rcWorkArea.bottom)
		y = rcWorkArea.bottom;
	if (y < rcWorkArea.top)
		y = rcWorkArea.top;
	// memorize adjusted position (to avoid window bouncing on height change)
	if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN || m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
	{
		if(m_style.shadow_color & 0xff000000 && m_style.shadow_radius != 0)
		{
			x -= abs(m_style.shadow_offset_x) + m_style.shadow_radius;
			y -= abs(m_style.shadow_offset_y) + m_style.shadow_radius;
		}
	}
	else 
	{
		int ox = 0;
		int oy = 0;
		if(m_style.shadow_color & 0xff000000 && m_style.shadow_radius != 0)
		{
			if(!m_style.inline_preedit)
			{
				if(m_style.shadow_offset_x == 0 && m_style.shadow_offset_y == 0)
				{
					ox = oy = m_style.shadow_radius;
				}
				else
				{
					ox = (m_style.shadow_offset_x < 0) ? m_style.shadow_radius : (m_style.shadow_offset_x + m_style.shadow_radius);
					oy = (m_style.shadow_offset_y < 0) ? m_style.shadow_radius : (m_style.shadow_offset_y + m_style.shadow_radius);
				}
			}
			x -= ox;
			y -= oy;
		}
	}

	m_inputPos.bottom = y;
	SetWindowPos(HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
}

static HRESULT _TextOutWithFallback(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch)
{
    SCRIPT_STRING_ANALYSIS ssa;
    HRESULT hr;

    hr = ScriptStringAnalyse(
        dc,
        psz, cch,
        2 * cch + 16,
        -1,
        SSA_GLYPHS|SSA_FALLBACK|SSA_LINK,
        0,
        NULL, // control
        NULL, // state
        NULL, // piDx
        NULL,
        NULL, // pbInClass
        &ssa);

    if (SUCCEEDED(hr))
    {
        hr = ScriptStringOut(
            ssa, x, y, 0,
            &rc,
            0, 0, FALSE);
    }

	hr = ScriptStringFree(&ssa);
	return hr;
}

HBITMAP WeaselPanel::_CreateAlphaTextBitmap(LPCWSTR inText, HFONT inFont, COLORREF inColor, int cch)
{
	HDC hTextDC = CreateCompatibleDC(NULL);
	HFONT hOldFont = (HFONT)SelectObject(hTextDC, inFont);
	HBITMAP hMyDIB = NULL;
	// get text area
	RECT TextArea = { 0, 0, 0, 0 };
	DrawText(hTextDC, inText, cch, &TextArea, DT_CALCRECT);
	if ((TextArea.right > TextArea.left) && (TextArea.bottom > TextArea.top))
	{
		BITMAPINFOHEADER BMIH;
		memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
		void* pvBits = NULL;
		// dib setup
		BMIH.biSize = sizeof(BMIH);
		BMIH.biWidth = TextArea.right - TextArea.left;
		BMIH.biHeight = TextArea.bottom - TextArea.top;
		BMIH.biPlanes = 1;
		BMIH.biBitCount = 32;
		BMIH.biCompression = BI_RGB;

		// create and select dib into dc
		hMyDIB = CreateDIBSection(hTextDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0);
		HBITMAP hOldBMP = (HBITMAP)SelectObject(hTextDC, hMyDIB);
		if (hOldBMP != NULL)
		{
			SetTextColor(hTextDC, 0x00FFFFFF);
			SetBkColor(hTextDC, 0x00000000);
			SetBkMode(hTextDC, OPAQUE);
			// draw text to buffer
			DrawText(hTextDC, inText, cch, &TextArea, DT_NOCLIP);
			BYTE* DataPtr = (BYTE*)pvBits;
			BYTE FillR = GetRValue(inColor);
			BYTE FillG = GetGValue(inColor);
			BYTE FillB = GetBValue(inColor);
			BYTE ThisA;

			for (int LoopY = 0; LoopY < BMIH.biHeight; LoopY++)
			{
				for (int LoopX = 0; LoopX < BMIH.biWidth; LoopX++)
				{
					ThisA = *DataPtr; // move alpha and premutiply with rgb
					*DataPtr++ = (FillB * ThisA) >> 8;
					*DataPtr++ = (FillG * ThisA) >> 8;
					*DataPtr++ = (FillR * ThisA) >> 8;
					*DataPtr++ = ThisA;
				}
			}
			SelectObject(hTextDC, hOldBMP);
		}
	}
	SelectObject(hTextDC, hOldFont);
	DeleteDC(hTextDC);
	return hMyDIB;
}

static HRESULT _TextOutWithFallback_ULW(CDCHandle dc, int x, int y, CRect const rc, LPCWSTR psz, int cch, long height, std::wstring fontface)
{
    SCRIPT_STRING_ANALYSIS ssa;
    HRESULT hr;
	CFont font;
	font.CreateFontW(height, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, fontface.c_str());
	int TextLength = cch;
	HDC hTextDC = CreateCompatibleDC(NULL);
	HFONT hOldFont = (HFONT)SelectObject(hTextDC, font);
	HBITMAP MyBMP = NULL;

    hr = ScriptStringAnalyse(
        hTextDC,
        psz, cch,
        2 * cch + 16,
        -1,
        SSA_GLYPHS|SSA_FALLBACK|SSA_LINK,
        0,
        NULL, // control
        NULL, // state
        NULL, // piDx
        NULL,
        NULL, // pbInClass
        &ssa);

    if (SUCCEEDED(hr))
    {

		BITMAPINFOHEADER BMIH;
		memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
		void* pvBits = NULL;
		BMIH.biSize = sizeof(BMIH);
		BMIH.biWidth = rc.right - rc.left;
		BMIH.biHeight = rc.bottom - rc.top;
		BMIH.biPlanes = 1;
		BMIH.biBitCount = 32;
		BMIH.biCompression = BI_RGB;
		MyBMP = CreateDIBSection(hTextDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0);
		HBITMAP hOldBMP;
		if(MyBMP)
			hOldBMP = (HBITMAP)SelectObject(hTextDC, MyBMP);
		COLORREF inColor = dc.GetTextColor();
		BYTE alpha = (inColor >> 24) & 255 ;
		if (hOldBMP != NULL)
		{
			SetTextColor(hTextDC, 0x00FFFFFF);
			SetBkColor(hTextDC, 0x00000000);
			SetBkMode(hTextDC, OPAQUE);
			// draw text to buffer
			hr = ScriptStringOut(ssa, 0, 0, 0, rc, 0, 0, FALSE);
			BYTE* DataPtr = (BYTE*)pvBits;
			BYTE FillR = GetRValue(inColor);
			BYTE FillG = GetGValue(inColor);
			BYTE FillB = GetBValue(inColor);
			BYTE ThisA;
			for (int LoopY = 0; LoopY < BMIH.biHeight; LoopY++)
			{
				for (int LoopX = 0; LoopX < BMIH.biWidth; LoopX++)
				{
					ThisA = *DataPtr; // move alpha and premutiply with rgb
					*DataPtr++ = (FillB * ThisA) >> 8;
					*DataPtr++ = (FillG * ThisA) >> 8;
					*DataPtr++ = (FillR * ThisA) >> 8;
					*DataPtr++ = ThisA;
				}
			}
			SelectObject(hTextDC, hOldBMP);
		}
		SelectObject(hTextDC, hOldFont);
		DeleteDC(hTextDC);
		DeleteObject(font);
		if (MyBMP)
		{
			// temporary dc select bmp into it
			HDC hTempDC = CreateCompatibleDC(dc);
			HBITMAP hOldBMP = (HBITMAP)SelectObject(hTempDC, MyBMP);
			if (hOldBMP)
			{
				BITMAP BMInf;
				GetObject(MyBMP, sizeof(BITMAP), &BMInf);
				// fill blend function and blend new text to window
				BLENDFUNCTION bf;
				bf.BlendOp = AC_SRC_OVER;
				bf.BlendFlags = 0;
				bf.SourceConstantAlpha = alpha;
				bf.AlphaFormat = AC_SRC_ALPHA;
				AlphaBlend(dc, x, y, BMInf.bmWidth, BMInf.bmHeight, hTempDC, 0, 0, BMInf.bmWidth, BMInf.bmHeight, bf);
				// clean up
				SelectObject(hTempDC, hOldBMP);
				DeleteObject(MyBMP);
				DeleteDC(hTempDC);
			}
		}
    }

	hr = ScriptStringFree(&ssa);
	return hr;
}

HRESULT WeaselPanel::_TextOutWithFallback_D2D (CDCHandle dc, CRect const rc, wstring psz, int cch, COLORREF gdiColor, IDWriteTextFormat* pTextFormat)
{
	float r = (float)(GetRValue(gdiColor))/255.0f;
	float g = (float)(GetGValue(gdiColor))/255.0f;
	float b = (float)(GetBValue(gdiColor))/255.0f;

	// alpha 
	float alpha = (float)((gdiColor >> 24) & 255) / 255.0f;
	ID2D1SolidColorBrush* pBrush = NULL;
	pDWR->pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(r, g, b, alpha), &pBrush);
	if (NULL != pBrush && NULL != pDWR->pTextFormat)
	{
		IDWriteTextLayout* pTextLayout = NULL;
		if (pTextFormat == NULL)
			pTextFormat = pDWR->pTextFormat;
		pDWR->pDWFactory->CreateTextLayout( ((wstring)psz).c_str(), ((wstring)psz).size(), pTextFormat, rc.Width(), rc.Height(), &pTextLayout);
		float offsetx = 0;
		float offsety = 0;
		pDWR->pRenderTarget->BindDC(dc, &rc);
		pDWR->pRenderTarget->BeginDraw();
		if (pTextLayout != NULL)
			pDWR->pRenderTarget->DrawTextLayout({ offsetx, offsety}, pTextLayout, pBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
		//D2D1_RECT_F rectf{ 0,0, rc.Width(), rc.Height() };
		//pDWR->pRenderTarget->DrawRectangle(&rectf, pBrush);
		pDWR->pRenderTarget->EndDraw();
		SafeRelease(&pTextLayout);
	}
	pBrush->Release();
	return S_OK;
}

void WeaselPanel::_TextOut(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch, IDWriteTextFormat* pTextFormat, int font_point, std::wstring font_face)
{
	if (m_style.color_font )
	{
		_TextOutWithFallback_D2D(dc, rc, psz, cch, dc.GetTextColor(), pTextFormat);
	}
	else
	{ 
		long height = -MulDiv(font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);
		std::vector<std::wstring> lines;
		split(lines, psz, is_any_of(L"\r"));
		int offset = 0;
		for (wstring line : lines)
		{
			CSize size;
			CFont font;
			font.CreateFontW(height, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, font_face.c_str());
			dc.SelectFont(font);
			dc.GetTextExtent(line.c_str(), line.length(), &size);
			if (FAILED(_TextOutWithFallback_ULW(dc, x, y+offset, rc, line.c_str(), line.length(), height, font_face ))) 
			{
				//CFont font;
				//font.CreateFontW(height, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());

				HBITMAP MyBMP = _CreateAlphaTextBitmap(psz, font, dc.GetTextColor(), cch);
				DeleteObject(font);
				if (MyBMP)
				{
					BYTE alpha = (BYTE)((dc.GetTextColor() >> 24) & 255) ;
					// temporary dc select bmp into it
					HDC hTempDC = CreateCompatibleDC(dc);
					HBITMAP hOldBMP = (HBITMAP)SelectObject(hTempDC, MyBMP);
					if (hOldBMP)
					{
						BITMAP BMInf;
						GetObject(MyBMP, sizeof(BITMAP), &BMInf);
						// fill blend function and blend new text to window
						BLENDFUNCTION bf;
						bf.BlendOp = AC_SRC_OVER;
						bf.BlendFlags = 0;
						bf.SourceConstantAlpha = alpha;
						bf.AlphaFormat = AC_SRC_ALPHA;
						AlphaBlend(dc, x, y, BMInf.bmWidth, BMInf.bmHeight, hTempDC, 0, 0, BMInf.bmWidth, BMInf.bmHeight, bf);
						// clean up
						SelectObject(hTempDC, hOldBMP);
						DeleteObject(MyBMP);
						DeleteDC(hTempDC);
					}
				}
			}
			offset += size.cy;
		}
	}
}

GraphicsRoundRectPath::GraphicsRoundRectPath(void) : Gdiplus::GraphicsPath()
{

}
GraphicsRoundRectPath::GraphicsRoundRectPath(int left, int top, int width, int height, int cornerx, int cornery) 
	: Gdiplus::GraphicsPath()
{
	AddRoundRect(left, top, width, height, cornerx, cornery);
}
GraphicsRoundRectPath::GraphicsRoundRectPath(const CRect rc, int corner)
{
	AddRoundRect(rc.left, rc.top, rc.Width(), rc.Height(), corner, corner);
}
GraphicsRoundRectPath::GraphicsRoundRectPath(const CRect rc, int corner, bool rtl, bool rtr, bool rbr, bool rbl)
{
	if (!(rtl || rtr || rbr || rbl))
	{
		Rect& rcp = Rect(rc.left, rc.top, rc.Width(), rc.Height());
		AddRectangle(rcp);
	}
	else
	{
		int cnx = ((corner*2 <= rc.Width()) ? corner : (rc.Width()/2));
		int cny = ((corner*2 <= rc.Height()) ? corner : (rc.Height()/2));
		int elWid = 2 * cnx;
		int elHei = 2 * cny;
		AddArc(rc.left, rc.top, elWid * rtl, elHei * rtl, 180, 90);
		AddLine(rc.left + cnx * rtl, rc.top, rc.right - cnx * rtr, rc.top);

		AddArc(rc.right - elWid * rtr, rc.top, elWid * rtr, elHei * rtr, 270, 90);
		AddLine(rc.right, rc.top + cny * rtr, rc.right, rc.bottom - cny * rbr);

		AddArc(rc.right - elWid * rbr, rc.bottom - elHei * rbr, elWid * rbr, elHei * rbr, 0, 90);
		AddLine(rc.right - cnx * rbr, rc.bottom, rc.left + cnx * rbl, rc.bottom);

		AddArc(rc.left, rc.bottom - elHei * rbl, elWid * rbl, elHei * rbl, 90, 90);
		AddLine(rc.left, rc.top + cny * rtl, rc.left, rc.bottom - cny * rbl);
	}
}
void GraphicsRoundRectPath::AddRoundRect(int left, int top, int width, int height, int cornerx, int cornery)
{
	if(cornery > 0 && cornerx >0)
	{
		int cnx = ((cornerx*2 <= width) ? cornerx : (width/2));
		int cny = ((cornery*2 <= height) ? cornery : (height/2));
		int elWid = 2 * cnx;
		int elHei = 2 * cny;

		AddArc(left, top, elWid, elHei, 180, 90);
		AddLine(left + cnx , top, left + width - cnx , top);

		AddArc(left + width - elWid, top, elWid, elHei, 270, 90);
		AddLine(left + width, top + cny, left + width, top + height - cny);

		AddArc(left + width - elWid, top + height - elHei, elWid, elHei, 0, 90);
		AddLine(left + width - cnx, top + height, left + cnx, top + height);

		AddArc(left, top + height - elHei, elWid, elHei, 90, 90);
		AddLine(left, top + cny, left, top + height - cny);
	}
	else
	{
		Gdiplus::Rect& rc = Rect(left, top, width, height);
		AddRectangle(rc);
	}
}

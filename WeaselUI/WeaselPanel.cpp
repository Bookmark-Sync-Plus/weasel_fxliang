﻿#include "stdafx.h"
#include "WeaselPanel.h"
#include <WeaselCommon.h>
#include <Usp10.h>

#include "VerticalLayout.h"
#include "HorizontalLayout.h"
#include "FullScreenLayout.h"

// for IDI_ZH, IDI_EN
#include <resource.h>

using namespace Gdiplus;
using namespace weasel;

WeaselPanel::WeaselPanel(weasel::UI &ui)
	: m_layout(NULL), 
	  m_ctx(ui.ctx()), 
	  m_status(ui.status()), 
	  m_style(ui.style())
{
	m_iconDisabled.LoadIconW(IDI_RELOAD, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconEnabled.LoadIconW(IDI_ZH, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconAlpha.LoadIconW(IDI_EN, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
}

WeaselPanel::~WeaselPanel()
{
	if (m_layout != NULL)
		delete m_layout;
}

void WeaselPanel::_ResizeWindow()
{
	CDCHandle dc = GetDC();
	long fontHeight = -MulDiv(m_style.font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);
	CFont font;
	font.CreateFontW(fontHeight, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());
	dc.SelectFont(font);

	CSize size = m_layout->GetContentSize();
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
	long fontHeight = -MulDiv(m_style.font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);
	CFont font;
	font.CreateFontW(fontHeight, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());
	dc.SelectFont(font);
	m_layout->DoLayout(dc);
	ReleaseDC(dc);

	_ResizeWindow();
	_RepositionWindow();
	RedrawWindow();
}

void WeaselPanel::_HighlightText(CDCHandle dc, CRect rc, COLORREF color)
{
	rc.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
	{
		Graphics gBack(dc);
		gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
		GraphicsRoundRectPath bgPath;
		bgPath.AddRoundRect(rc.left, rc.top, rc.Width(), rc.Height(), m_style.round_corner, m_style.round_corner);
		Color border_color(GetRValue(color), GetGValue(color), GetBValue(color));
		Color back_color(GetRValue(color), GetGValue(color), GetBValue(color));
		SolidBrush gBrBack(back_color);
		Pen gPenBorder(border_color, 0);
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
			dc.GetTextExtent(t.c_str(), range.start, &selStart);
			dc.GetTextExtent(t.c_str(), range.end, &selEnd);
			int x = rc.left;
			if (range.start > 0)
			{
				// zzz
				std::wstring str_before(t.substr(0, range.start));
				CRect rc_before(x, rc.top, rc.left + selStart.cx, rc.bottom);
				_TextOut(dc, x, rc.top, rc_before, str_before.c_str(), str_before.length());
				x += selStart.cx + m_style.hilite_spacing;
			}
			{
				// zzz[yyy]
				std::wstring str_highlight(t.substr(range.start, range.end - range.start));
				CRect rc_hi(x, rc.top, x + (selEnd.cx - selStart.cx), rc.bottom);
				_HighlightText(dc, rc_hi, m_style.hilited_back_color);
				dc.SetTextColor(m_style.hilited_text_color);
				dc.SetBkColor(m_style.hilited_back_color);
				_TextOut(dc, x, rc.top, rc_hi, str_highlight.c_str(), str_highlight.length());
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
				_TextOut(dc, x, rc.top, rc_after, str_after.c_str(), str_after.length());
			}
		}
		else
		{
			CRect rcText(rc.left, rc.top, rc.right, rc.bottom);
			_TextOut(dc, rc.left, rc.top, rcText, t.c_str(), t.length());
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

	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		CRect rect;
		if (i == m_ctx.cinfo.highlighted)
		{
			_HighlightText(dc, m_layout->GetHighlightRect(), m_style.hilited_candidate_back_color);
			dc.SetTextColor(m_style.hilited_label_text_color);
		}
		else
			dc.SetTextColor(m_style.label_text_color);

		// Draw label
		std::wstring label = m_layout->GetLabelText(labels, i, m_style.label_text_format.c_str());
		rect = m_layout->GetCandidateLabelRect(i);
		_TextOut(dc, rect.left, rect.top, rect, label.c_str(), label.length());

		// Draw text
		std::wstring text = candidates.at(i).str;
		if (i == m_ctx.cinfo.highlighted)
			dc.SetTextColor(m_style.hilited_candidate_text_color);
		else
			dc.SetTextColor(m_style.candidate_text_color);
		rect = m_layout->GetCandidateTextRect(i);
		_TextOut(dc, rect.left, rect.top, rect, text.c_str(), text.length());
		
		// Draw comment
		std::wstring comment = comments.at(i).str;
		if (!comment.empty())
		{
			if (i == m_ctx.cinfo.highlighted)
				dc.SetTextColor(m_style.hilited_comment_text_color);
			else
				dc.SetTextColor(m_style.comment_text_color);
			rect = m_layout->GetCandidateCommentRect(i);
			_TextOut(dc, rect.left, rect.top, rect, comment.c_str(), comment.length());
		}
		drawn = true;
	}
	dc.SetTextColor(m_style.text_color);
	return drawn;
}

//draw client area
void WeaselPanel::DoPaint(CDCHandle dc)
{
	CRect rc;
	GetClientRect(&rc);
	LONG t = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
	t |= WS_EX_LAYERED;
	::SetWindowLong(m_hWnd, GWL_EXSTYLE, t);
	int alpha = (m_style.back_color >> 24) & 255 ? (m_style.back_color >> 24) : 255;
	SetLayeredWindowAttributes(m_hWnd, RGB(0,0,0), alpha, LWA_ALPHA);
	// background
	{
		CRgn rgn;
		CPoint point(m_style.round_corner, m_style.round_corner);
		Graphics gBack(dc);
		gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
		GraphicsRoundRectPath bgPath;
		
		// 坐标修正，起点要-1，终点要+1
		bgPath.AddRoundRect(rc.left+m_style.border/2 -1, rc.top+m_style.border/2 - 1, rc.Width()-m_style.border+1, rc.Height()-m_style.border+1, m_style.round_corner, m_style.round_corner);

		Color border_color(GetRValue(m_style.border_color), GetGValue(m_style.border_color), GetBValue(m_style.border_color));
		Color back_color(GetRValue(m_style.back_color), GetGValue(m_style.back_color), GetBValue(m_style.back_color));
		SolidBrush gBrBack(back_color);
		Pen gPenBorder(border_color, m_style.border*2);

		gBack.DrawPath(&gPenBorder, &bgPath);
		gBack.FillPath(&gBrBack, &bgPath);
		// 坐标修正
		rgn.CreateRoundRectRgn(rc.left, rc.top,
				rc.right+1, rc.bottom+1, point.x*2+1, point.y*2+1);
		::SetWindowRgn(m_hWnd, rgn, true);
		rgn.DeleteObject();
	}

	long height = -MulDiv(m_style.font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);

	CFont font;
	font.CreateFontW(height, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());
	CFontHandle oldFont = dc.SelectFont(font);

	dc.SetTextColor(m_style.text_color);
	dc.SetBkColor(m_style.back_color);
	dc.SetBkMode(TRANSPARENT);
	
	bool drawn = false;

	// draw preedit string
	if (!m_layout->IsInlinePreedit())
		drawn |= _DrawPreedit(m_ctx.preedit, dc, m_layout->GetPreeditRect());
	
	// draw auxiliary string
	drawn |= _DrawPreedit(m_ctx.aux, dc, m_layout->GetAuxiliaryRect());

	// status icon (I guess Metro IME stole my idea :)
	if (m_layout->ShouldDisplayStatusIcon())
	{
		const CRect iconRect(m_layout->GetStatusIconRect());
		CIcon& icon(m_status.disabled ? m_iconDisabled : m_status.ascii_mode ? m_iconAlpha : m_iconEnabled);
		dc.DrawIconEx(iconRect.left, iconRect.top, icon, 0, 0);
		drawn = true;
	}

	// draw candidates
	drawn |= _DrawCandidates(dc);

	/* Nothing drawn, hide candidate window */
	if (!drawn)
		ShowWindow(SW_HIDE);

	dc.SelectFont(oldFont);	
}

LRESULT WeaselPanel::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	Refresh();
	//CenterWindow();
	GdiplusStartup(&_m_gdiplusToken, &_m_gdiplusStartupInput, NULL);
	GetWindowRect(&m_inputPos);

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

	ScriptStringFree(&ssa);
	return hr;
}

void WeaselPanel::_TextOut(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch)
{
	if (FAILED(_TextOutWithFallback(dc, x, y, rc, psz, cch))) {
		dc.TextOutW(x, y, psz, cch);
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

void GraphicsRoundRectPath::AddRoundRect(int left, int top, int width, int height, int cornerx, int cornery)
{
	if(cornery > 0 && cornerx >0)
	{
		int elWid = 2 * cornerx;
		int elHei = 2 * cornery;
		AddArc(left, top, elWid, elHei, 180, 90);
		AddLine(left + cornerx, top, left + width - cornerx, top);

		AddArc(left + width - elWid, top, elWid, elHei, 270, 90);
		AddLine(left + width, top + cornery, left + width, top + height - cornery);

		AddArc(left + width - elWid, top + height - elHei, elWid, elHei, 0, 90);
		AddLine(left + width - cornerx, top + height, left + cornerx, top + height);

		AddArc(left, top + height - elHei, elWid, elHei, 90, 90);
		AddLine(left, top + cornery, left, top + height - cornery);
	}
	else
	{
		AddLine(left, top, left + width - cornerx, top);
		AddLine(left + width, top, left + width, top + height);
		AddLine(left + width, top + height, left, top + height);
		AddLine(left, top, left, top + height);
	}
}

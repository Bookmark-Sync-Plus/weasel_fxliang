#include "stdafx.h"
#include "VerticalLayout.h"

using namespace weasel;

VerticalLayout::VerticalLayout(const UIStyle &style, const Context &context, const Status &status)
	: StandardLayout(style, context, status)
{
}

void VerticalLayout::DoLayout(CDCHandle dc, GDIFonts* pFonts)
{
	const std::vector<Text> &candidates(_context.cinfo.candies);
	const std::vector<Text> &comments(_context.cinfo.comments);
	const std::vector<Text> &labels(_context.cinfo.labels);
	CSize size;
	//const int space = size.cx / 4;
	const int space = _style.hilite_spacing;
	int width = 0, height = _style.margin_y;
	CFont labelFont, textFont, commentFont;
	CFontHandle oldFont;
	long hlabel = -MulDiv(pFonts->m_LabelFont.m_FontPoint, dc.GetDeviceCaps(LOGPIXELSY), 72);
	long htext = -MulDiv(pFonts->m_TextFont.m_FontPoint, dc.GetDeviceCaps(LOGPIXELSY), 72);
	long hcmmt = -MulDiv(pFonts->m_CommentFont.m_FontPoint, dc.GetDeviceCaps(LOGPIXELSY), 72);

	labelFont.CreateFontW(hlabel, 0, 0, 0, pFonts->m_LabelFont.m_FontWeight, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, pFonts->m_LabelFont.m_FontFace.c_str());
	textFont.CreateFontW(htext, 0, 0, 0, pFonts->m_TextFont.m_FontWeight, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, pFonts->m_TextFont.m_FontFace.c_str());
	commentFont.CreateFontW(hcmmt, 0, 0, 0, pFonts->m_CommentFont.m_FontWeight, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, pFonts->m_CommentFont.m_FontFace.c_str());
	/* Preedit */
	oldFont = dc.SelectFont(textFont);
	if (!IsInlinePreedit() && !_context.preedit.str.empty())
	{
		size = GetPreeditSize(dc);
		_preeditRect.SetRect(_style.margin_x, height, _style.margin_x + size.cx, height + size.cy);
		_preeditRect.OffsetRect(offsetX, offsetY);
		width = max(width, _style.margin_x + size.cx + _style.margin_x);
		height += size.cy + _style.spacing;
	}

	/* Auxiliary */
	if (!_context.aux.str.empty())
	{
		GetTextExtentDCMultiline(dc, _context.aux.str, _context.aux.str.length(), &size);
		_auxiliaryRect.SetRect(_style.margin_x, height, _style.margin_x + size.cx, height + size.cy);
		width = max(width, _style.margin_x + size.cx + _style.margin_x);
		height += size.cy + _style.spacing;
	}

	/* Candidates */
	int comment_shift_width = 0;  /* distance to the left of the candidate text */
	int max_candidate_width = 0;  /* label + text */
	int max_comment_width = 0;    /* comment, or none */
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		if (i > 0 )
			height += _style.candidate_spacing;

		int w = _style.margin_x, h = 0;
		int candidate_width = 0, comment_width = 0;
		/* Label */
		oldFont = dc.SelectFont(labelFont);
		std::wstring label = GetLabelText(labels, i, _style.label_text_format.c_str());
		GetTextExtentDCMultiline(dc, label, label.length(), &size);
		_candidateLabelRects[i].SetRect(w, height, w + size.cx * ((int)(pFonts->m_LabelFont.m_FontPoint > 0)), height + size.cy);
		_candidateLabelRects[i].OffsetRect(offsetX, offsetY);
		w += (size.cx + space) * ((int)(pFonts->m_LabelFont.m_FontPoint > 0)), h = max(h, size.cy);
		candidate_width += (size.cx + space) * ((int)(pFonts->m_LabelFont.m_FontPoint > 0));

		/* Text */
		oldFont = dc.SelectFont(textFont);
		const std::wstring& text = candidates.at(i).str;
		GetTextExtentDCMultiline(dc, text, text.length(), &size);
		_candidateTextRects[i].SetRect(w, height, w + size.cx * ((int)(pFonts->m_TextFont.m_FontPoint > 0)), height + size.cy);
		_candidateTextRects[i].OffsetRect(offsetX, offsetY);
		w += size.cx * ((int)(pFonts->m_LabelFont.m_FontPoint > 0)), h = max(h, size.cy);
		candidate_width += size.cx * ((int)(pFonts->m_TextFont.m_FontPoint > 0));
		max_candidate_width = max(max_candidate_width, candidate_width);

		/* Comment */
		oldFont = dc.SelectFont(commentFont);
		if (!comments.at(i).str.empty() && pFonts->m_CommentFont.m_FontPoint > 0)
		{
			w += space;
			comment_shift_width = max(comment_shift_width, w - _style.margin_x);

			const std::wstring& comment = comments.at(i).str;
			GetTextExtentDCMultiline(dc, comment, comment.length(), &size);
			_candidateCommentRects[i].SetRect(0, height, size.cx, height + size.cy);
			_candidateCommentRects[i].OffsetRect(offsetX, offsetY);
			w += size.cx, h = max(h, size.cy);
			comment_width += size.cx;
			max_comment_width = max(max_comment_width, comment_width);
		}
		int ol = 0, ot = 0, oc = 0;
		if (_style.align_type == UIStyle::ALIGN_CENTER)
		{
			ol = (h - _candidateLabelRects[i].Height()) / 2;
			ot = (h - _candidateTextRects[i].Height()) / 2;
			oc = (h - _candidateCommentRects[i].Height()) / 2;
		}
		else if (_style.align_type == UIStyle::ALIGN_BOTTOM)
		{
			ol = (h - _candidateLabelRects[i].Height()) ;
			ot = (h - _candidateTextRects[i].Height()) ;
			oc = (h - _candidateCommentRects[i].Height()) ;

		}
		_candidateLabelRects[i].OffsetRect(0, ol);
		_candidateTextRects[i].OffsetRect(0, ot);
		_candidateCommentRects[i].OffsetRect(0, oc);
		//w += margin;
		//width = max(width, w);
		height += h;
	}
	dc.SelectFont(oldFont);
	/* comments are left-aligned to the right of the longest candidate who has a comment */
	int max_content_width = max(max_candidate_width, comment_shift_width + max_comment_width);
	width = max(width, max_content_width + 2 * _style.margin_x);

	/* Align comments */
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
		_candidateCommentRects[i].OffsetRect(_style.margin_x + comment_shift_width, 0);

	if (candidates.size())
		height += _style.spacing;

	/* Trim the last spacing */
	if (height > 0)
		height -= _style.spacing;
	height += _style.margin_y;

	if (!_context.preedit.str.empty() && !candidates.empty())
	{
		width = max(width, _style.min_width);
		height = max(height, _style.min_height);
	}
	UpdateStatusIconLayout(&width, &height);
	_contentSize.SetSize(width + offsetX * 2, height + offsetY * 2);

	/* Highlighted Candidate */
	int id = _context.cinfo.highlighted;
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		int hlTop = _candidateTextRects[i].top;
		int hlBot = _candidateTextRects[i].bottom;

		if (_candidateLabelRects[i].Height() > 0)
		{
			hlTop = min(_candidateLabelRects[i].top, hlTop);
			hlBot = max(_candidateLabelRects[i].bottom, _candidateTextRects[i].bottom);
		}
		if (_candidateCommentRects[i].Height() > 0)
		{
			hlTop = min(hlTop, _candidateCommentRects[i].top);
			hlBot = max(hlBot, _candidateCommentRects[i].bottom);
		}
		_candidateRects[i].SetRect(_style.margin_x + offsetX, hlTop, width - _style.margin_x + offsetX, hlBot);
	}
	_highlightRect = _candidateRects[id];

	labelFont.DeleteObject();
	textFont.DeleteObject();
	commentFont.DeleteObject();
	oldFont.DeleteObject();
}

void weasel::VerticalLayout::DoLayout(CDCHandle dc, DirectWriteResources* pDWR)
{
	const std::vector<Text> &candidates(_context.cinfo.candies);
	const std::vector<Text> &comments(_context.cinfo.comments);
	const std::vector<Text> &labels(_context.cinfo.labels);

	CSize size;
	//dc.GetTextExtent(L"\x4e2d", 1, &size);
	//const int space = size.cx / 4;
	const int space = _style.hilite_spacing;
	int width = 0, height = _style.margin_y;

	/* Preedit */
	if (!IsInlinePreedit() && !_context.preedit.str.empty())
	{
		size = GetPreeditSize(dc, pDWR->pTextFormat, pDWR->pDWFactory);
		_preeditRect.SetRect(_style.margin_x, height, _style.margin_x + size.cx, height + size.cy);
		_preeditRect.OffsetRect(offsetX, offsetY);
		width = max(width, _style.margin_x + size.cx + _style.margin_x);
		height += size.cy + _style.spacing;
	}

	/* Auxiliary */
	if (!_context.aux.str.empty())
	{
		GetTextExtentDCMultiline(dc, _context.aux.str, _context.aux.str.length(), &size);
		_auxiliaryRect.SetRect(_style.margin_x, height, _style.margin_x + size.cx, height + size.cy);
		_auxiliaryRect.OffsetRect(offsetX, offsetY);
		width = max(width, _style.margin_x + size.cx + _style.margin_x);
		height += size.cy + _style.spacing;
	}

	/* Candidates */
	int comment_shift_width = 0;  /* distance to the left of the candidate text */
	int max_candidate_width = 0;  /* label + text */
	int max_comment_width = 0;    /* comment, or none */
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		if (i > 0 )
			height += _style.candidate_spacing;

		int w = _style.margin_x, h = 0;
		int candidate_width = 0, comment_width = 0;
		/* Label */
		std::wstring label = GetLabelText(labels, i, _style.label_text_format.c_str());
		GetTextSizeDW(label, label.length(), pDWR->pLabelTextFormat, pDWR->pDWFactory, &size);
		_candidateLabelRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		_candidateLabelRects[i].OffsetRect(offsetX, offsetY);
		w += size.cx + space * ((int)(size.cx != 0)), h = max(h, size.cy);
		candidate_width += size.cx + space * ((int)(size.cx != 0));

		/* Text */
		const std::wstring& text = candidates.at(i).str;
		GetTextSizeDW(text, text.length(), pDWR->pTextFormat, pDWR->pDWFactory, &size);
		_candidateTextRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		_candidateTextRects[i].OffsetRect(offsetX, offsetY);
		w += size.cx, h = max(h, size.cy);
		candidate_width += size.cx;
		max_candidate_width = max(max_candidate_width, candidate_width);

		/* Comment */
		if (!comments.at(i).str.empty() && pDWR->pCommentTextFormat != NULL)
		{
			w += space;
			comment_shift_width = max(comment_shift_width, w - _style.margin_x);

			const std::wstring& comment = comments.at(i).str;
			GetTextSizeDW(comment, comment.length(), pDWR->pCommentTextFormat, pDWR->pDWFactory, &size);
			_candidateCommentRects[i].SetRect(0, height, size.cx, height + size.cy);
			_candidateCommentRects[i].OffsetRect(offsetX, offsetY);
			w += size.cx, h = max(h, size.cy);
			comment_width += size.cx;
			max_comment_width = max(max_comment_width, comment_width);
		}
		//w += margin;
		//width = max(width, w);
		
		int ol = 0, ot = 0, oc = 0;
		if (_style.align_type == UIStyle::ALIGN_CENTER)
		{
			ol = (h - _candidateLabelRects[i].Height()) / 2;
			ot = (h - _candidateTextRects[i].Height()) / 2;
			oc = (h - _candidateCommentRects[i].Height()) / 2;
		}
		else if (_style.align_type == UIStyle::ALIGN_BOTTOM)
		{
			ol = (h - _candidateLabelRects[i].Height()) ;
			ot = (h - _candidateTextRects[i].Height()) ;
			oc = (h - _candidateCommentRects[i].Height()) ;

		}
		_candidateLabelRects[i].OffsetRect(0, ol);
		_candidateTextRects[i].OffsetRect(0, ot);
		_candidateCommentRects[i].OffsetRect(0, oc);

		height += h;
	}

	/* comments are left-aligned to the right of the longest candidate who has a comment */
	int max_content_width = max(max_candidate_width, comment_shift_width + max_comment_width);
	width = max(width, max_content_width + 2 * _style.margin_x);

	/* Align comments */
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
		_candidateCommentRects[i].OffsetRect(_style.margin_x + comment_shift_width, 0);

	if (candidates.size())
		height += _style.spacing;

	/* Trim the last spacing */
	if (height > 0)
		height -= _style.spacing;
	height += _style.margin_y;

	if (!_context.preedit.str.empty() && !candidates.empty())
	{
		width = max(width, _style.min_width);
		height = max(height, _style.min_height);
	}
	UpdateStatusIconLayout(&width, &height);
	_contentSize.SetSize(width + offsetX*2, height + offsetY*2);

	/* Highlighted Candidate */
	int id = _context.cinfo.highlighted;
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		int hlTop = _candidateTextRects[i].top;
		int hlBot = _candidateTextRects[i].bottom;

		if (_candidateLabelRects[i].Height() > 0)
		{
			hlTop = min(_candidateLabelRects[i].top, hlTop);
			hlBot = max(_candidateLabelRects[i].bottom, _candidateTextRects[i].bottom);
		}
		if (_candidateCommentRects[i].Height() > 0)
		{
			hlTop = min(hlTop, _candidateCommentRects[i].top);
			hlBot = max(hlBot, _candidateCommentRects[i].bottom);
		}
		_candidateRects[i].SetRect(_style.margin_x + offsetX, hlTop, width - _style.margin_x + offsetX, hlBot);
	}
	_highlightRect = _candidateRects[id];
}

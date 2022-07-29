#pragma once

#include <WeaselCommon.h>
#include <WeaselUI.h>
#include "fontClasses.h"

namespace weasel
{
	class Layout
	{
	public:
		Layout(const UIStyle &style, const Context &context, const Status &status);

		virtual void DoLayout(CDCHandle dc, GDIFonts* pFonts = 0) = 0;
		virtual void DoLayout(CDCHandle dc, DirectWriteResources* pDWR) = 0;
		/* All points in this class is based on the content area */
		/* The top-left corner of the content area is always (0, 0) */
		virtual CSize GetContentSize() const = 0;
		virtual CRect GetPreeditRect() const = 0;
		virtual CRect GetAuxiliaryRect() const = 0;
		virtual CRect GetHighlightRect() const = 0;
		virtual CRect GetCandidateLabelRect(int id) const = 0;
		virtual CRect GetCandidateTextRect(int id) const = 0;
		virtual CRect GetCandidateCommentRect(int id) const = 0;
		virtual CRect GetStatusIconRect() const = 0;

		virtual wstring GetLabelText(const vector<Text> &labels, int id, const wchar_t *format) const = 0;
		virtual bool IsInlinePreedit() const = 0;
		virtual bool ShouldDisplayStatusIcon() const = 0;
		virtual void GetTextExtentDCMultiline(CDCHandle dc, wstring wszString, int nCount, LPSIZE lpSize) const = 0;
		virtual void GetTextSizeDW(const wstring text, int nCount, IDWriteTextFormat* pTextFormat, IDWriteFactory* pDWFactaory,  LPSIZE lpSize) const = 0;
		
		virtual wstring Layout::ConvertCRLF(wstring strString, wstring strCRLF) const = 0;
	protected:
		const UIStyle &_style;
		const Context &_context;
		const Status &_status;
	};
};

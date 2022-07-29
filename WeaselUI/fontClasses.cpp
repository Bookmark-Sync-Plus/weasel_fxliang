#include "stdafx.h"
#include <string>
#include "fontClasses.h"

DirectWriteResources::DirectWriteResources() :
	dpiScaleX_(0),
	dpiScaleY_(0),
	pD2d1Factory(NULL),
	pDWFactory(NULL),
	pRenderTarget(NULL),
	pTextFormat(NULL),
	pLabelTextFormat(NULL),
	pCommentTextFormat(NULL)
{
	// prepare d2d1 resources
	HRESULT hResult = S_OK;
	// create factory
	if (pD2d1Factory == NULL)
		hResult = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2d1Factory);
	// create IDWriteFactory
	if (pDWFactory == NULL)
		hResult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWFactory));
	/* ID2D1HwndRenderTarget */
	if (pRenderTarget == NULL)
	{
		const D2D1_PIXEL_FORMAT format = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
		const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, format);
		pD2d1Factory->CreateDCRenderTarget(&properties, &pRenderTarget);
	}
	//get the dpi information
	pD2d1Factory->GetDesktopDpi(&dpiScaleX_, &dpiScaleY_);
	dpiScaleX_ /= 72.0f;
	dpiScaleY_ /= 72.0f;
}

DirectWriteResources::~DirectWriteResources()
{
	SafeRelease(&pTextFormat);
	SafeRelease(&pLabelTextFormat);
	SafeRelease(&pCommentTextFormat);
	SafeRelease(&pRenderTarget);
	SafeRelease(&pDWFactory);
	SafeRelease(&pD2d1Factory);
}

static void AddAMapping(IDWriteFontFallbackBuilder* pFontFallbackBuilder, const wchar_t* fname, const unsigned int start, const unsigned int end)
{
	DWRITE_UNICODE_RANGE rng;
	rng.first = start;
	rng.last = end;
	pFontFallbackBuilder->AddMapping(&rng, 1, &fname, 1);
}

HRESULT DirectWriteResources::InitResources(wstring label_font_face, int label_font_point,
	wstring font_face, int font_point,
	wstring comment_font_face, int comment_font_point, UIStyle::LayoutAlignType alignType) 
{
	// prepare d2d1 resources
	DWRITE_PARAGRAPH_ALIGNMENT paragraphAliment;
	if (alignType == UIStyle::ALIGN_BOTTOM)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
	else if(alignType == UIStyle::ALIGN_CENTER)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
	else
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;

	HRESULT hResult = S_OK;
	vector<wstring> fontFaceStrVector;
	// text font text format set up
	split(fontFaceStrVector, font_face, is_any_of(L","));
	wstring mainFontFace;
	DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	_ParseFontFace(fontFaceStrVector[0], mainFontFace, fontWeight);
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			fontWeight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pTextFormat));
	if( pTextFormat != NULL)
	{
		pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	// label font text format set up
	split(fontFaceStrVector, label_font_face, is_any_of(L","));
	fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	_ParseFontFace(fontFaceStrVector[0], mainFontFace, fontWeight);
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			fontWeight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			label_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pLabelTextFormat));
	if( pLabelTextFormat != NULL)
	{
		pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pLabelTextFormat->SetParagraphAlignment(paragraphAliment);
		pLabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pLabelTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	// comment font text format set up
	split(fontFaceStrVector, comment_font_face, is_any_of(L","));
	fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	_ParseFontFace(fontFaceStrVector[0], mainFontFace, fontWeight);
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			fontWeight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			comment_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pCommentTextFormat));
	if( pCommentTextFormat != NULL)
	{
		pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pCommentTextFormat->SetParagraphAlignment(paragraphAliment);
		pCommentTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pCommentTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());
	return hResult;
}

HRESULT DirectWriteResources::InitResources(const UIStyle style)
{
	// prepare d2d1 resources
	HRESULT hResult = S_OK;
	DWRITE_PARAGRAPH_ALIGNMENT paragraphAliment;
	if (style.align_type == UIStyle::ALIGN_BOTTOM)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
	else if(style.align_type == UIStyle::ALIGN_CENTER)
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
	else
		paragraphAliment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;

	vector<wstring> fontFaceStrVector;
	// text font text format set up
	split(fontFaceStrVector, style.font_face, is_any_of(L","));
	wstring mainFontFace;
	DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	_ParseFontFace(fontFaceStrVector[0], mainFontFace, fontWeight);
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			fontWeight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			style.font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pTextFormat));
	if( pTextFormat != NULL)
	{
		pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		// candidate text always center vertical
		pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	// label font text format set up
	split(fontFaceStrVector, style.label_font_face, is_any_of(L","));
	fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	_ParseFontFace(fontFaceStrVector[0], mainFontFace, fontWeight);
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			fontWeight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			style.label_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pLabelTextFormat));
	if( pLabelTextFormat != NULL)
	{
		pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pLabelTextFormat->SetParagraphAlignment(paragraphAliment);
		pLabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pLabelTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	// comment font text format set up
	split(fontFaceStrVector, style.comment_font_face, is_any_of(L","));
	fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	_ParseFontFace(fontFaceStrVector[0], mainFontFace, fontWeight);
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
			fontWeight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			style.comment_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pCommentTextFormat));
	if( pCommentTextFormat != NULL)
	{
		pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pCommentTextFormat->SetParagraphAlignment(paragraphAliment);
		pCommentTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pCommentTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(vector<wstring>());

	return hResult;
}
void DirectWriteResources::_ParseFontFace(const std::wstring fontFaceStr, std::wstring& fontFace, DWRITE_FONT_WEIGHT& fontWeight)
{
	std::vector<std::wstring> parsedStrV; 
	boost::algorithm::split(parsedStrV, fontFaceStr, boost::algorithm::is_any_of(L":"));
	fontFace = parsedStrV[0];
	boost::wsmatch res;
	boost::wregex regex  ( L":((THIN)|(EXTRA_LIGHT)|(ULTRA_LIGHT)|(LIGHT)|(SEMI_LIGHT)|(NORMAL)|(MEDIUM)|(DEMI_BOLD)|(SEMI_BOLD)|(BOLD)|(EXTRA_BOLD)|(ULTRA_BOLD)|(BLACK)|(HEAVY)|(EXTRA_BLACK)|(ULTRA_BLACK))" , boost::wregex::icase);
	if (boost::regex_search(fontFaceStr, res, regex))
	{
		if (res[0] == L":THIN" || res[0] == L":thin")
			fontWeight = DWRITE_FONT_WEIGHT_THIN;
		else if (res[0] == L":EXTRA_LIGHT" || res[0] == L":extra_light")
			fontWeight = DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
		else if (res[0] == L":ULTRA_LIGHT" || res[0] == L":ultra_light")
			fontWeight = DWRITE_FONT_WEIGHT_ULTRA_LIGHT;
		else if (res[0] == L":LIGHT" || res[0] == L":light")
			fontWeight = DWRITE_FONT_WEIGHT_LIGHT;
		else if (res[0] == L":SEMI_LIGHT" || res[0] == L":semi_light")
			fontWeight = DWRITE_FONT_WEIGHT_SEMI_LIGHT;
		else if (res[0] == L":MEDIUM" || res[0] == L":medium")
			fontWeight = DWRITE_FONT_WEIGHT_MEDIUM;
		else if (res[0] == L":DEMI_BOLD" || res[0] == L":demi_bold")
			fontWeight = DWRITE_FONT_WEIGHT_DEMI_BOLD;
		else if (res[0] == L":SEMI_BOLD" || res[0] == L":semi_bold")
			fontWeight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
		else if (res[0] == L":BOLD" || res[0] == L":bold")
			fontWeight = DWRITE_FONT_WEIGHT_BOLD;
		else if (res[0] == L":EXTRA_BOLD" || res[0] == L":extra_bold")
			fontWeight = DWRITE_FONT_WEIGHT_EXTRA_BOLD;
		else if (res[0] == L":ULTRA_BOLD" || res[0] == L":ultra_bold")
			fontWeight = DWRITE_FONT_WEIGHT_ULTRA_BOLD;
		else if (res[0] == L":BLACK" || res[0] == L":black")
			fontWeight = DWRITE_FONT_WEIGHT_BLACK;
		else if (res[0] == L":HEAVY" || res[0] == L":heavy")
			fontWeight = DWRITE_FONT_WEIGHT_HEAVY;
		else if (res[0] == L":EXTRA_BLACK" || res[0] == L":extra_black")
			fontWeight = DWRITE_FONT_WEIGHT_EXTRA_BLACK;
		else if (res[0] == L":ULTRA_BLACK" || res[0] == L":ultra_black")
			fontWeight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
		else
			fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	}
	else
		fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
}

void DirectWriteResources::_SetFontFallback(IDWriteTextFormat1* pTextFormat, vector<wstring> fontVector)
{
	IDWriteFontFallback* pSysFallback;
	pDWFactory->GetSystemFontFallback(&pSysFallback);
	IDWriteFontFallback* pFontFallback = NULL;
	IDWriteFontFallbackBuilder* pFontFallbackBuilder = NULL;
	pDWFactory->CreateFontFallbackBuilder(&pFontFallbackBuilder);
	vector<wstring> fallbackFontsVector;
	for (UINT32 i = 1; i < fontVector.size(); i++)
	{
		split(fallbackFontsVector, fontVector[i], is_any_of(L":"));
		wstring _fontFaceWstr, firstWstr, lastWstr;
		if (fallbackFontsVector.size() == 3)
		{
			_fontFaceWstr = fallbackFontsVector[0];
			firstWstr = fallbackFontsVector[1];
			lastWstr = fallbackFontsVector[2];
			if (lastWstr.empty())
				lastWstr = L"10ffff";
			if (firstWstr.empty())
				firstWstr = L"0";
		}
		else if (fallbackFontsVector.size() == 2)	// fontName : codepoint
		{
			_fontFaceWstr = fallbackFontsVector[0];
			firstWstr = fallbackFontsVector[1];
			if (firstWstr.empty())
				firstWstr = L"0";
			lastWstr = L"10ffff";
		}
		else if (fallbackFontsVector.size() == 1)	// if only font defined, use all range
		{
			_fontFaceWstr = fallbackFontsVector[0];
			firstWstr = L"0";
			lastWstr = L"10ffff";
		}
		UINT first = 0, last = 0x10ffff;
		try {
			first = stoi(firstWstr.c_str(), 0, 16);
		}
		catch(...){
			first = 0;
		}
		try {
			last = stoi(lastWstr.c_str(), 0, 16);
		}
		catch(...){
			first = 0x10ffff;
		}
		AddAMapping(pFontFallbackBuilder, _fontFaceWstr.c_str(), first, last);
		fallbackFontsVector.swap(vector<wstring>());
	}
	pFontFallbackBuilder->AddMappings(pSysFallback);
	pFontFallbackBuilder->CreateFontFallback(&pFontFallback);
	pTextFormat->SetFontFallback(pFontFallback);
	SafeRelease(&pFontFallback);
	SafeRelease(&pSysFallback);
	SafeRelease(&pFontFallbackBuilder);
}

GDIFonts::GDIFonts(wstring labelFontFace, int labelFontPoint, wstring textFontFace, int textFontPoint, wstring commentFontFace, int commentFontPoint) 
{
	vector<wstring> fontFaceStrVector;
	split(fontFaceStrVector, labelFontFace, is_any_of(L","));
	wstring _fontFace ;
	vector<wstring> _fontFaceVector;
	split(_fontFaceVector, fontFaceStrVector[0], is_any_of(L":"));
	_LabelFontFace		= _fontFaceVector[0];
	_LabelFontPoint		= labelFontPoint;
	fontFaceStrVector.swap(vector<wstring>());
	_fontFaceVector.swap(vector<wstring>());

	split(fontFaceStrVector, textFontFace, is_any_of(L","));
	split(_fontFaceVector, fontFaceStrVector[0], is_any_of(L":"));
	_TextFontFace		= _fontFaceVector[0];
	_TextFontPoint		= textFontPoint;
	fontFaceStrVector.swap(vector<wstring>());
	_fontFaceVector.swap(vector<wstring>());

	split(fontFaceStrVector, commentFontFace, is_any_of(L","));
	split(_fontFaceVector, fontFaceStrVector[0], is_any_of(L":"));
	_CommentFontFace	= _fontFaceVector[0];
	_CommentFontPoint	= commentFontPoint;
	fontFaceStrVector.swap(vector<wstring>());
	_fontFaceVector.swap(vector<wstring>());
}
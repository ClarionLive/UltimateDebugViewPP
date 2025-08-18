// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "windows.h"

namespace fusion {
namespace debugviewpp {

namespace Colors {

extern const COLORREF Auto;
extern COLORREF BackGround;
extern COLORREF Text;
extern COLORREF Highlight;
extern COLORREF Selection;
extern COLORREF ItemHighlight;
extern COLORREF ItemHighlightText;

void UpdateThemeColors();

} // namespace Colors

COLORREF GetRandomBackColor();
COLORREF GetRandomTextColor();
COLORREF GetRandomProcessColor();

// Calculate a contrasting text color (black or white) based on background color
COLORREF GetContrastingTextColor(COLORREF backgroundColor);

} // namespace debugviewpp
} // namespace fusion

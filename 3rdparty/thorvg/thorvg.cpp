// NOTE: This is essentially just a quick and easy unity build (must faster build times, easier to integrate)
//		 with some extra macro hacks to avoid duplicate "static" symbols
//		 as well as an easy way to isolate specific compiler warnings

// NOTE: Not my code, so not my warnings!
#pragma warning(disable:4067)
#pragma warning(disable:4244)
#pragma warning(disable:4819)
#pragma warning(disable:4334)
#pragma warning(disable:4267)

#include "thorvg.h"

// NOTE: src/lib --------------------------------------------------------------------------------
#include "src/tvgAccessor.cpp"
#define _lineLength _tvgBezier_lineLength
#include "src/tvgBezier.cpp"
#undef _lineLength
#include "src/tvgCanvas.cpp"
#include "src/tvgFill.cpp"
#include "src/tvgGlCanvas.cpp"
#include "src/tvgInitializer.cpp"
#include "src/tvgLinearGradient.cpp"
#define _find _tvgLoader_find
#include "src/tvgLoader.cpp"
#undef _find
#include "src/tvgLzw.cpp"
#include "src/tvgPaint.cpp"
#include "src/tvgPicture.cpp"
#include "src/tvgRadialGradient.cpp"
#include "src/tvgRender.cpp"
#define _lineLength _tvgSaver_lineLength
#include "src/tvgSaver.cpp"
#undef _lineLength
#include "src/tvgScene.cpp"
#include "src/tvgShape.cpp"
#include "src/tvgSwCanvas.cpp"
#include "src/tvgTaskScheduler.cpp"
// ----------------------------------------------------------------------------------------------

// NOTE: src/lib/sw_engine ----------------------------------------------------------------------
#include "src/tvgSwFill.cpp"
#include "src/tvgSwImage.cpp"
#include "src/tvgSwMath.cpp"
#include "src/tvgSwMemPool.cpp"
#include "src/tvgSwRaster.cpp"
#include "src/tvgSwRenderer.cpp"
#include "src/tvgSwRle.cpp"
#define _lineLength _tvgSwShape_lineLength
#include "src/tvgSwShape.cpp"
#undef _lineLength
#include "src/tvgSwStroke.cpp"
// ----------------------------------------------------------------------------------------------

// NOTE: src/loaders/jpg ------------------------------------------------------------------------
#include "src/tvgJpgd.cpp"
#include "src/tvgJpgLoader.cpp"
// ----------------------------------------------------------------------------------------------

// NOTE: src/loaders/png ------------------------------------------------------------------------
#include "src/tvgLodePng.cpp"
#include "src/tvgPngLoader.cpp"
// ----------------------------------------------------------------------------------------------

// NOTE: src/loaders/svg ------------------------------------------------------------------------
#include "src/tvgSvgCssStyle.cpp"
#define _skipComma _tvgSvgLoader_skipComma
#include "src/tvgSvgLoader.cpp"
#undef _skipComma
#define _skipComma _tvgSvgPath_skipComma
#include "src/tvgSvgPath.cpp"
#undef _skipComma
#include "src/tvgSvgSceneBuilder.cpp"
#include "src/tvgSvgUtil.cpp"
#include "src/tvgXmlParser.cpp"
// ----------------------------------------------------------------------------------------------

// NOTE: src/loaders/raw ------------------------------------------------------------------------
#include "src/tvgRawLoader.cpp"
// ----------------------------------------------------------------------------------------------

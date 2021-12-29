// 以下の ifdef ブロックは、DLL からのエクスポートを容易にするマクロを作成するための
// 一般的な方法です。この DLL 内のすべてのファイルは、コマンド ラインで定義された WINDOWSGRAPHICSCAPTURE_EXPORTS
// シンボルを使用してコンパイルされます。このシンボルは、この DLL を使用するプロジェクトでは定義できません。
// ソースファイルがこのファイルを含んでいる他のプロジェクトは、
// WINDOWSGRAPHICSCAPTURE_API 関数を DLL からインポートされたと見なすのに対し、この DLL は、このマクロで定義された
// シンボルをエクスポートされたと見なします。
#ifdef WINDOWSGRAPHICSCAPTURE_EXPORTS
#define WINDOWSGRAPHICSCAPTURE_API __declspec(dllexport)
#else
#define WINDOWSGRAPHICSCAPTURE_API __declspec(dllimport)
#endif


#include "..\UmaCruise\IScreenShotWindow.h"

extern "C" WINDOWSGRAPHICSCAPTURE_API IScreenShotWindow* CreateWindowsGraphicsCapture();

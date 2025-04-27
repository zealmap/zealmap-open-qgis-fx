#ifndef QSMAPAPI_H
#define QSMAPAPI_H

#ifdef _MSC_VER //用于判断是否是 vs 平台
#define QsMap_APP_HIDDEN_API
#ifdef QSMAP_APP_LIBRARY_EXPORTS
#define QSMAP_APP_API __declspec(dllexport)
#else
#define QSMAP_APP_API __declspec(dllimport)
#endif
#else // 说明是 OSX 或者 Linux
#define QSMAP_APP_API __attribute((visibility("default"))) // 明确指示，这个函数在动态库中可见
#define QsMap_APP_HIDDEN_API __attribute((visibility("hidden"))) // 明确指示，这个函数在动态库中不可见
#endif

#endif

#pragma once

#include <errno.h>
#include <cstdio>

#ifndef SRC_FILE_NAME
#define SRC_FILE_NAME __FILE__
#endif

#ifdef __unix__
	#define MTL_COLOR_BLUE		"\x1B[34m"
	#define MTL_COLOR_DEFAULT	"\x1B[0m"
	#define MTL_COLOR_GREEN		"\x1B[32m"
	#define MTL_COLOR_RED		"\e[0;91m"
	#define MTL_COLOR_BROWN		"\x1B[38;2;150;75;0m"
#else
	#define MTL_COLOR_BLUE		""
	#define MTL_COLOR_DEFAULT	""
	#define MTL_COLOR_GREEN		""
	#define MTL_COLOR_RED		""
	#define MTL_COLOR_BROWN		""
#endif
#define MTL_TRACE_COLOR MTL_COLOR_BROWN

#if defined(QODS_SILENT)
#define mtl_info(fmt, args...) do {} while (0)
#elif defined(_MSC_VER)
#define mtl_info(fmt, ...) fprintf(stdout, \
	"%s[%s %s %.3d] " fmt "%s\n", MTL_COLOR_BLUE, SRC_FILE_NAME, \
	__FUNCTION__, __LINE__, __VA_ARGS__, MTL_COLOR_DEFAULT)
#else
#define mtl_info(fmt, args...) fprintf(stdout, \
	"%s[%s %s %.3d]%s " fmt "%s\n", MTL_TRACE_COLOR, \
	SRC_FILE_NAME, __FUNCTION__, __LINE__, MTL_COLOR_BLUE, \
	##args, MTL_COLOR_DEFAULT)
#endif

#if defined(QODS_SILENT)
#define mtl_warn(fmt, args...) do {} while (0)
#elif defined(_MSC_VER)
#define mtl_warn(fmt, ...) fprintf(stderr, \
	"%s[Warning %s %s %.3d] " fmt "%s\n", MTL_COLOR_RED, SRC_FILE_NAME, \
	__FUNCTION__, __LINE__, __VA_ARGS__, MTL_COLOR_DEFAULT)
#else
#define mtl_warn(fmt, args...) fprintf(stderr, \
	"%s[Warning %s %s %.3d] " fmt "%s\n", MTL_COLOR_RED, SRC_FILE_NAME, \
	__FUNCTION__, __LINE__, ##args, MTL_COLOR_DEFAULT)
#endif

#if defined(QODS_SILENT)
#define mtl_trace(fmt, args...) do {} while (0)
#elif defined(_MSC_VER)
#define mtl_trace(fmt, ...) fprintf(stderr, \
	"%s[Trace %s %s %.3d] " fmt "%s\n", MTL_COLOR_RED, SRC_FILE_NAME, \
	__FUNCTION__, __LINE__, __VA_ARGS__, MTL_COLOR_DEFAULT)
#else
#define mtl_trace(fmt, args...) fprintf(stderr, \
	"%s[Trace %s %s %.3d] " fmt "%s\n", MTL_COLOR_RED, SRC_FILE_NAME, \
	__FUNCTION__, __LINE__, ##args, MTL_COLOR_DEFAULT)
#endif

#if defined(QODS_SILENT)
#define mtl_status(status) do {} while (0)
#else
#define mtl_status(status) fprintf (stderr, "%s[%s %s %.3d] %s%s\n", \
	MTL_COLOR_RED, SRC_FILE_NAME, \
	__FUNCTION__, __LINE__, strerror(status), MTL_COLOR_DEFAULT)
#endif

#define mtl_errno fprintf (stderr, "[%s %.3d] %m\n", __LINE__, __FUNCTION__)

#define NO_ASSIGN_COPY_MOVE(TypeName)	\
	TypeName(const TypeName&) = delete;		\
	void operator=(const TypeName&) = delete; \
	TypeName(TypeName&&) = delete;

#define NO_MOVE(TypeName)	\
	TypeName(TypeName&&) = delete;

#define CHECK_TRUE(x) {\
	if (!x) {\
		mtl_trace();\
		return false;\
	}\
}

#define CHECK_TRUE_NULL(x) {\
	if (!x) {\
		mtl_trace();\
		return nullptr;\
	}\
}

#define CHECK_TRUE_VOID(x) {\
	if (!x) {\
		mtl_trace();\
		return;\
	}\
}

#define TRUE_OR(x, v, bits) {\
	if (!x) {\
		v |= uint32_t(bits); \
		mtl_trace();\
	}\
}

#define CHECK_TRUE_AND_DEF(x, obj) {\
	obj = x; \
	if (!obj) {\
		mtl_trace();\
		return false;\
	}\
}

#define CHECK_HANDLE(x) {\
	if (x == VK_NULL_HANDLE) {\
		mtl_trace();\
		return false;\
	}\
}

#define CHECK_PTR(x) {\
	if (x == nullptr) {\
		mtl_trace();\
		return false;\
	}\
}

#define CHECK_PTR_NULL(x) {\
	if (x == nullptr) {\
		mtl_trace();\
		return nullptr;\
	}\
}

#define CHECK_PTR_VOID(x) {\
	if (x == nullptr) {\
		mtl_trace();\
		return;\
	}\
}

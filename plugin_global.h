#pragma once

#if defined(BAZEL_LIBRARY)
#  define BAZEL_EXPORT Q_DECL_EXPORT
#else
#  define BAZEL_EXPORT Q_DECL_IMPORT
#endif

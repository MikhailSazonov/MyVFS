#pragma once

/*\
    Заголовки взял отсюда https://stackoverflow.com/questions/152016/detecting-cpu-architecture-compile-time
    с небольшими изменениями (см. Spin.cpp)
*/

#if defined(__x86_64__) || \
    defined(__i386__) || defined(__i386)

#define VFS_X86

#elif defined(_M_IX86)

#define VFS_X86_WINDOWS

#elif \
defined(__ARM_ARCH_2__) || defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__) \
|| defined(__TARGET_ARM_4T) \
|| defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_) \
|| defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_) \
|| defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) \
|| defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) \
|| defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) \ 
|| defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__) \
|| defined(__aarch64__)

#define VFS_ARM

#endif

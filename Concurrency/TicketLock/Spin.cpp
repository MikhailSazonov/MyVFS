#include "Spin.hpp"

/*
    Задействуем специальную инструкцию ассемблера, уменьшая количество
    потребляемых ресурсов в спинлоке
    (если такая инструкция есть в нашей архитектуре)
*/

/*
    Тестировалось на godbolt.org,
    на x86 и ARM, 64- и 32-битные версии,
    на компиляторах gcc, clang, msvc
*/

void TestTask::Concurrency::Spin() {
    #if defined(VFS_X86)
        asm volatile("pause\n" : : : "memory");
    #elif defined(VFS_X86_WINDOWS)
        __asm pause;
    #elif defined(VFS_ARM)
        asm volatile("yield\n" : : : "memory");
    #endif
}

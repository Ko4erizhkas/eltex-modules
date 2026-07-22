#include "calc.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif


    interface_calc();
    return 0;
}

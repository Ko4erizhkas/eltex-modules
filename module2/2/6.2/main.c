#ifdef _WIN32
#include <windows.h>
#endif

#include "menu.h"

int main()
{
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    print_interface();
    return 0;
}

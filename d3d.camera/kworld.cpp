#include "kwindow.h"
#include "krenderingengine.h"

using namespace std;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    // Ignore the return value because we want to continue running in
    // the unlikely event that HeapSetInformatin fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        {
            unique_ptr<KRenderingEngine> rendering_engine = make_unique<KRenderingEngine>(340, 230);
            rendering_engine->run();
        }
        CoUninitialize();
    }

    return 0;
}

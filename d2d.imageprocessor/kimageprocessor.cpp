#include "kwindow.h"
#include "kimagingengine.h"

using namespace std;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    // Ignore the return value because we want to continue running in
    // the unlikely event that HeapSetInformatin fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        {
            unique_ptr<KImagingEngine> imaging_engine = make_unique<KImagingEngine>(340, 230);
            imaging_engine->run();
        }
        CoUninitialize();
    }

    return 0;
}

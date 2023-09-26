#include "kwindow.h"
#include "kdrawingengine.h"

using namespace std;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    // Ignore the return value because we want to continue running in
    // the unlikely event that HeapSetInformatin fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        {
            unique_ptr<KDrawingEngine> drawing_engine = make_unique<KDrawingEngine>(320, 200);
            drawing_engine->Run();
        }
        CoUninitialize();
    }

    return 0;
}

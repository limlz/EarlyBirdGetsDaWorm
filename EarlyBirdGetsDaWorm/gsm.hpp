typedef void(*FP)(void);

extern int current, previous, next;

extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

void GSM_Initialize(int startingState);
void GSM_Update();

bool GSM_ShouldPreserveCurrentStateOnExit();
bool GSM_ShouldRunLoadAndInitialize();
void GSM_OnStateTransition();

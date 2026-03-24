void Notifications_Load();

void Notifications_Initialize();

void Notifications_Trigger();

void Notifications_Update(bool LiftActive, f32 DT);

bool IsPagerActive();

void Notifications_Draw(s8 PatientFloorNumber, s8 PatientDoorNumber, s8 DestinationFloorNumber, s8 DestinationDoorNumber);

void Notifications_Free();

void Notifications_ResetTaskComplete();
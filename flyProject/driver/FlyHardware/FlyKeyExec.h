#ifndef FLY_KEYEXEC_H_
#define FLY_KEYEXEC_H_

void DemoKeyMessage(void);
void putKeyMessage(BYTE keyID);
void keyEventToHAL(BYTE eKeyEvent);

void touchTimeoutInitClear(void);
void touchTimeoutProc(void);

#endif

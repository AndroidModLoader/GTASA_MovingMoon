void (*RwRenderStateSet)(int, void*);
void (*FlushSpriteBuffer)();
bool (*CalcScreenCoors)(CVector*, CVector*, float*, float*, bool, bool);
void (*RenderBufferedOneXLUSprite)(CVector, float, float, uint8_t, uint8_t, uint8_t, short, float, uint8_t);

float *Foggyness, *CloudCoverage, *ms_fFarClipZ;
void ***gpCoronaTexture, **gpMoonMask;
uint8_t *ms_nGameClockHours, *ms_nGameClockMinutes, *ms_nGameClockDays;
int32_t *m_CurrentStoredValue;
uint32_t *MoonSize;
char* TheCamera;
CVector *CamPos, *m_VectorToSun;
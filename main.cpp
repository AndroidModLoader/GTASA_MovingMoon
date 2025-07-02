#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <stdint.h>
#include <math.h>
#include <time.h>

#define MOON_DRAW_DIST          (150.0f)
#define IMPROVED_MOON_HEIGHT    (50.0f)

#include "SimpleGTA.h"
#include "vars.inl"
#include "iskygfx.h"

MYMOD(net.rusjj.movingmoon, Moving Moon, 1.1, CowBoy69 & RusJJ)
NEEDGAME(com.rockstargames.gtasa)

uintptr_t pGTASA;
void *hGTASA;

CVector MoonVector;
bool MoonVisible = false;
ISkyGFX* skygfx = NULL;



#define DotProduct(v1, v2) (v1.z * v2.z + v1.y * v2.y + v1.x * v2.x)
DECL_HOOKv(FrontNormie, CVector& vec)
{
    FrontNormie(vec);
    if(MoonVisible)
    {
        FrontNormie(MoonVector);
        float dotprod = DotProduct(vec, MoonVector);
        if(dotprod > 0.997f) *MoonSize = (*MoonSize + 1) % 8;
    }
}

uintptr_t RenderMoon_BackTo;
extern "C" void RenderMoon_Patch()
{
    float szx, szy;
    RwV3d screenpos;
    RwV3d worldpos;
    
    float coverage = fmaxf(*Foggyness, *CloudCoverage);
    float decoverage = 1.0f - coverage;
    MoonVisible = false;

    if(decoverage != 0)
    {
        // Moon
        float minute = 60.0f * *ms_nGameClockHours + *ms_nGameClockMinutes;
        int moonfadeout;
        float smoothBrightnessAdjust = 1.9f;
        if(minute > 1100)
        {
            moonfadeout = (int)(fabsf(minute - 1100.0f) / smoothBrightnessAdjust);
        }
        else if(minute < 240)
        {
            moonfadeout = 180;
        }
        else
        {
            moonfadeout = (int)(180.0f - fabsf(minute - 240.0f) * smoothBrightnessAdjust);
        }
        
        if (moonfadeout > 0 && moonfadeout < 340)
        {
            CVector& vecsun = m_VectorToSun[*m_CurrentStoredValue];
            MoonVector = { -vecsun.x, -vecsun.y, -(IMPROVED_MOON_HEIGHT / MOON_DRAW_DIST) * vecsun.z }; // normalized vector (important for DotProd)
            RwV3d pos = {
                MOON_DRAW_DIST * MoonVector.x,
                MOON_DRAW_DIST * MoonVector.y,
                MOON_DRAW_DIST * MoonVector.z
            };

            CamPos = &(*(CMatrix**)(TheCamera + BYBIT(20, 24)))->pos;
            
            worldpos = pos + *CamPos;
            if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false, true))
            {
                screenpos.z = *ms_fFarClipZ;
                
                MoonVisible = true;
                float sz = *MoonSize * 2.7f + 4.0f;
                int brightness = decoverage * moonfadeout;

                RwRenderStateSet(8, (void*)0);
                RwRenderStateSet(6, (void*)0);
                RwRenderStateSet(12, (void*)1);

                if(skygfx && skygfx->GetFeatureLevel(GFX_MOONPHASES))
                {
                    szx *= sz;
                    szy *= sz;

                    float moonPhaseStep = (*ms_nGameClockDays / 31) - 0.5f;

                    // Base
                    RwRenderStateSet(1, NULL);
                    RwRenderStateSet(10, (void*)5);
                    RwRenderStateSet(11, (void*)2);
                    RenderBufferedOneXLUSprite(screenpos, szx, szy, 0, 0, 0, 255, 1.0f / screenpos.z, 255);

                    // Moon mask
                    CVector maskScreenpos = screenpos;
                    maskScreenpos.x += moonPhaseStep * 2.0f * (2.7f * szx);
                    maskScreenpos.y += 0.7f * szy;
                    RwRenderStateSet(1, *gpMoonMask);
                    RwRenderStateSet(10, (void*)5);
                    RwRenderStateSet(11, (void*)2);
                    skygfx->PreMoonMaskRender();
                    RenderBufferedOneXLUSprite(maskScreenpos, 1.7f * szx, 1.7f * szy, 0, 0, 0, 0, 1.0f / screenpos.z, 255);
                    skygfx->PostMoonMaskRender();
            
                    // Actual moon
                    RwRenderStateSet(1, *(gpCoronaTexture[2]));
                    RwRenderStateSet(10, (void*)7);
                    RwRenderStateSet(11, (void*)2);
                    RwRenderStateSet(8, (void*)0);
                    RenderBufferedOneXLUSprite(screenpos, szx, szy, brightness, brightness, 0.85f * brightness, 255, 1.0f / screenpos.z, 255);
                }
                else
                {
                    RwRenderStateSet(1, *(gpCoronaTexture[2]));
                    RwRenderStateSet(10, (void*)5);
                    RwRenderStateSet(11, (void*)2);
                    RwRenderStateSet(8, (void*)0);
                    RenderBufferedOneXLUSprite(screenpos, szx * sz, szy * sz, brightness, brightness, 0.85f * brightness, 255, 1.0f / screenpos.z, 255);
                }
                
                FlushSpriteBuffer();
            }
        }
    }
    
    RwRenderStateSet(10, (void*)2);
    RwRenderStateSet(11, (void*)2);
}
__attribute__((optnone)) __attribute__((naked)) void RenderMoon_Inject(void)
{
#ifdef AML32
    asm volatile(
        "PUSH            {R0-R4}\n"
        "BL              RenderMoon_Patch\n");
    asm volatile(
        "MOV             R12, %0\n"
    :: "r" (RenderMoon_BackTo));
    asm volatile(
        "POP             {R0-R4}\n"
        "MOV             PC, R12");
#else
    asm volatile(
        "BL              RenderMoon_Patch\n");
    asm volatile(
        "MOV             X16, %0"
    :: "r"(RenderMoon_BackTo));
    asm volatile(
        "BR              X16");
#endif
}

extern "C" void OnAllModsLoaded()
{
    logger->SetTag("MovingMoon");
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");
    
    if(!pGTASA || !hGTASA) return;
    
    SET_TO(RwRenderStateSet, aml->GetSym(hGTASA, "_Z16RwRenderStateSet13RwRenderStatePv"));
    SET_TO(FlushSpriteBuffer, aml->GetSym(hGTASA, "_ZN7CSprite17FlushSpriteBufferEv"));
    SET_TO(CalcScreenCoors, aml->GetSym(hGTASA, "_ZN7CSprite15CalcScreenCoorsERK5RwV3dPS0_PfS4_bb"));
    SET_TO(RenderBufferedOneXLUSprite, aml->GetSym(hGTASA, "_ZN7CSprite26RenderBufferedOneXLUSpriteEfffffhhhsfh"));
    
    SET_TO(Foggyness, aml->GetSym(hGTASA, "_ZN8CWeather9FoggynessE"));
    SET_TO(CloudCoverage, aml->GetSym(hGTASA, "_ZN8CWeather13CloudCoverageE"));
    SET_TO(ms_fFarClipZ, aml->GetSym(hGTASA, "_ZN5CDraw12ms_fFarClipZE"));
    SET_TO(gpCoronaTexture, aml->GetSym(hGTASA, "gpCoronaTexture"));
    SET_TO(gpMoonMask, aml->GetSym(hGTASA, "gpMoonMask"));
    SET_TO(ms_nGameClockHours, aml->GetSym(hGTASA, "_ZN6CClock18ms_nGameClockHoursE"));
    SET_TO(ms_nGameClockMinutes, aml->GetSym(hGTASA, "_ZN6CClock20ms_nGameClockMinutesE"));
    SET_TO(ms_nGameClockDays, aml->GetSym(hGTASA, "_ZN6CClock17ms_nGameClockDaysE"));
    SET_TO(m_CurrentStoredValue, aml->GetSym(hGTASA, "_ZN10CTimeCycle20m_CurrentStoredValueE"));
    SET_TO(MoonSize, aml->GetSym(hGTASA, "_ZN8CCoronas8MoonSizeE"));
    SET_TO(TheCamera, aml->GetSym(hGTASA, "TheCamera"));
    SET_TO(m_VectorToSun, aml->GetSym(hGTASA, "_ZN10CTimeCycle13m_VectorToSunE"));
    
  #ifdef AML32
    aml->PlaceB(pGTASA + 0x5DD7F0 + 0x1, pGTASA + 0x5DD80E + 0x1);
    HOOKBLX(FrontNormie, pGTASA + 0x5DD810 + 0x1);
  #else
    aml->PlaceB(pGTASA + 0x702C18, pGTASA + 0x702C34);
    HOOKBL(FrontNormie, pGTASA + 0x702C38);
  #endif

    RenderMoon_BackTo = pGTASA + BYBIT(0x59EEE2 + 0x1, 0x6C2E98);
    aml->Redirect(pGTASA + BYBIT(0x59EC30 + 0x1, 0x6C2BD4), (uintptr_t)RenderMoon_Inject);

    skygfx = (ISkyGFX*)GetInterface("SkyGFX");
}
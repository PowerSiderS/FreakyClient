/* Xash3D/CS 1.6 Gyroscope Feature BY PowerSiderS
	Modification Fixes By NaruTo 1.6 */

#include "hud.h"
#include "usercmd.h"
#include "cvardef.h"

#ifdef __ANDROID__
#include <android/sensor.h>
#include <android/looper.h>
#endif

#include "gyroscope.h"

cvar_t *gyroscope;
cvar_t *gyroscope_sensitivity;

#ifdef __ANDROID__

static ASensorManager *g_pSensorManager = NULL;
static ASensorEventQueue *g_pSensorEventQueue = NULL;
static const ASensor *g_pGyroSensor = NULL;
static ALooper *g_pLooper = NULL;

static float g_flGyroYaw = 0.0f;
static float g_flGyroPitch = 0.0f;
static int64_t g_iLastTimestamp = 0;
static bool g_bGyroInitialized = false;
static bool g_bGyroSensorEnabled = false;
static bool g_bFirstEvent = true;
static float g_flSmoothYaw = 0.0f;
static float g_flSmoothPitch = 0.0f;
static float g_flYawAccum = 0.0f;
static float g_flPitchAccum = 0.0f;

static const float GYRO_RAD2DEG = 57.2957795f;
static const float GYRO_MIN_DEADZONE = 0.010f;
static const float GYRO_MIN_ALPHA = 0.18f;
static const float GYRO_MAX_ALPHA = 0.75f;
static const float GYRO_ALPHA_SCALE = 3.5f;

static const float NS2S = 1.0f / 1000000000.0f;

static void Gyro_EnableSensor(void)
{
	if (!g_bGyroInitialized || !g_pSensorEventQueue || !g_pGyroSensor)
		return;
	
	if (!g_bGyroSensorEnabled)
	{
		ASensorEventQueue_enableSensor(g_pSensorEventQueue, g_pGyroSensor);
		ASensorEventQueue_setEventRate(g_pSensorEventQueue, g_pGyroSensor, 16666);
		g_bGyroSensorEnabled = true;
		g_bFirstEvent = true;
		g_flGyroYaw = 0.0f;
		g_flGyroPitch = 0.0f;
	}
}

static void Gyro_DisableSensor(void)
{
	if (!g_bGyroInitialized || !g_pSensorEventQueue || !g_pGyroSensor)
		return;
	
	if (g_bGyroSensorEnabled)
	{
		ASensorEventQueue_disableSensor(g_pSensorEventQueue, g_pGyroSensor);
		g_bGyroSensorEnabled = false;
		g_flGyroYaw = 0.0f;
		g_flGyroPitch = 0.0f;
	}
}

static int Gyro_SensorCallback(int fd, int events, void *data)
{
    ASensorEvent event;

    while (ASensorEventQueue_getEvents(g_pSensorEventQueue, &event, 1) > 0)
    {
        if (event.type != ASENSOR_TYPE_GYROSCOPE)
            continue;

       
        if (g_bFirstEvent)
        {
            g_iLastTimestamp = event.timestamp;
            g_bFirstEvent = false;
            continue;
        }

       
        float dT = (float)(event.timestamp - g_iLastTimestamp) * NS2S;
        g_iLastTimestamp = event.timestamp;

        if (dT <= 0.0f || dT > 0.5f)
            continue;

        
        float gyroX = event.data[0];
        float gyroY = event.data[1];

      
        float rawYaw   = -gyroX;   
        float rawPitch =  gyroY;  

       
        if (fabsf(rawYaw) < 0.010f) rawYaw = 0.0f;
        if (fabsf(rawPitch) < 0.010f) rawPitch = 0.0f;

        
        float sens = (gyroscope_sensitivity && gyroscope_sensitivity->value > 0.0f)
                     ? gyroscope_sensitivity->value : 1.0f;

        float expo = sens * (1.0f + sens * 0.25f);

        
        float scaledYaw   = rawYaw   * dT * expo * 57.2957795f;
        float scaledPitch = rawPitch * dT * expo * 57.2957795f;

        
        float mag = fmaxf(fabsf(scaledYaw), fabsf(scaledPitch));

       
        float baseAlpha = dT * 140.0f; 

        if (baseAlpha < 0.12f) baseAlpha = 0.12f; 
        if (baseAlpha > 0.30f) baseAlpha = 0.30f;  

        
        float alpha = baseAlpha + mag * 0.40f;

        if (alpha > 0.85f)
            alpha = 0.85f; 

        
        g_flSmoothYaw   = g_flSmoothYaw   * (1.0f - alpha) + scaledYaw   * alpha;
        g_flSmoothPitch = g_flSmoothPitch * (1.0f - alpha) + scaledPitch * alpha;

        g_flGyroYaw   += g_flSmoothYaw;
        g_flGyroPitch += g_flSmoothPitch;
    }

    return 1;
}





void Gyro_Init(void)
{
	gyroscope = gEngfuncs.pfnRegisterVariable("gyroscope", "0", FCVAR_ARCHIVE);
	gyroscope_sensitivity = gEngfuncs.pfnRegisterVariable("gyroscope_sensitivity", "1.0", FCVAR_ARCHIVE);
	
	g_pSensorManager = ASensorManager_getInstance();
	if (!g_pSensorManager)
	{
		gEngfuncs.Con_Printf("Gyroscope: Failed to get sensor manager\n");
		return;
	}
	
	g_pGyroSensor = ASensorManager_getDefaultSensor(g_pSensorManager, ASENSOR_TYPE_GYROSCOPE);
	if (!g_pGyroSensor)
	{
		gEngfuncs.Con_Printf("Gyroscope: Gyroscope sensor not available\n");
		return;
	}
	
	g_pLooper = ALooper_forThread();
	if (!g_pLooper)
	{
		g_pLooper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
	}
	
	if (!g_pLooper)
	{
		gEngfuncs.Con_Printf("Gyroscope: Failed to get looper\n");
		return;
	}
	
	g_pSensorEventQueue = ASensorManager_createEventQueue(
		g_pSensorManager,
		g_pLooper,
		ALOOPER_POLL_CALLBACK,
		Gyro_SensorCallback,
		NULL
	);
	
	if (!g_pSensorEventQueue)
	{
		gEngfuncs.Con_Printf("Gyroscope: Failed to create event queue\n");
		return;
	}
	
	g_bGyroInitialized = true;
	g_bGyroSensorEnabled = false;
	g_bFirstEvent = true;
	g_flGyroYaw = 0.0f;
	g_flGyroPitch = 0.0f;
	
	gEngfuncs.Con_Printf("Gyroscope: Initialized successfully\n");
}

void Gyro_Shutdown(void)
{
	Gyro_DisableSensor();
	
	if (g_pSensorManager && g_pSensorEventQueue)
	{
		ASensorManager_destroyEventQueue(g_pSensorManager, g_pSensorEventQueue);
	}
	
	g_pSensorEventQueue = NULL;
	g_pGyroSensor = NULL;
	g_pSensorManager = NULL;
	g_pLooper = NULL;
	g_bGyroInitialized = false;
	
	gEngfuncs.Con_Printf("Gyroscope: Shutdown\n");
}

void Gyro_Update(float *yaw, float *pitch)
{
    if (!g_bGyroInitialized)
        return;

    bool bShouldEnable = (gyroscope && gyroscope->value != 0.0f);

    if (bShouldEnable && !g_bGyroSensorEnabled)
    {
        Gyro_EnableSensor();
    }
    else if (!bShouldEnable && g_bGyroSensorEnabled)
    {
        Gyro_DisableSensor();
    }

    if (!g_bGyroSensorEnabled)
    {
        g_flSmoothYaw = g_flSmoothPitch = 0.0f;
        g_flGyroYaw = g_flGyroPitch = 0.0f;
        if (yaw) *yaw = 0.0f;
        if (pitch) *pitch = 0.0f;
        return;
    }

    if (g_pLooper)
    {
        ALooper_pollOnce(0, NULL, NULL, NULL);
    }

    if (yaw)   *yaw = g_flGyroYaw;
    if (pitch) *pitch = g_flGyroPitch;

    g_flGyroYaw = 0.0f;
    g_flGyroPitch = 0.0f;
}


void Gyro_Reset(void)
{
    g_flGyroYaw = 0.0f;
    g_flGyroPitch = 0.0f;
    g_flSmoothYaw = 0.0f;
    g_flSmoothPitch = 0.0f;
    g_bFirstEvent = true;
    g_iLastTimestamp = 0;
}


int Gyro_IsEnabled(void)
{
	if (!g_bGyroInitialized)
		return 0;
	if (!gyroscope)
		return 0;
	return (gyroscope->value != 0.0f) ? 1 : 0;
}

#else

void Gyro_Init(void)
{
	gyroscope = gEngfuncs.pfnRegisterVariable("gyroscope", "0", FCVAR_ARCHIVE);
	gyroscope_sensitivity = gEngfuncs.pfnRegisterVariable("gyroscope_sensitivity", "1.0", FCVAR_ARCHIVE);
	gEngfuncs.Con_Printf("Gyroscope: Not available on this platform\n");
}

void Gyro_Shutdown(void)
{
}

void Gyro_Update(float *yaw, float *pitch)
{
	if (yaw) *yaw = 0.0f;
	if (pitch) *pitch = 0.0f;
}

void Gyro_Reset(void)
{
}

int Gyro_IsEnabled(void)
{
	return 0;
}

#endif

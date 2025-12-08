/***
*
*       Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*       
*       This product contains software technology licensed from Id 
*       Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*       All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "hud.h"
#include "cl_util.h"
#include "event_api.h"
#include "studio.h"
#include "com_weapons.h"
#include "wpn_shared.h"
#include "eventscripts.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

static float g_flInspectEndTime = 0.0f;
static float g_flNextInspectTime = 0.0f;
static float g_flOldViewAnimTime = 0.0f;
static int g_iOldViewSequence = 0;
static int g_iLastSetInspectAnimation = 0;

static void SetInspectTime(studiohdr_t *hdr, int seq)
{
        float duration;
        float inspectStartTime;
        mstudioseqdesc_t *desc;

        if (!hdr || seq < 0 || seq >= hdr->numseq)
                return;

        desc = (mstudioseqdesc_t *)((byte *)hdr + hdr->seqindex) + seq;

        if (desc->fps > 0)
                duration = (float)desc->numframes / desc->fps;
        else
                duration = 1.0f;

        inspectStartTime = gEngfuncs.GetClientTime();

        g_flInspectEndTime = inspectStartTime + duration;
        g_flNextInspectTime = (inspectStartTime + 0.5f < g_flInspectEndTime) ? (inspectStartTime + 0.5f) : g_flInspectEndTime;
}

static int g_inspectAnims[] =
{
        0,     /* WEAPON_NONE */
        7,     /* WEAPON_P228 */
        13,    /* WEAPON_GLOCK */
        5,     /* WEAPON_SCOUT */
        0,     /* WEAPON_HEGRENADE */
        6,     /* WEAPON_XM1014 */
        0,     /* WEAPON_C4 */
        6,     /* WEAPON_MAC10 */
        6,     /* WEAPON_AUG */
        0,     /* WEAPON_SMOKEGRENADE */
        16,    /* WEAPON_ELITE */
        6,     /* WEAPON_FIVESEVEN */
        6,     /* WEAPON_UMP45 */
        5,     /* WEAPON_SG550 */
        6,     /* WEAPON_GALIL */
        6,     /* WEAPON_FAMAS */
        15,    /* WEAPON_USP */
        12,    /* WEAPON_GLOCK18 */
        6,     /* WEAPON_AWP */
        6,     /* WEAPON_MP5N */
        5,     /* WEAPON_M249 */
        7,     /* WEAPON_M3 */
        13,    /* WEAPON_M4A1 */
        6,     /* WEAPON_TMP */
        5,     /* WEAPON_G3SG1 */
        0,     /* WEAPON_FLASHBANG */
        5,     /* WEAPON_DEAGLE */
        6,     /* WEAPON_SG552 */
        6,     /* WEAPON_AK47 */
        0,     /* WEAPON_KNIFE */
        6,     /* WEAPON_P90 */
};

static int LookupInspect(int curSequence, int weaponID)
{
        if (weaponID >= (int)(sizeof(g_inspectAnims) / sizeof(int)))
                return g_inspectAnims[0];

        if (weaponID == WEAPON_USP)
        {
                if ((curSequence >= USP_UNSIL_IDLE) &&
                        (curSequence != g_inspectAnims[weaponID]))
                        return g_inspectAnims[weaponID] + 1;
        }
        else if (weaponID == WEAPON_M4A1)
        {
                if ((curSequence >= M4A1_UNSIL_IDLE) &&
                        (curSequence != g_inspectAnims[weaponID]))
                        return g_inspectAnims[weaponID] + 1;
        }

        return g_inspectAnims[weaponID];
}

static void Inspect_f()
{
        cl_entity_t *vm;
        studiohdr_t *studiohdr;
        int sequence;
        int iCurrentWeapon = HUD_GetWeapon();

        vm = gEngfuncs.GetViewModel();

        if (!vm || !vm->model)
        {
                return;
        }

        if (g_iUser1) /* spectating, don't inspect */
                return;

        if (g_flNextInspectTime > gEngfuncs.GetClientTime())
                return;

        switch (iCurrentWeapon)
        {
                /* these weapons have no inspect anims */
                case WEAPON_NONE:
                case WEAPON_HEGRENADE:
                case WEAPON_C4:
                case WEAPON_SMOKEGRENADE:
                case WEAPON_FLASHBANG:
                case WEAPON_SHIELDGUN:
                        return;

                /* don't inspect if doing silencer stuff on usp/m4a1 */
                case WEAPON_USP:
                        switch (vm->curstate.sequence)
                        {
                                case USP_ATTACH_SILENCER:
                                case USP_DETACH_SILENCER:
                                        return;
                        }
                        break;

                case WEAPON_M4A1:
                        switch (vm->curstate.sequence)
                        {
                                case M4A1_ATTACH_SILENCER:
                                case M4A1_DETACH_SILENCER:
                                        return;
                        }
                        break;
        }

        if (!vm->model)
                return;

        studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(vm->model);
        if (!studiohdr)
                return;

        sequence = LookupInspect(vm->curstate.sequence, iCurrentWeapon);

        if (sequence >= studiohdr->numseq)
                return; /* no inspect animation in this model */

        g_iLastSetInspectAnimation = sequence;

        SetInspectTime(studiohdr, sequence);

        gEngfuncs.pEventAPI->EV_WeaponAnimation(sequence, vm->curstate.body);
}

static bool IsIdleSequence(int seq)
{
        /* most idle animations have index of 0 */
        if (seq == 0)
                return true;

        int iCurrentWeapon = HUD_GetWeapon();

        /* handle idle animations that aren't 0 */
        switch (iCurrentWeapon)
        {
                case WEAPON_ELITE:
                        return seq == ELITE_IDLE_LEFTEMPTY;
                case WEAPON_USP:
                        return seq == USP_UNSIL_IDLE;
                case WEAPON_GLOCK18:
                        return seq == GLOCK18_IDLE2 || seq == GLOCK18_IDLE3;
                case WEAPON_M4A1:
                        return seq == M4A1_UNSIL_IDLE;
        }

        return false;
}

void Inspect_Think()
{
        if (g_flInspectEndTime > gEngfuncs.GetClientTime())
        {
                cl_entity_t *vm = gEngfuncs.GetViewModel();

                if (vm && vm->curstate.animtime != g_flOldViewAnimTime && IsIdleSequence(vm->curstate.sequence))
                {
                        /* animation has changed and it's the idle animation, change back to inspect */
                        vm->curstate.sequence = g_iOldViewSequence;
                        vm->curstate.animtime = g_flOldViewAnimTime;
                }

                if (!vm || vm->curstate.sequence != g_iLastSetInspectAnimation)
                {
                        g_flNextInspectTime = 0.0f;
                }

                /* update vars */
                if (vm)
                {
                        g_iOldViewSequence = vm->curstate.sequence;
                        g_flOldViewAnimTime = vm->curstate.animtime;
                }
        }
}

void Inspect_Init()
{
        gEngfuncs.pfnAddCommand("+lookatweapon", Inspect_f);
}

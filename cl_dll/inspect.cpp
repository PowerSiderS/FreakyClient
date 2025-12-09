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

/* Cvar pointers for inspection sequences - users can override in config */
static struct cvar_s *cl_inspect_none = NULL;
static struct cvar_s *cl_inspect_p228 = NULL;
static struct cvar_s *cl_inspect_glock = NULL;
static struct cvar_s *cl_inspect_scout = NULL;
static struct cvar_s *cl_inspect_hegrenade = NULL;
static struct cvar_s *cl_inspect_xm1014 = NULL;
static struct cvar_s *cl_inspect_c4 = NULL;
static struct cvar_s *cl_inspect_mac10 = NULL;
static struct cvar_s *cl_inspect_aug = NULL;
static struct cvar_s *cl_inspect_smokegrenade = NULL;
static struct cvar_s *cl_inspect_elite = NULL;
static struct cvar_s *cl_inspect_fiveseven = NULL;
static struct cvar_s *cl_inspect_ump45 = NULL;
static struct cvar_s *cl_inspect_sg550 = NULL;
static struct cvar_s *cl_inspect_galil = NULL;
static struct cvar_s *cl_inspect_famas = NULL;
static struct cvar_s *cl_inspect_usp = NULL;
static struct cvar_s *cl_inspect_glock18 = NULL;
static struct cvar_s *cl_inspect_awp = NULL;
static struct cvar_s *cl_inspect_mp5n = NULL;
static struct cvar_s *cl_inspect_m249 = NULL;
static struct cvar_s *cl_inspect_m3 = NULL;
static struct cvar_s *cl_inspect_m4a1 = NULL;
static struct cvar_s *cl_inspect_tmp = NULL;
static struct cvar_s *cl_inspect_g3sg1 = NULL;
static struct cvar_s *cl_inspect_flashbang = NULL;
static struct cvar_s *cl_inspect_deagle = NULL;
static struct cvar_s *cl_inspect_sg552 = NULL;
static struct cvar_s *cl_inspect_ak47 = NULL;
static struct cvar_s *cl_inspect_knife = NULL;
static struct cvar_s *cl_inspect_p90 = NULL;

static void SetInspectTime(studiohdr_t *hdr, int seq)
{
        float duration;
        float inspectStartTime;
        mstudioseqdesc_t *desc;
        int iCurrentWeapon = HUD_GetWeapon();

        if (!hdr || seq < 0 || seq >= hdr->numseq)
                return;

        desc = (mstudioseqdesc_t *)((byte *)hdr + hdr->seqindex) + seq;

        if (desc->fps > 0)
        {
                float fps = desc->fps;
                
                /* Increase fps for long inspection animations
                if ((iCurrentWeapon == WEAPON_P228 || iCurrentWeapon == WEAPON_FIVESEVEN) && desc->numframes >= 200)
                {
                        fps = desc->fps * 1.5f;
                }
                */
                
                duration = (float)desc->numframes / fps;
        }
        else
                duration = 1.0f;

        inspectStartTime = gEngfuncs.GetClientTime();

        g_flInspectEndTime = inspectStartTime + duration;
        g_flNextInspectTime = (inspectStartTime + 0.5f < g_flInspectEndTime) ? (inspectStartTime + 0.5f) : g_flInspectEndTime;
}

static int LookupInspect(int curSequence, int weaponID)
{
        int inspectSeq = 0;
        
        /* Get inspection sequence from cvars */
        switch (weaponID)
        {
                case WEAPON_NONE:
                        inspectSeq = cl_inspect_none ? (int)cl_inspect_none->value : 0;
                        break;
                case WEAPON_P228:
                        inspectSeq = cl_inspect_p228 ? (int)cl_inspect_p228->value : 7;
                        break;
                case WEAPON_GLOCK:
                        inspectSeq = cl_inspect_glock ? (int)cl_inspect_glock->value : 13;
                        break;
                case WEAPON_SCOUT:
                        inspectSeq = cl_inspect_scout ? (int)cl_inspect_scout->value : 5;
                        break;
                case WEAPON_HEGRENADE:
                        inspectSeq = cl_inspect_hegrenade ? (int)cl_inspect_hegrenade->value : 0;
                        break;
                case WEAPON_XM1014:
                        inspectSeq = cl_inspect_xm1014 ? (int)cl_inspect_xm1014->value : 6;
                        break;
                case WEAPON_C4:
                        inspectSeq = cl_inspect_c4 ? (int)cl_inspect_c4->value : 0;
                        break;
                case WEAPON_MAC10:
                        inspectSeq = cl_inspect_mac10 ? (int)cl_inspect_mac10->value : 6;
                        break;
                case WEAPON_AUG:
                        inspectSeq = cl_inspect_aug ? (int)cl_inspect_aug->value : 6;
                        break;
                case WEAPON_SMOKEGRENADE:
                        inspectSeq = cl_inspect_smokegrenade ? (int)cl_inspect_smokegrenade->value : 0;
                        break;
                case WEAPON_ELITE:
                        inspectSeq = cl_inspect_elite ? (int)cl_inspect_elite->value : 16;
                        break;
                case WEAPON_FIVESEVEN:
                        inspectSeq = cl_inspect_fiveseven ? (int)cl_inspect_fiveseven->value : 6;
                        break;
                case WEAPON_UMP45:
                        inspectSeq = cl_inspect_ump45 ? (int)cl_inspect_ump45->value : 6;
                        break;
                case WEAPON_SG550:
                        inspectSeq = cl_inspect_sg550 ? (int)cl_inspect_sg550->value : 5;
                        break;
                case WEAPON_GALIL:
                        inspectSeq = cl_inspect_galil ? (int)cl_inspect_galil->value : 6;
                        break;
                case WEAPON_FAMAS:
                        inspectSeq = cl_inspect_famas ? (int)cl_inspect_famas->value : 6;
                        break;
                case WEAPON_USP:
                        inspectSeq = cl_inspect_usp ? (int)cl_inspect_usp->value : 15;
                        break;
                case WEAPON_GLOCK18:
                        inspectSeq = cl_inspect_glock18 ? (int)cl_inspect_glock18->value : 12;
                        break;
                case WEAPON_AWP:
                        inspectSeq = cl_inspect_awp ? (int)cl_inspect_awp->value : 6;
                        break;
                case WEAPON_MP5N:
                        inspectSeq = cl_inspect_mp5n ? (int)cl_inspect_mp5n->value : 6;
                        break;
                case WEAPON_M249:
                        inspectSeq = cl_inspect_m249 ? (int)cl_inspect_m249->value : 5;
                        break;
                case WEAPON_M3:
                        inspectSeq = cl_inspect_m3 ? (int)cl_inspect_m3->value : 7;
                        break;
                case WEAPON_M4A1:
                        inspectSeq = cl_inspect_m4a1 ? (int)cl_inspect_m4a1->value : 13;
                        break;
                case WEAPON_TMP:
                        inspectSeq = cl_inspect_tmp ? (int)cl_inspect_tmp->value : 6;
                        break;
                case WEAPON_G3SG1:
                        inspectSeq = cl_inspect_g3sg1 ? (int)cl_inspect_g3sg1->value : 5;
                        break;
                case WEAPON_FLASHBANG:
                        inspectSeq = cl_inspect_flashbang ? (int)cl_inspect_flashbang->value : 0;
                        break;
                case WEAPON_DEAGLE:
                        inspectSeq = cl_inspect_deagle ? (int)cl_inspect_deagle->value : 5;
                        break;
                case WEAPON_SG552:
                        inspectSeq = cl_inspect_sg552 ? (int)cl_inspect_sg552->value : 6;
                        break;
                case WEAPON_AK47:
                        inspectSeq = cl_inspect_ak47 ? (int)cl_inspect_ak47->value : 6;
                        break;
                case WEAPON_KNIFE:
                        inspectSeq = cl_inspect_knife ? (int)cl_inspect_knife->value : 0;
                        break;
                case WEAPON_P90:
                        inspectSeq = cl_inspect_p90 ? (int)cl_inspect_p90->value : 6;
                        break;
                default:
                        return 0;
        }

        if (weaponID == WEAPON_USP)
        {
                if ((curSequence >= USP_UNSIL_IDLE) && (curSequence != inspectSeq))
                        return inspectSeq + 1;
        }
        else if (weaponID == WEAPON_M4A1)
        {
                if ((curSequence >= M4A1_UNSIL_IDLE) && (curSequence != inspectSeq))
                        return inspectSeq + 1;
        }

        return inspectSeq;
}

static bool IsReloadSequence(int seq, int weaponID)
{
        /* Check if current sequence is a reload animation */
        switch (weaponID)
        {
                case WEAPON_P228:
                        return seq == P228_RELOAD;
                case WEAPON_GLOCK:
                        return seq == GLOCK18_RELOAD;
                case WEAPON_SCOUT:
                        return seq == SCOUT_RELOAD;
                case WEAPON_XM1014:
                        return seq == XM1014_RELOAD;
                case WEAPON_MAC10:
                        return seq == MAC10_RELOAD;
                case WEAPON_AUG:
                        return seq == AUG_RELOAD;
                case WEAPON_ELITE:
                        return seq == ELITE_RELOAD;
                case WEAPON_FIVESEVEN:
                        return seq == FIVESEVEN_RELOAD;
                case WEAPON_UMP45:
                        return seq == UMP45_RELOAD;
                case WEAPON_SG550:
                        return seq == SG550_RELOAD;
                case WEAPON_GALIL:
                        return seq == GALIL_RELOAD;
                case WEAPON_FAMAS:
                        return seq == FAMAS_RELOAD;
                case WEAPON_USP:
                        return seq == USP_RELOAD || seq == USP_UNSIL_RELOAD;
                case WEAPON_GLOCK18:
                        return seq == GLOCK18_RELOAD || seq == GLOCK18_RELOAD2;
                case WEAPON_AWP:
                        return seq == AWP_RELOAD;
                case WEAPON_MP5N:
                        return seq == MP5N_RELOAD;
                case WEAPON_M249:
                        return seq == M249_RELOAD;
                case WEAPON_M3:
                        return seq == M3_RELOAD || seq == M3_START_RELOAD;
                case WEAPON_M4A1:
                        return seq == M4A1_RELOAD || seq == M4A1_UNSIL_RELOAD;
                case WEAPON_TMP:
                        return seq == TMP_RELOAD;
                case WEAPON_G3SG1:
                        return seq == G3SG1_RELOAD;
                case WEAPON_DEAGLE:
                        return seq == DEAGLE_RELOAD;
                case WEAPON_SG552:
                        return seq == SG552_RELOAD;
                case WEAPON_AK47:
                        return seq == AK47_RELOAD;
                case WEAPON_P90:
                        return seq == P90_RELOAD;
        }
        return false;
}

static bool IsShootingSequence(int seq, int weaponID)
{
        /* Check if current sequence is a shoot/fire animation */
        switch (weaponID)
        {
                case WEAPON_P228:
                        return (seq >= P228_SHOOT1 && seq <= P228_SHOOT_EMPTY);
                case WEAPON_GLOCK:
                        return (seq >= GLOCK18_SHOOT && seq <= GLOCK18_SHOOT_EMPTY);
                case WEAPON_SCOUT:
                        return seq == SCOUT_SHOOT || seq == SCOUT_SHOOT2;
                case WEAPON_XM1014:
                        return seq == XM1014_FIRE1 || seq == XM1014_FIRE2 || seq == XM1014_PUMP;
                case WEAPON_MAC10:
                        return (seq >= MAC10_SHOOT1 && seq <= MAC10_SHOOT3);
                case WEAPON_AUG:
                        return (seq >= AUG_SHOOT1 && seq <= AUG_SHOOT3);
                case WEAPON_ELITE:
                        return (seq >= ELITE_SHOOTLEFT1 && seq <= ELITE_SHOOTRIGHTLAST);
                case WEAPON_FIVESEVEN:
                        return (seq >= FIVESEVEN_SHOOT1 && seq <= FIVESEVEN_SHOOT_EMPTY);
                case WEAPON_UMP45:
                        return (seq >= UMP45_SHOOT1 && seq <= UMP45_SHOOT3);
                case WEAPON_SG550:
                        return seq == SG550_SHOOT || seq == SG550_SHOOT2;
                case WEAPON_GALIL:
                        return (seq >= GALIL_SHOOT1 && seq <= GALIL_SHOOT3);
                case WEAPON_FAMAS:
                        return (seq >= FAMAS_SHOOT1 && seq <= FAMAS_SHOOT3);
                case WEAPON_USP:
                        return (seq >= USP_SHOOT1 && seq <= USP_UNSIL_SHOOT_EMPTY);
                case WEAPON_GLOCK18:
                        return (seq >= GLOCK18_SHOOT && seq <= GLOCK18_SHOOT_EMPTY);
                case WEAPON_AWP:
                        return seq == AWP_SHOOT || seq == AWP_SHOOT2 || seq == AWP_SHOOT3;
                case WEAPON_MP5N:
                        return (seq >= MP5N_SHOOT1 && seq <= MP5N_SHOOT3);
                case WEAPON_M249:
                        return seq == M249_SHOOT1 || seq == M249_SHOOT2;
                case WEAPON_M3:
                        return seq == M3_FIRE1 || seq == M3_FIRE2;
                case WEAPON_M4A1:
                        return (seq >= M4A1_SHOOT1 && seq <= M4A1_UNSIL_SHOOT3);
                case WEAPON_TMP:
                        return (seq >= TMP_SHOOT1 && seq <= TMP_SHOOT3);
                case WEAPON_G3SG1:
                        return seq == G3SG1_SHOOT || seq == G3SG1_SHOOT2;
                case WEAPON_DEAGLE:
                        return (seq >= DEAGLE_SHOOT1 && seq <= DEAGLE_SHOOT_EMPTY);
                case WEAPON_SG552:
                        return (seq >= SG552_SHOOT1 && seq <= SG552_SHOOT3);
                case WEAPON_AK47:
                        return (seq >= AK47_SHOOT1 && seq <= AK47_SHOOT3);
                case WEAPON_P90:
                        return (seq >= P90_SHOOT1 && seq <= P90_SHOOT3);
        }
        return false;
}

static bool IsDrawingSequence(int seq, int weaponID)
{
        /* Check if current sequence is a draw/equip animation */
        switch (weaponID)
        {
                case WEAPON_P228:
                        return seq == P228_DRAW;
                case WEAPON_GLOCK:
                        return seq == GLOCK18_DRAW;
                case WEAPON_SCOUT:
                        return seq == SCOUT_DRAW;
                case WEAPON_XM1014:
                        return seq == XM1014_DRAW;
                case WEAPON_MAC10:
                        return seq == MAC10_DRAW;
                case WEAPON_AUG:
                        return seq == AUG_DRAW;
                case WEAPON_ELITE:
                        return seq == ELITE_DRAW;
                case WEAPON_FIVESEVEN:
                        return seq == FIVESEVEN_DRAW;
                case WEAPON_UMP45:
                        return seq == UMP45_DRAW;
                case WEAPON_SG550:
                        return seq == SG550_DRAW;
                case WEAPON_GALIL:
                        return seq == GALIL_DRAW;
                case WEAPON_FAMAS:
                        return seq == FAMAS_DRAW;
                case WEAPON_USP:
                        return seq == USP_DRAW || seq == USP_UNSIL_DRAW;
                case WEAPON_GLOCK18:
                        return seq == GLOCK18_DRAW || seq == GLOCK18_DRAW2;
                case WEAPON_AWP:
                        return seq == AWP_DRAW;
                case WEAPON_MP5N:
                        return seq == MP5N_DRAW;
                case WEAPON_M249:
                        return seq == M249_DRAW;
                case WEAPON_M3:
                        return seq == M3_DRAW;
                case WEAPON_M4A1:
                        return seq == M4A1_DRAW || seq == M4A1_UNSIL_DRAW;
                case WEAPON_TMP:
                        return seq == TMP_DRAW;
                case WEAPON_G3SG1:
                        return seq == G3SG1_DRAW;
                case WEAPON_DEAGLE:
                        return seq == DEAGLE_DRAW;
                case WEAPON_SG552:
                        return seq == SG552_DRAW;
                case WEAPON_AK47:
                        return seq == AK47_DRAW;
                case WEAPON_P90:
                        return seq == P90_DRAW;
        }
        return false;
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
                //case WEAPON_HEGRENADE:
                //case WEAPON_C4:
                //case WEAPON_SMOKEGRENADE:
                //case WEAPON_FLASHBANG:
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

        /* Block inspection during reload, fire, or weapon draw */
        if (IsReloadSequence(vm->curstate.sequence, iCurrentWeapon))
                return;

        if (IsShootingSequence(vm->curstate.sequence, iCurrentWeapon))
                return;

        if (IsDrawingSequence(vm->curstate.sequence, iCurrentWeapon))
                return;

        if (!vm->model)
                return;

        studiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(vm->model);
        if (!studiohdr)
                return;

        sequence = LookupInspect(vm->curstate.sequence, iCurrentWeapon);

        if (sequence >= studiohdr->numseq || sequence < 0)
                return; /* no inspect animation in this model or invalid sequence */

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
                int iCurrentWeapon = HUD_GetWeapon();

                if (!vm)
                        return;

                /* Check if action sequence (reload/fire/draw) is playing - let it complete */
                if (IsReloadSequence(vm->curstate.sequence, iCurrentWeapon) ||
                        IsShootingSequence(vm->curstate.sequence, iCurrentWeapon) ||
                        IsDrawingSequence(vm->curstate.sequence, iCurrentWeapon))
                {
                        /* Action sequence playing - don't interfere, just update tracking */
                        g_iOldViewSequence = vm->curstate.sequence;
                        g_flOldViewAnimTime = vm->curstate.animtime;
                        return;
                }

                /* If idle played during inspect protection, restore inspect animation */
                if (vm->curstate.animtime != g_flOldViewAnimTime && IsIdleSequence(vm->curstate.sequence))
                {
                        vm->curstate.sequence = g_iOldViewSequence;
                        vm->curstate.animtime = g_flOldViewAnimTime;
                }

                /* Update tracking vars */
                g_iOldViewSequence = vm->curstate.sequence;
                g_flOldViewAnimTime = vm->curstate.animtime;
        }
}

void Inspect_Init()
{
        /* Register all inspection sequence cvars with default values */
        cl_inspect_none = gEngfuncs.pfnRegisterVariable("cl_inspect_none", "0", 0);
        cl_inspect_p228 = gEngfuncs.pfnRegisterVariable("cl_inspect_p228", "7", 0);
        cl_inspect_glock = gEngfuncs.pfnRegisterVariable("cl_inspect_glock", "13", 0);
        cl_inspect_scout = gEngfuncs.pfnRegisterVariable("cl_inspect_scout", "5", 0);
        cl_inspect_hegrenade = gEngfuncs.pfnRegisterVariable("cl_inspect_hegrenade", "4", 0);
        cl_inspect_xm1014 = gEngfuncs.pfnRegisterVariable("cl_inspect_xm1014", "6", 0);
        cl_inspect_c4 = gEngfuncs.pfnRegisterVariable("cl_inspect_c4", "4", 0);
        cl_inspect_mac10 = gEngfuncs.pfnRegisterVariable("cl_inspect_mac10", "6", 0);
        cl_inspect_aug = gEngfuncs.pfnRegisterVariable("cl_inspect_aug", "6", 0);
        cl_inspect_smokegrenade = gEngfuncs.pfnRegisterVariable("cl_inspect_smokegrenade", "4", 0);
        cl_inspect_elite = gEngfuncs.pfnRegisterVariable("cl_inspect_elite", "16", 0);
        cl_inspect_fiveseven = gEngfuncs.pfnRegisterVariable("cl_inspect_fiveseven", "6", 0);
        cl_inspect_ump45 = gEngfuncs.pfnRegisterVariable("cl_inspect_ump45", "6", 0);
        cl_inspect_sg550 = gEngfuncs.pfnRegisterVariable("cl_inspect_sg550", "5", 0);
        cl_inspect_galil = gEngfuncs.pfnRegisterVariable("cl_inspect_galil", "6", 0);
        cl_inspect_famas = gEngfuncs.pfnRegisterVariable("cl_inspect_famas", "6", 0);
        cl_inspect_usp = gEngfuncs.pfnRegisterVariable("cl_inspect_usp", "15", 0);
        cl_inspect_glock18 = gEngfuncs.pfnRegisterVariable("cl_inspect_glock18", "12", 0);
        cl_inspect_awp = gEngfuncs.pfnRegisterVariable("cl_inspect_awp", "6", 0);
        cl_inspect_mp5n = gEngfuncs.pfnRegisterVariable("cl_inspect_mp5n", "6", 0);
        cl_inspect_m249 = gEngfuncs.pfnRegisterVariable("cl_inspect_m249", "5", 0);
        cl_inspect_m3 = gEngfuncs.pfnRegisterVariable("cl_inspect_m3", "7", 0);
        cl_inspect_m4a1 = gEngfuncs.pfnRegisterVariable("cl_inspect_m4a1", "13", 0);
        cl_inspect_tmp = gEngfuncs.pfnRegisterVariable("cl_inspect_tmp", "6", 0);
        cl_inspect_g3sg1 = gEngfuncs.pfnRegisterVariable("cl_inspect_g3sg1", "5", 0);
        cl_inspect_flashbang = gEngfuncs.pfnRegisterVariable("cl_inspect_flashbang", "4", 0);
        cl_inspect_deagle = gEngfuncs.pfnRegisterVariable("cl_inspect_deagle", "5", 0);
        cl_inspect_sg552 = gEngfuncs.pfnRegisterVariable("cl_inspect_sg552", "6", 0);
        cl_inspect_ak47 = gEngfuncs.pfnRegisterVariable("cl_inspect_ak47", "6", 0);
        cl_inspect_knife = gEngfuncs.pfnRegisterVariable("cl_inspect_knife", "0", 0);
        cl_inspect_p90 = gEngfuncs.pfnRegisterVariable("cl_inspect_p90", "6", 0);

        gEngfuncs.pfnAddCommand("+lookatweapon", Inspect_f);
        gEngfuncs.pfnAddCommand("-lookatweapon", Inspect_Think);
}

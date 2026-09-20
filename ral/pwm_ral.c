/*******************************************************************************
* File Name        : pwm_ral.c
*
* Description      : Implementation of the vendor-neutral PWM Register Access
*                    Layer (RAL) for the PSOC™ Control C3M/P8 (PSC3) PPCA TCPWM.
*
*                    Every hardware access in this file goes through the device
*                    Register Access Layer headers (the Infineon "Ifx" SFR
*                    headers in mtb-dsl-psc3m8/.../pdl/devices/sfr). No
*                    higher-level PDL/HAL driver call is used here - only the
*                    raw, vendor-supplied register definitions. This file is the
*                    single place that has to change when porting the wrapper to
*                    another MCU vendor.
*
*                    SFR register map used (Ifx_PPCA_TCPWM_GRP_CNT):
*                      - CTRL.MODE / UP_DOWN_MODE / ONE_SHOT / ENABLED
*                      - PERIOD.PERIOD        : PWM period
*                      - CC0.CC / CC0_BUFF.CC : compare (duty-cycle) value
*                      - LINE_SEL.OUT_SEL     : PWM line output source
*                      - TR_CMD.START / STOP  : software start/stop trigger
*                      - COUNTER.COUNTER      : live counter value
*
* Related Document : See README.md
*
********************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "pwm_ral.h"

/* Device Register Access Layer (vendor SFR definitions). This single header
 * brings in the Ifx_PPCA_TCPWM_GRP_CNT register structure and the MODULE_*
 * base-address objects for every PPCA TCPWM counter. */
#include "IfxPPCA_reg.h"

/*******************************************************************************
* Private Macros
*******************************************************************************/
/* CTRL.MODE field: PWM (pulse width modulation) mode. */
#define PWM_RAL_CTRL_MODE_PWM       (4U)
/* CTRL.UP_DOWN_MODE field: count up to PERIOD (left/edge-aligned PWM). */
#define PWM_RAL_CTRL_COUNT_UP       (0U)
/* LINE_SEL.OUT_SEL / COMPL_OUT_SEL fields. */
#define PWM_RAL_LINE_SEL_PWM        (2U) /* line_out      = PWM signal          */
#define PWM_RAL_LINE_SEL_PWM_INV    (3U) /* line_compl_out= inverted PWM signal */
/* TR_OUT_SEL.OUT0 field: emit the terminal-count (period) event on trigger
 * output 0 so it can start a hardware-triggered ADC conversion. */
#define PWM_RAL_TR_OUT_TC           (2U)

/*******************************************************************************
* Private Helper Functions
*******************************************************************************/

/*******************************************************************************
* Function Name: pwm_ral_get_base
********************************************************************************
* Summary:
*  Maps a vendor-neutral PWM instance identifier to the concrete Register Access
*  Layer base object for that TCPWM counter. This is the only function that is
*  aware of the physical register addresses.
*
* Parameters:
*  instance : Vendor-neutral PWM instance identifier.
*
* Return:
*  Pointer to the SFR TCPWM counter register block for the requested instance.
*******************************************************************************/
static Ifx_PPCA_TCPWM_GRP_CNT *pwm_ral_get_base(pwm_ral_instance_t instance)
{
    Ifx_PPCA_TCPWM_GRP_CNT *base;

    switch (instance)
    {
        case PWM_RAL_INSTANCE_1:
            base = &MODULE_PPCA_TCPWM0_GRP0_CNT1;
            break;

        case PWM_RAL_INSTANCE_2:
            base = &MODULE_PPCA_TCPWM0_GRP0_CNT2;
            break;

        case PWM_RAL_INSTANCE_3:
            base = &MODULE_PPCA_TCPWM0_GRP0_CNT3;
            break;

        case PWM_RAL_INSTANCE_0:
        default:
            base = &MODULE_PPCA_TCPWM0_GRP0_CNT0;
            break;
    }

    return base;
}

/*******************************************************************************
* Public Functions
*******************************************************************************/

void pwm_ral_subsystem_enable(void)
{
    /* Enable the peripheral subsystem (un-gates clocks and powers the PPCA
     * blocks) by setting the enable bit of the subsystem control register
     * through the RAL. Idempotent: safe if already enabled by another RAL. */
    REG_PPCA_CNFG_CTRL.B.PPCA_EN = 1U;
}

void pwm_ral_init(pwm_ral_instance_t instance, uint32_t period, uint32_t compare)
{
    Ifx_PPCA_TCPWM_GRP_CNT *pwm = pwm_ral_get_base(instance);

    /* Disable and clear the counter before changing static configuration. */
    pwm->CTRL.U = 0U;

    /* Period and initial compare (duty) value. */
    pwm->PERIOD.B.PERIOD = period;
    pwm->CC0.B.CC        = compare;
    pwm->CC0_BUFF.B.CC   = compare;

    /* Route the PWM signal to the line outputs (line_out / inverted compl). */
    pwm->LINE_SEL.B.OUT_SEL       = PWM_RAL_LINE_SEL_PWM;
    pwm->LINE_SEL.B.COMPL_OUT_SEL = PWM_RAL_LINE_SEL_PWM_INV;

    /* Emit the terminal-count event on trigger output 0 (used to start an ADC
     * conversion on every PWM period). */
    pwm->TR_OUT_SEL.B.OUT0 = PWM_RAL_TR_OUT_TC;

    /* PWM mode, continuous up-counting. Static config must be set while the
     * counter is disabled (ENABLED = 0). */
    pwm->CTRL.B.MODE         = PWM_RAL_CTRL_MODE_PWM;
    pwm->CTRL.B.UP_DOWN_MODE = PWM_RAL_CTRL_COUNT_UP;
    pwm->CTRL.B.ONE_SHOT     = 0U;

    /* Enable (power up) the counter. */
    pwm->CTRL.B.ENABLED = 1U;
}

void pwm_ral_set_compare(pwm_ral_instance_t instance, uint32_t compare)
{
    Ifx_PPCA_TCPWM_GRP_CNT *pwm = pwm_ral_get_base(instance);

    /* Update both the active and buffered compare registers. */
    pwm->CC0.B.CC      = compare;
    pwm->CC0_BUFF.B.CC = compare;
}

void pwm_ral_start(pwm_ral_instance_t instance)
{
    Ifx_PPCA_TCPWM_GRP_CNT *pwm = pwm_ral_get_base(instance);

    /* Issue a software start trigger through the RAL. */
    pwm->TR_CMD.B.START = 1U;
}

void pwm_ral_stop(pwm_ral_instance_t instance)
{
    Ifx_PPCA_TCPWM_GRP_CNT *pwm = pwm_ral_get_base(instance);

    /* Issue a software stop trigger through the RAL. */
    pwm->TR_CMD.B.STOP = 1U;
}

uint32_t pwm_ral_get_counter(pwm_ral_instance_t instance)
{
    Ifx_PPCA_TCPWM_GRP_CNT *pwm = pwm_ral_get_base(instance);

    /* Read the live counter value. */
    return (uint32_t)(pwm->COUNTER.B.COUNTER);
}

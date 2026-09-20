/*******************************************************************************
* File Name        : adc_ral.c
*
* Description      : Implementation of the vendor-neutral ADC Register Access
*                    Layer (RAL) for the PSOC™ Control C3M/P8 (PSC3) ADC.
*
*                    Every hardware access in this file goes through the device
*                    Register Access Layer headers (the Infineon "Ifx" SFR
*                    headers in mtb-dsl-psc3m8/.../pdl/devices/sfr). No
*                    higher-level PDL/HAL driver call is used here - only the
*                    raw, vendor-supplied register definitions. This file is the
*                    single place that has to change when porting the wrapper to
*                    another MCU vendor.
*
*                    SFR register map used (Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC):
*                      - ADC_CTL.ADC_EN     : ADC enable / power
*                      - ADC_CTL.ADC_BUSY   : conversion-in-progress flag
*                      - ADC_TRIGGER        : software/manual channel trigger
*                      - ADC_DATA[ch].ADC_DATA : converted result per channel
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
#include "adc_ral.h"

/* Device Register Access Layer (vendor SFR definitions). This single header
 * brings in the Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC register structure and the
 * MODULE_* base-address objects for every ADC group. */
#include "IfxPPCA_reg.h"

/*******************************************************************************
* Private Macros
*******************************************************************************/
/* ADC_CNV_CNFG.TRIGGER_MODE values: software (manual) start of conversion vs
 * hardware start of conversion (started by an external event routed to the ADC
 * start-of-conversion input). */
#define ADC_RAL_CNV_TRIGGER_MANUAL      (0U)
#define ADC_RAL_CNV_TRIGGER_HARDWARE    (2U)

/*******************************************************************************
* Private Helper Functions
*******************************************************************************/

/*******************************************************************************
* Function Name: adc_ral_get_base
********************************************************************************
* Summary:
*  Maps a vendor-neutral ADC group identifier to the concrete Register Access
*  Layer base object for that ADC slice. This is the only function that is aware
*  of the physical register addresses.
*
* Parameters:
*  group : Vendor-neutral ADC group identifier.
*
* Return:
*  Pointer to the SFR ADC slice register block for the requested group.
*******************************************************************************/
static Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC *adc_ral_get_base(adc_ral_group_t group)
{
    Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC *base;

    switch (group)
    {
        case ADC_RAL_GROUP_1:
            base = &MODULE_PPCA_ATOPSS_ADC_GRP1_SLICE0_ADC;
            break;

        case ADC_RAL_GROUP_2:
            base = &MODULE_PPCA_ATOPSS_ADC_GRP2_SLICE0_ADC;
            break;

        case ADC_RAL_GROUP_3:
            base = &MODULE_PPCA_ATOPSS_ADC_GRP3_SLICE0_ADC;
            break;

        case ADC_RAL_GROUP_0:
        default:
            base = &MODULE_PPCA_ATOPSS_ADC_GRP0_SLICE0_ADC;
            break;
    }

    return base;
}

/*******************************************************************************
* Public Functions
*******************************************************************************/

void adc_ral_subsystem_enable(void)
{
    /* Enable the analog peripheral subsystem (un-gates clocks and powers the
     * analog blocks) by setting the enable bit of the subsystem control
     * register through the RAL. */
    REG_PPCA_CNFG_CTRL.B.PPCA_EN = 1U;
}

void adc_ral_reference_enable(void)
{
    Ifx_PPCA_ATOPSS_ADC_GRP_AREF *aref = &MODULE_PPCA_ATOPSS_ADC_GRP0_AREF;

    /* Configure the analog reference: normal start-up mode, locally generated
     * bias current and locally generated bandgap reference voltage. */
    aref->AREF_CTL.U = 0U;
    aref->AREF_CTL.B.AREF_MODE       = 0U; /* Nominal-noise normal start-up  */
    aref->AREF_CTL.B.AREF_BIAS_SCALE = 3U; /* Recommended bias scaling       */
    aref->AREF_CTL.B.IZTAT_SEL       = 1U; /* Use local 250 nA reference     */
    aref->AREF_CTL.B.VREF_SEL        = 1U; /* Use locally generated VREF     */

    /* Power-on-reset of the reference is controlled by hardware. */
    aref->AREF_ANA_CTL.U = 0U;

    /* Enable the reference. */
    aref->AREF_CTL.B.ENABLED = 1U;
}

void adc_ral_init(adc_ral_group_t group, uint8_t channel, adc_ral_trigger_mode_t trigger_mode)
{
    Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC *adc = adc_ral_get_base(group);

    /* Enable the requested channel as single-ended (CHANNEL_TYPE bit = 0). */
    adc->ADC_CNFG.U = 0U;
    adc->ADC_CNFG.B.CHANNEL_EN = (uint16_t)(1U << channel);

    /* Unsigned conversion result for all channels. */
    adc->ADC_SIGN_UNSIGN_CNFG.U = 0U;

    /* Single channel. Select the start-of-conversion source: software trigger
     * (started later by adc_ral_trigger()) or hardware trigger (started by an
     * external event, e.g. a PWM terminal count, routed to the ADC SOC input). */
    adc->ADC_CNV_CNFG.U = 0U;
    adc->ADC_CNV_CNFG.B.GROUP_CH     = 0U;
    adc->ADC_CNV_CNFG.B.TRIGGER_MODE =
        (ADC_RAL_TRIGGER_HARDWARE == trigger_mode) ? ADC_RAL_CNV_TRIGGER_HARDWARE
                                                   : ADC_RAL_CNV_TRIGGER_MANUAL;

    /* Clear the control register (sub-system handles calibration/gain). */
    adc->ADC_CTL.U = 0U;
}

void adc_ral_enable(adc_ral_group_t group)
{
    Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC *adc = adc_ral_get_base(group);

    /* Set the ADC_EN bit of the ADC control register through the RAL. */
    adc->ADC_CTL.B.ADC_EN = 1U;
}

void adc_ral_disable(adc_ral_group_t group)
{
    Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC *adc = adc_ral_get_base(group);

    /* Clear the ADC_EN bit of the ADC control register through the RAL. */
    adc->ADC_CTL.B.ADC_EN = 0U;
}

void adc_ral_trigger(adc_ral_group_t group, uint16_t channel_mask)
{
    Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC *adc = adc_ral_get_base(group);

    /* Write the per-channel trigger mask to the ADC trigger register. */
    adc->ADC_TRIGGER.U = (uint32_t)channel_mask;
}

bool adc_ral_is_busy(adc_ral_group_t group)
{
    Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC *adc = adc_ral_get_base(group);

    /* Read the ADC_BUSY status bit of the ADC control register. */
    return (bool)(adc->ADC_CTL.B.ADC_BUSY);
}

uint16_t adc_ral_read(adc_ral_group_t group, uint8_t channel)
{
    Ifx_PPCA_ATOPSS_ADC_GRP_SLICE_ADC *adc = adc_ral_get_base(group);

    /* Read the converted-data field of the per-channel ADC data register. */
    return (uint16_t)(adc->ADC_DATA[channel].B.ADC_DATA);
}

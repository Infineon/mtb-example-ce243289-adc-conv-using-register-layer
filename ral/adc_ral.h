/*******************************************************************************
* File Name        : adc_ral.h
*
* Description      : Vendor-neutral ADC Register Access Layer (RAL).
*                    This thin wrapper exposes a small, MCU-vendor independent
*                    API for operating an ADC. The application calls only these
*                    functions; the underlying hardware peripheral is accessed
*                    exclusively through the device Register Access Layer
*                    (the Infineon "Ifx" SFR headers shipped with the device
*                    support library). To port the application to a different
*                    MCU vendor, only adc_ral.c needs to be re-implemented for
*                    that vendor's register map - the application code stays the
*                    same.
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

#ifndef ADC_RAL_H
#define ADC_RAL_H

#include <stdint.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
* Public Types
*******************************************************************************/

/**
 * \brief Vendor-neutral ADC group identifier.
 *
 * The application refers to ADC instances by these abstract identifiers only.
 * The mapping to a concrete hardware base address is done inside adc_ral.c.
 */
typedef enum
{
    ADC_RAL_GROUP_0 = 0U,   /**< First  ADC group  */
    ADC_RAL_GROUP_1 = 1U,   /**< Second ADC group  */
    ADC_RAL_GROUP_2 = 2U,   /**< Third  ADC group  */
    ADC_RAL_GROUP_3 = 3U    /**< Fourth ADC group  */
} adc_ral_group_t;

/**
 * \brief ADC start-of-conversion source.
 */
typedef enum
{
    ADC_RAL_TRIGGER_SOFTWARE = 0U,  /**< Conversions are started by adc_ral_trigger() */
    ADC_RAL_TRIGGER_HARDWARE = 1U   /**< Conversions are started by a hardware event (e.g. a PWM terminal count) routed to the ADC start-of-conversion input */
} adc_ral_trigger_mode_t;

/*******************************************************************************
* Public Function Prototypes
*******************************************************************************/

/**
 * \brief Enable the analog subsystem that hosts the ADC groups.
 *
 * Powers up and un-gates the clocks of the analog peripheral subsystem so that
 * the ADC and reference registers become accessible/operational. Must be called
 * once before any other RAL function. Implemented purely via the Register
 * Access Layer.
 */
void adc_ral_subsystem_enable(void);

/**
 * \brief Initialize and enable the on-chip analog voltage reference.
 *
 * Configures the reference for the locally generated bandgap voltage and
 * enables it. Must be called before triggering conversions. Implemented purely
 * via the Register Access Layer.
 */
void adc_ral_reference_enable(void);

/**
 * \brief Configure one ADC group for a single-channel conversion.
 *
 * Enables the requested channel as single-ended/unsigned and selects the
 * start-of-conversion source (software or hardware). Implemented purely via
 * the Register Access Layer.
 *
 * \param group        ADC group identifier.
 * \param channel      Channel number to enable for conversion.
 * \param trigger_mode Start-of-conversion source (software or hardware).
 */
void adc_ral_init(adc_ral_group_t group, uint8_t channel, adc_ral_trigger_mode_t trigger_mode);

/**
 * \brief Enable (power up) the selected ADC group via the Register Access Layer.
 * \param group Vendor-neutral ADC group identifier.
 */
void adc_ral_enable(adc_ral_group_t group);

/**
 * \brief Disable (power down) the selected ADC group via the Register Access Layer.
 * \param group Vendor-neutral ADC group identifier.
 */
void adc_ral_disable(adc_ral_group_t group);

/**
 * \brief Issue a software/manual conversion trigger via the Register Access Layer.
 * \param group        Vendor-neutral ADC group identifier.
 * \param channel_mask Bit mask of channels to trigger (bit n -> channel n).
 */
void adc_ral_trigger(adc_ral_group_t group, uint16_t channel_mask);

/**
 * \brief Return whether the selected ADC group is currently converting.
 * \param group Vendor-neutral ADC group identifier.
 * \return true while a conversion is in progress, false when idle.
 */
bool adc_ral_is_busy(adc_ral_group_t group);

/**
 * \brief Read the latest converted sample for one channel of an ADC group.
 * \param group   Vendor-neutral ADC group identifier.
 * \param channel Channel number whose result register is read.
 * \return The converted ADC data for that channel.
 */
uint16_t adc_ral_read(adc_ral_group_t group, uint8_t channel);

#if defined(__cplusplus)
}
#endif

#endif /* ADC_RAL_H */

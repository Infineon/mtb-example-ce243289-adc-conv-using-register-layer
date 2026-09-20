/*******************************************************************************
* File Name        : pwm_ral.h
*
* Description      : Vendor-neutral PWM Register Access Layer (RAL).
*                    This thin wrapper exposes a small, MCU-vendor independent
*                    API for operating a PWM. The application calls only these
*                    functions; the underlying hardware peripheral is accessed
*                    exclusively through the device Register Access Layer
*                    (the Infineon "Ifx" SFR headers shipped with the device
*                    support library). To port the application to a different
*                    MCU vendor, only pwm_ral.c needs to be re-implemented for
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

#ifndef PWM_RAL_H
#define PWM_RAL_H

#include <stdint.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
* Public Types
*******************************************************************************/

/**
 * \brief Vendor-neutral PWM instance identifier.
 *
 * The application refers to PWM instances by these abstract identifiers only.
 * The mapping to a concrete hardware base address is done inside pwm_ral.c.
 */
typedef enum
{
    PWM_RAL_INSTANCE_0 = 0U,   /**< First  PWM counter */
    PWM_RAL_INSTANCE_1 = 1U,   /**< Second PWM counter */
    PWM_RAL_INSTANCE_2 = 2U,   /**< Third  PWM counter */
    PWM_RAL_INSTANCE_3 = 3U    /**< Fourth PWM counter */
} pwm_ral_instance_t;

/*******************************************************************************
* Public Function Prototypes
*******************************************************************************/

/**
 * \brief Enable the subsystem that hosts the PWM counters.
 *
 * Powers up and un-gates the clocks of the peripheral subsystem so that the
 * PWM registers become accessible/operational. Must be called once before any
 * other RAL function (it is idempotent, so it is safe if the subsystem was
 * already enabled by another RAL, e.g. the ADC RAL). Implemented purely via
 * the Register Access Layer.
 */
void pwm_ral_subsystem_enable(void);

/**
 * \brief Configure and enable one PWM instance for left-aligned, continuous
 *        edge-aligned PWM generation.
 *
 * Sets the period and the initial compare (duty) value, routes the PWM signal
 * to the counter's output lines and enables the counter. The counter starts
 * running when pwm_ral_start() is called. Implemented purely via the Register
 * Access Layer.
 *
 * \param instance Vendor-neutral PWM instance identifier.
 * \param period   Period value (counter counts 0..period). For a period of
 *                 n counts, pass n-1.
 * \param compare  Compare value that sets the duty cycle (0..period).
 */
void pwm_ral_init(pwm_ral_instance_t instance, uint32_t period, uint32_t compare);

/**
 * \brief Update the PWM compare (duty-cycle) value at run time.
 * \param instance Vendor-neutral PWM instance identifier.
 * \param compare  New compare value (0..period).
 */
void pwm_ral_set_compare(pwm_ral_instance_t instance, uint32_t compare);

/**
 * \brief Issue a software start trigger for the selected PWM via the Register
 *        Access Layer.
 * \param instance Vendor-neutral PWM instance identifier.
 */
void pwm_ral_start(pwm_ral_instance_t instance);

/**
 * \brief Issue a software stop trigger for the selected PWM via the Register
 *        Access Layer.
 * \param instance Vendor-neutral PWM instance identifier.
 */
void pwm_ral_stop(pwm_ral_instance_t instance);

/**
 * \brief Read the live counter value of the selected PWM instance.
 *
 * Useful to confirm the PWM counter is running when no output pin is routed.
 *
 * \param instance Vendor-neutral PWM instance identifier.
 * \return The current counter value.
 */
uint32_t pwm_ral_get_counter(pwm_ral_instance_t instance);

#if defined(__cplusplus)
}
#endif

#endif /* PWM_RAL_H */

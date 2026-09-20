/*******************************************************************************
* File Name:   main.c
*
* Description: This is the main file for the PPCA CPU core 0. The ADC is started
* by the PWM terminal count (hardware trigger); this core reads each conversion
* result through the ADC Register Access Layer (RAL) and shares it with the main
* CPU through shared memory.
*
* Related Document: See README.md
*
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
#include "cy_pdl.h"
#include "adc_ral.h"

/******************************************************************************
* Macros
*******************************************************************************/
/* This is the address in the memory shared between main CPU and PPCA CPUs
 * at which PPCA CPU 0 is using for sharing data with main CPU. */
#define PPCA_M33_0_SHARED_ADDRESS 0x20000400

/* The ADC group and channel used by this example. */
#define ADC_GROUP                 ADC_RAL_GROUP_0
#define ADC_CHANNEL               0U

/* Delay between shared-memory updates, in milliseconds, to throttle the print rate. */
#define CONVERSION_PERIOD_MS      100U

/*******************************************************************************
* Global Variables
*******************************************************************************/
volatile int32_t *adc_read_data   = (int32_t *)PPCA_M33_0_SHARED_ADDRESS;
volatile int32_t *adc_conv_status = (int32_t *)PPCA_M33_0_SHARED_ADDRESS + 1;

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: main
*********************************************************************************
* Summary:
* This is the main function for PPCA CPU 0. The ADC is hardware-triggered by the
* PWM terminal count; this function periodically reads the latest converted data
* through the ADC Register Access Layer and writes it to shared memory for the
* main core to print.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    *adc_read_data   = 0;
    *adc_conv_status = 0;

    for(;;)
    {
        /* The ADC is started by the PWM terminal count (hardware trigger), so no
         * software trigger is issued here. Read the most recent conversion result
         * and share it with the main core. */
        *adc_read_data = (int32_t)adc_ral_read(ADC_GROUP, ADC_CHANNEL);

        /* Inform the main core that new data is available. */
        *adc_conv_status = 1;

        /* Throttle the shared-memory update / print rate. */
        Cy_SysLib_Delay(CONVERSION_PERIOD_MS);
    }
}

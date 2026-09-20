/*******************************************************************************
* File Name:   main.c
*
* Description: This is the main file for the main CPU non safe application of
* the code example. It brings up the ADC analog signal chain and a PWM entirely
* through the vendor-neutral ADC/PWM Register Access Layer (RAL), boots the PPCA
* CPU core 0 that performs the ADC conversions, then reads the converted data
* shared by that core and prints it (with the live PWM counter) over UART.
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
#include "cycfg.h"
#include "cybsp.h"
#include <stdio.h>
#include "cy_retarget_io.h"
#include "adc_ral.h"
#include "pwm_ral.h"

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Debug UART variables */
static cy_stc_scb_uart_context_t    UART_context; /* UART context */
static mtb_hal_uart_t               UART_hal_obj; /* Debug UART HAL object */

/******************************************************************************
* Macros
*******************************************************************************/
/* These are the addresses where the core0 and core1 images are located and its size*/
#define CORE0_IMAGE_ADDRESS    CYMEM_CM33_0_S_m33s_ppca0_nvm_C_S_START    //   0x12030000
#define CORE1_IMAGE_ADDRESS    CYMEM_CM33_0_S_m33s_ppca1_nvm_C_S_START    //   0x12038000
#define PPCA0_IMAGE_SIZE       CYMEM_CM33_0_S_ppca0_code_SIZE
#define PPCA1_IMAGE_SIZE       CYMEM_CM33_0_S_ppca1_code_SIZE

/* This is the address in the memory shared between main CPU and PPCA CPUs
 * at which PPCA CPU 0 is using for sharing data with main CPU. */
#define PPCA_M33_0_SHARED_ADDRESS 0x53020400

/* The ADC group and channel used by this example. */
#define ADC_GROUP              ADC_RAL_GROUP_0
#define ADC_CHANNEL            0U

/* The PWM instance and its period / compare (50% duty) used by this example. */
#define PWM_INSTANCE           PWM_RAL_INSTANCE_0
#define PWM_PERIOD             999U
#define PWM_COMPARE            500U

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: main
*********************************************************************************
* Summary:
* This is the main function for the non safe project for the main core. It
* initializes the debug UART, brings up the ADC analog signal chain through the
* vendor-neutral ADC Register Access Layer, starts the PPCA CPU core 0 that
* performs the conversions, and prints the data received from it over UART.
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
    cy_rslt_t result;

    volatile int32_t *adc_read_data   = (int32_t *)PPCA_M33_0_SHARED_ADDRESS;
    volatile int32_t *adc_conv_status = (int32_t *)PPCA_M33_0_SHARED_ADDRESS + 1;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Init(UART_HW, &UART_config, &UART_context);
    Cy_SCB_UART_Enable(UART_HW);

    /* Setup the HAL UART */
    result = mtb_hal_uart_setup(&UART_hal_obj, &UART_hal_config,
                                &UART_context, NULL);

    /* HAL UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize redirecting of low level IO */
    result = cy_retarget_io_init(&UART_hal_obj);

    /* retarget IO init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Transmit header to the terminal */
    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("************************************************************\r\n");
    printf("PSOC Control C3M/P8: ADC and PWM using Register Access Layer\r\n");
    printf("************************************************************\r\n\n");

    /* enable interrupts */
    __enable_irq();

    /* Bring up the analog signal chain and the PWM entirely through the
     * vendor-neutral ADC/PWM Register Access Layer (no PDL peripheral-init /
     * device-configurator settings are used for the ADC or PWM). */

    /* Enable the analog peripheral subsystem that hosts the ADC. */
    adc_ral_subsystem_enable();

    /* Initialize and enable the on-chip analog voltage reference. */
    adc_ral_reference_enable();

    /* Configure ADC group 0 for a single-channel, hardware-triggered
     * conversion (started by the PWM terminal count) and enable it. */
    adc_ral_init(ADC_GROUP, ADC_CHANNEL, ADC_RAL_TRIGGER_HARDWARE);
    adc_ral_enable(ADC_GROUP);

    /* Configure the PWM through the PWM RAL. Its terminal-count event is routed
     * (by the device configuration) to the ADC start-of-conversion input. */
    pwm_ral_init(PWM_INSTANCE, PWM_PERIOD, PWM_COMPARE);

    /* Activate that trigger route: the routing unit is set up by the device
     * configuration but left disabled, so enable the event-processing unit and
     * the routing unit before starting the PWM. */
    Cy_PPCA_EPU_Enable(PPCA_EPU);
    Cy_PPCA_EPU_PU_T1_Enable(START_TRIG_HW, START_TRIG_INDEX, CY_ENABLE_ASYNC_BYPASS);

    /* Start the PWM; each period now triggers one ADC conversion. */
    pwm_ral_start(PWM_INSTANCE);

    /* Initializing and starting PPCA CPU Core 0 and Core 1. */
    Cy_System_Init_CPU0((void*)CORE0_IMAGE_ADDRESS, PPCA0_IMAGE_SIZE);
    Cy_System_Init_CPU1((void*)CORE1_IMAGE_ADDRESS, PPCA1_IMAGE_SIZE);

    for (;;)
    {
        /* Checking for data update */
        if(1 == *adc_conv_status)
        {
            /* Print the ADC result (via ADC RAL) and the live PWM counter
             * (via PWM RAL) to show both peripherals running register-only. */
            printf("\r\n ADC Channel 0 Data: %d | PWM Counter: %u",
                   (int)*adc_read_data,
                   (unsigned)pwm_ral_get_counter(PWM_INSTANCE));
            *adc_conv_status = 0;
        }
    }
}

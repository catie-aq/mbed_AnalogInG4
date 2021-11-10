/*
 * Copyright (c) 2017, CATIE, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef ANALOGING4_H
#define ANALOGING4_H

#define NUMBER_OF_DMA_READINGS  50

#include <mbed.h>


/*!
 *  \class AnalogInG4
 *  ADC Through DMA using HRTIM STM32 Driver
 */
class AnalogInG4 {

public:

    /** Create an AnalogInG4, connected to the specified pin.
     *  This will setup a DMA to write value from ADC in a buffer at all time.
     *  The ADC is configured to be triggered from the first PwmOutG4 declared.
     *
     *
     * @param pin AnalogInG4 pin to connect to.
     *
     */
    AnalogInG4(PinName pin);

    ~AnalogInG4();

    /** Read the input voltage, represented as an unsigned short in the range [0x0, 0x1000]
     *
     * THE CORESPONDING PwmOutG4 OBJECT MUST BE START BEFORE READING THE ADC
     * (Because if not, the ADC will never be triggered by the HRTIM, and the corresponding DMA will not work)
     *
     * @returns
     *   16-bit unsigned short representing the current input voltage
     */
    uint16_t read_u16();

    // Custom function, not MBED related, to set the offset at start (no load must be plugged on the power board)
    void setCurrentOffset();

    // Custom function, to convert [V] to [mA] using shunt amplifier on Zest_Actuator_HalfBridges.
    int16_t getCurrentMilliAmps();

private:

    static bool _adc_hal_clk_enable;
    static bool _dma_hal_clk_enable;

    PinName _pin;
    ADC_HandleTypeDef _hadc;
    DMA_HandleTypeDef _hdma_adc;

    ADC_TypeDef *_instance;
    uint32_t _channel;
    bool _multimode;

    DMA_Channel_TypeDef *_dma_instance;
    uint32_t _dma_request;

    GPIO_TypeDef *_gpio_port;
    uint32_t _gpio_pin;

    volatile uint16_t _adc_dma_buffer[NUMBER_OF_DMA_READINGS] = {0};
    uint16_t _current_offset = 0;

    void initADC();

    void setupADC();

    void setupDMA();

    void setupGPIO();

    void startADC();

};


#endif //ANALOGING4_H

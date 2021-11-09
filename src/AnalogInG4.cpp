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


#include "AnalogInG4.h"

// Define static members, initialized only one time
bool AnalogInG4::_adc_hal_clk_enable = false;


AnalogInG4::AnalogInG4(PinName pin) :
        _pin(pin) {

    // Init specific registers regarding the ADC_INx for the STM32G474VET6
    // TODO: Get parameters automatically for request pin ? could it be possible ?
    switch (_pin) {

        case ADC_IN1: // PA0 (HAL: ADC1 IN1)
            _instance = ADC1;
            _channel = ADC_CHANNEL_1;
            _gpio_port = GPIOA;
            _gpio_pin = GPIO_PIN_0;
            break;
        case ADC_IN2: // PA7 (HAL: ADC2 IN4)
            _instance = ADC2;
            _channel = ADC_CHANNEL_4;
            _gpio_port = GPIOA;
            _gpio_pin = GPIO_PIN_7;
            break;
        case ADC_IN3: // PB1 (HAL: ADC3 IN1)
            _instance = ADC3;
            _channel = ADC_CHANNEL_1;
            _gpio_port = GPIOB;
            _gpio_pin = GPIO_PIN_1;
            break;
        case ADC_IN4: // PE8 (HAL: AD4 IN6)
            _instance = ADC4;
            _channel = ADC_CHANNEL_6;
            _gpio_port = GPIOE;
            _gpio_pin = GPIO_PIN_8;
            break;
        default:
            while (true) {
                error("ERROR AnalogInG4: object must be initialized with a known pin. Pin %d not recognized. See AnalogInG4.cpp for a list of known pins.",
                      _pin);
            }
    }

    initADC();
}

AnalogInG4::~AnalogInG4() {

}

void AnalogInG4::initADC() {

    setupDMA();
    setupGPIO();
    setupADC();
    startADC();
}

void AnalogInG4::setupADC() {

    // Turn off all ADC clock if not already done
    if (!_adc_hal_clk_enable) {
        __HAL_RCC_ADC12_CLK_ENABLE();
        __HAL_RCC_ADC345_CLK_ENABLE();
        _adc_hal_clk_enable = true;
    }

    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    /** Common config
    */
    _hadc.Instance = _instance;
    _hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    _hadc.Init.Resolution = ADC_RESOLUTION_12B;
    _hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    _hadc.Init.GainCompensation = 0;
    _hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    _hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    _hadc.Init.LowPowerAutoWait = DISABLE;
    _hadc.Init.ContinuousConvMode = DISABLE;
    _hadc.Init.NbrOfConversion = 1;
    _hadc.Init.DiscontinuousConvMode = DISABLE;
    _hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIG_HRTIM_TRG1; // Always the same trig source regarding the ADC
    _hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    _hadc.Init.DMAContinuousRequests = ENABLE; // To get DMA working at all time
    _hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN; // To get DMA working at all time
    _hadc.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&_hadc) != HAL_OK) {
        printf("Error while HAL init ADC.\n");
    }
    /** Configure the ADC multi-mode
    */
    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&_hadc, &multimode) != HAL_OK) {
        printf("Error while setting ADC independent mode.\n");
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&_hadc, &sConfig) != HAL_OK) {
        printf("Error while setting ADC channel.\n");
    }

}

void AnalogInG4::setupDMA() {

    // DMA controller clock enable
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* ADC1 DMA Init */
    /* ADC1 Init */
    _hdma_adc.Instance = DMA1_Channel1;
    _hdma_adc.Init.Request = DMA_REQUEST_ADC1;
    _hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    _hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
    _hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
    _hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    _hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    _hdma_adc.Init.Mode = DMA_CIRCULAR;
    _hdma_adc.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&_hdma_adc) != HAL_OK) {
        printf("Error while setting DMA for ADC.\n");
    }

    __HAL_LINKDMA(&_hadc, DMA_Handle, _hdma_adc);

}

void AnalogInG4::setupGPIO() {

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // can't use switch case, because gpio port is a type def structure
    if (_gpio_port == GPIOA)
        __HAL_RCC_GPIOA_CLK_ENABLE();

    if (_gpio_port == GPIOB)
        __HAL_RCC_GPIOB_CLK_ENABLE();

    if (_gpio_port == GPIOC)
        __HAL_RCC_GPIOC_CLK_ENABLE();

    if (_gpio_port == GPIOD)
        __HAL_RCC_GPIOD_CLK_ENABLE();

    if (_gpio_port == GPIOE)
        __HAL_RCC_GPIOE_CLK_ENABLE();

    if (_gpio_port == GPIOF)
        __HAL_RCC_GPIOF_CLK_ENABLE();

    if (_gpio_port == GPIOG)
        __HAL_RCC_GPIOG_CLK_ENABLE();

    // init gpio struct
    GPIO_InitStruct.Pin = _gpio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(_gpio_port, &GPIO_InitStruct);
}

void AnalogInG4::startADC() {

    HAL_ADC_Start_DMA(&_hadc, (uint32_t *) _adc_dma_buffer, NUMBER_OF_DMA_READINGS);

}

uint16_t AnalogInG4::read_u16() {
    return _adc_dma_buffer[0];
}
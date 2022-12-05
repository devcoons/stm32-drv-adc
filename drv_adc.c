/*!
	@file   drv_adc.c
	@brief  <brief description here>
	@t.odo	-
	---------------------------------------------------------------------------
	MIT License
	Copyright (c) 2020 Io. D
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/
/******************************************************************************
* Preprocessor Definitions & Macros
******************************************************************************/

/******************************************************************************
* Includes
******************************************************************************/

#include "drv_adc.h"

#define ADC_DATA_MAXROWS 2
#define ADC_CHANNELS 11

#ifdef DRV_ADC_ENABLED
/******************************************************************************
* Enumerations, structures & Variables
******************************************************************************/

static adc_t *adc_interfaces[4] = {NULL,NULL,NULL,NULL};
static uint32_t adc_interfaces_cnt = 0;

/******************************************************************************
* Declaration | Static Functions
******************************************************************************/

/******************************************************************************
* Definition  | Static Functions
******************************************************************************/

/******************************************************************************
* Definition  | Public Functions
******************************************************************************/

i_status adc_initialize(adc_t* instance)
{
	memset(instance->value,0,sizeof(uint16_t) * instance->dma_storage_sz * instance->max_adc_channels);

#if defined(STM32L5)
	if(HAL_ADCEx_Calibration_Start(instance->handler, ADC_SINGLE_ENDED) != HAL_OK)  goto analog_initialize_error;
#elif defined(STM32H7)
	if(HAL_ADCEx_Calibration_Start(instance->handler, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)  goto analog_initialize_error;
#endif

	HAL_Delay(10);
	if(HAL_ADC_Start_DMA(instance->handler, (uint32_t*)instance->value, instance->dma_storage_sz * instance->max_adc_channels) != HAL_OK)  goto analog_initialize_error;

	instance->init_sts = 1;
	instance->is_busy = 0;

	for(register int i=0;i<adc_interfaces_cnt;i++)
		if(adc_interfaces[i] == instance)
			return I_OK;

	adc_interfaces[adc_interfaces_cnt] = instance;
	adc_interfaces_cnt++;
	return I_OK;

	analog_initialize_error:
	return I_ERROR;
}

i_status adc_deinitialize(adc_t* instance)
{
	if(instance == NULL)
		return I_ERROR;

	instance->init_sts = 0;
	instance->is_busy = 0;

	HAL_ADC_DeInit(instance->handler);
	memset(instance->value,0,sizeof(uint16_t) * instance->dma_storage_sz * instance->max_adc_channels);
	return I_OK;
}

i_status adc_get_value(adc_t* instance, ADC_Input input, uint16_t* value)
{
	uint8_t current_adc = (input & 0xF0) >> 4;
	uint8_t current_channel = input & 0x0F;
	uint64_t sum = 0;

	if (current_adc == 0 || instance->value == NULL)
		return I_NOTEXISTS;

	for (register uint16_t i = 0; i < instance->dma_storage_sz; i++)
		sum += (uint64_t)/*pow(*/instance->value[i * instance->max_adc_channels + current_channel]/*, 2)*/;

	sum /= instance->dma_storage_sz;

	*value = (uint16_t)/*sqrt(*/sum/*)*/;

	return I_OK;
}

i_status stop_adc(adc_t* instance)
{
	instance->init_sts=0;
	HAL_ADC_Stop_DMA(instance->handler);
	return I_OK;
}

/******************************************************************************
* Definition  | Callback Functions
******************************************************************************/

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	adc_t* current_adc;

	for(register int i=0;i<adc_interfaces_cnt;i++)
		if(adc_interfaces[i]->handler == hadc)
			current_adc = adc_interfaces[i];

	if(current_adc == NULL)
		return;
	current_adc->is_busy = 0;
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
	adc_t* current_adc;

	for(register int i=0;i<adc_interfaces_cnt;i++)
		if(adc_interfaces[i]->handler == hadc)
			current_adc = adc_interfaces[i];

	if(current_adc == NULL)
		return;

	current_adc->is_busy = 1;
}

/******************************************************************************
* EOF - NO CODE AFTER THIS LINE
******************************************************************************/
#endif

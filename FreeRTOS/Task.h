/*
 * Task.h
 *
 *  Created on: 9 Sep 2020
 *      Author: Joshua
 */

#ifndef FREERTOS_TASK_H_
#define FREERTOS_TASK_H_

#include <type_traits>
#include "FreeRTOS.h"
#include "task.h"
#include <type_traits>

/* Bind a parameter to a FreeRTOS task with compile-time type checking */
namespace FreeRTOS {
template <typename F, typename T>
void inline bind(
		F&& f, T* parameter, char const * const name,
		configSTACK_DEPTH_TYPE stack_size = configMINIMAL_STACK_SIZE,
		UBaseType_t priority = tskIDLE_PRIORITY + 1UL,
		TaskHandle_t * const created_task = nullptr) {
	static_assert( std::is_invocable<F, T*>::value );
	xTaskCreate((TaskFunction_t) +f, name, stack_size, parameter, priority, created_task);
}
}

#endif /* FREERTOS_TASK_H_ */

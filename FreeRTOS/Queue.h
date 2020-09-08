/*
 * Queue.h
 *
 *  Created on: 7 Sep 2020
 *      Author: Joshua
 */

#ifndef FREERTOS_QUEUE_H_
#define FREERTOS_QUEUE_H_

#include "FreeRTOS.h"
#include "queue.h"

namespace FreeRTOS {
template <typename T, size_t S>
class Queue {
public:
	using value_type = T;
	Queue() : queue{ xQueueCreate(S, sizeof(T)) } { }

	~Queue() {
		vQueueDelete(queue);
	}

	Queue(Queue const & queue) = delete;
	Queue(Queue&&) = delete;

	void push_front(const T& t, TickType_t ticksToWait = 0) noexcept {
		if (!is_interrupt())
			xQueueSendToFront(queue, &t, ticksToWait);
		else
			xQueueSendToFrontFromISR(queue, &t, nullptr);
	}

	void push_back(const T& t, TickType_t ticksToWait = 0) noexcept {
		if (!is_interrupt())
			xQueueSendToBack(queue, &t, ticksToWait);
		else
			xQueueSendToBackFromISR(queue, &t, nullptr);
	}

	// I guess anything that constructs a T could potentially throw an exception?
	[[nodiscard]] T pop_back(TickType_t ticksToWait = portMAX_DELAY) {
		T t;
		if (!is_interrupt())
			xQueueReceive(queue, &t, ticksToWait);
		else
			xQueueReceiveFromISR(queue, &t, nullptr);
		return t;
	}

	[[nodiscard]] T peek(TickType_t ticksToWait = portMAX_DELAY) {
		T t;
		if (!is_interrupt())
			xQueuePeek(queue, &t, ticksToWait);
		else
			xQueuePeekFromISR(queue, &t);
		return t;
	}

	[[nodiscard]] bool empty() noexcept {
		if (!is_interrupt())
			return uxQueueMessagesWaiting(queue);
		else
			return xQueueIsQueueEmptyFromISR(queue);
	}

	[[nodiscard]] size_t size() noexcept {
		if (!is_interrupt())
			return uxQueueMessagesWaiting(queue);
		else
			return uxQueueMessagesWaitingFromISR(queue);
	}

private:
	QueueHandle_t queue;

	[[nodiscard]] static inline bool is_interrupt() noexcept {
		return SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
	}
};
}

#endif /* FREERTOS_QUEUE_H_ */

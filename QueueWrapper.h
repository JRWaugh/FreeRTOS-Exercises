/*
 * QueueWrapper.h
 *
 *  Created on: 7 Sep 2020
 *      Author: Joshua
 */

#ifndef QUEUEWRAPPER_H_
#define QUEUEWRAPPER_H_

#include "FreeRTOS.h"
#include "queue.h"

template <typename T, size_t S>
class QueueWrapper {
public:
	using value_type = T;
	QueueWrapper() : queue{ xQueueCreate(S, sizeof(T)) } { }

	~QueueWrapper() {
		vQueueDelete(queue);
	}

    QueueWrapper(QueueWrapper const & queue) = delete;
    QueueWrapper(QueueWrapper&&) = delete;

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

#endif /* QUEUEWRAPPER_H_ */

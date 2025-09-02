#ifndef __DEVICE_TIMER_H
#define __DEVICE_TIMER_H

void timer_init(void);
static void intr_timer_handler(void);
static void ticks_to_sleep(uint32_t sleep_ticks);
void mtime_sleep(uint32_t m_seconds);

#endif
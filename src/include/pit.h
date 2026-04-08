#ifndef INCLUDE_PIT_H
#define INCLUDE_PIT_H

void         pit_init(unsigned int freq_hz);
void         pit_handler_c(void);
unsigned int pit_get_ticks(void);
void         sleep_ticks(unsigned int t);

#endif /* INCLUDE_PIT_H */

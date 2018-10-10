

#ifndef SNTP_TIME_H
#define SNTP_TIME_H

/*
 * Initialize the SNTP server
 */
void initialize_sntp(void);
/*
 * get the time from NTP server
 */
void obtain_time(void);
/*
 *
 */
uint64_t getTimeUs();

#endif /* SNTP_TIME_H */
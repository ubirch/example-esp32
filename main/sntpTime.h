/*!
 * @file    sntpTime.h
 * @brief   Synchronize the system clock with the SNTP time and provide
 *          functions to read the current time
 *
 * @author Waldemar Gruenwald
 * @date   2018-10-10
 *
 * @copyright &copy; 2018 ubirch GmbH (https://ubirch.com)
 *
 * ```
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ```
 */


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
 * Get the current time in microseconds accuracy
 */
uint64_t getTimeUs();

#endif /* SNTP_TIME_H */
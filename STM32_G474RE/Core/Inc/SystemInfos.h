/*******************************************************************
 * SystemInfos.h
 *
 *  Created on: 14 avr. 2020
 *      Author: Jack Lestrohan
 *
 *      Very low priority task to fill up a struct tat contains all
 *      informations about the current processor
 *      	- UUID
 *      	- flash size
 *      	- internal temperature
 *      	- Internal voltage
 *
 *******************************************************************/

#ifndef INC_SYSTEMINFOS_H_
#define INC_SYSTEMINFOS_H_

typedef struct {

} NUCLEOG474RE_CoreInfo_t;
NUCLEOG474RE_CoreInfo_t NUCLEOG474RE_CoreInfo;

#endif /* INC_SYSTEMINFOS_H_ */

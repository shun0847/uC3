/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, Network
    Network Bridge - IEEE 802.1D Implementataion
    Copyright (c) 2015, eForce Co., Ltd. All rights reserved.

    Version Information
      2015.09.28: Created
 ***************************************************************************/

#ifndef BRG_HAL_H_
#define BRG_HAL_H_

void hal_flush_db(UH portNum);
void hal_discard_port(UH portNum);
void hal_forward_port(UH portNum);
void hal_learn_port(UH portNum);
void hal_get_mac(UH portNum, UB *mac);
void hal_add_entry(UH portNum, UB *mac);
void hal_rmv_entry(UH portNum, UB *mac);
void hal_get_entry(UH portNum, UB *mac);
void hal_set_ageing(UH portNum, UH ageingTime);
void hal_get_ageing(UH portNum, UH *ageingTime);
void hal_get_pathCost(UH portNum, UW *cost);
void hal_get_portState(UH portNum, UB *state);

#define PORT_LINK_UP    1  /*#define  PORT_STATE_UP    1 */
#define PORT_LINK_DOWN  0  /*#define  PORT_STATE_DOWN  0 */
#endif /* BRG_HAL_H_ */

/* SECU-3  - An open source, free engine control unit
   Copyright (C) 2007 Alexey A. Shabelnikov. Ukraine, Gorlovka

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   contacts:
              http://secu-3.org
              email: shabelnikov@secu-3.org
*/

/** \file ignlogic.h
 * Logic determining calculation and regulation of anvance angle
 * (������ ������������ ���������� � ������������� ���� ����������).
 */

#ifndef _IGNLOGIC_H_
#define _IGNLOGIC_H_

#include <stdint.h>

struct ecudata_t;

/** Start mode of engine (used by state machine) */
#define EM_START 0

/** Idle mode of engine (used by state machine) */
#define EM_IDLE  1

/** Work mode of engine (used by state machine) */
#define EM_WORK  2

/**Implements state machine of engine's modes (�������� ������� ������� ���������)
 * \param padvance_angle_inhibitor_state pointer to data structure storing information about inhibition
 *        of alternating speed of advane angle
 * \param d pointer to ECU data structure
 * \return advance angle
 */
int16_t advance_angle_state_machine(int16_t* padvance_angle_inhibitor_state, struct ecudata_t* d);

#endif //_IGNLOGIC_H_

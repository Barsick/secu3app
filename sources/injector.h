/* SECU-3  - An open source, free engine control unit
   Copyright (C) 2007 Alexey A. Shabelnikov. Ukraine, Kiev

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

/** \file injector.h
 * Fuel injector control
 * (���������� ��������� ���������).
 */

#ifndef _INJECTOR_H_
#define _INJECTOR_H_

#ifdef FUEL_INJECT

#include <stdint.h>

/**Initialization of injector module (hardware & variables)*/
void inject_init_state(void);

/** Initialization of used I/O ports */
void inject_init_ports(void);

/** Set number of engine cylinders
 * \param cylnum Nymber of engine cylinders to set
 */
void inject_set_cyl_number(uint8_t cylnum);

/** Set number of squirts per 1 cycle
 * numsqr Number of squirts per cycle
 */
void inject_set_num_squirts(uint8_t numsqr);

/**Set injection time
 * \param time Injection time, one tick = 3.2us
 */
void inject_set_inj_time(uint16_t time);

/**Start injection (open injector for specified time).
 * This function must be called synchronously with crankshaft
 */
void inject_start_inj(void);

/** This function directly opens injectors, used for priming pulse (before cranking)
 * \param time Injection time, one tick = 3.2us
 */
void inject_open_inj(uint16_t time);

#endif //FUEL_INJECT

#endif //_INJECTOR_H_

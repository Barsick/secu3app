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

/** \file lambda.h
 * \author Alexey A. Shabelnikov
 * Correction algorithms using an exhaust gas oxygen sensor
 */

#ifndef _LAMBDA_H_
#define _LAMBDA_H_

#if defined(FUEL_INJECT) || defined(CARB_AFR) || defined(GD_CONTROL)

struct ecudata_t;

/** Initialization of state variables
 */
void lambda_init_state(void);

/**Control of lambda correction
 * \param d pointer to ECU data structure
 */
void lambda_control(struct ecudata_t* d);

/** Must be called from the main loop to notify about stroke events
 * \param d pointer to ECU data structure
 */
void lambda_stroke_event_notification(struct ecudata_t* d);

/** Check for activation of lambda sensor (heated-up)
 * \return 1 - activated, 0 - still not activated
 */
uint8_t lambda_is_activated(void);

#endif

#endif //_LAMBDA_H_

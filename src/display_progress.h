/**
 * u3-tool - U3 USB stick manager
 * Copyright (C) 2007 Daviedev, daviedev@users.sourceforge.net
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */ 
#ifndef __DISPLAY_PROGRESS_H__
#define __DISPLAY_PROGRESS_H__

#define PROGRESS_BAR_WIDTH 50 // width of the progress bar on screen

/**
 * Display progress of an operation
 *
 * @param cur		Number of current itteration
 * @param total		Total number of iterations
 */
void display_progress(unsigned int cur, unsigned int total);
 
#endif // __DISPLAY_PROGRESS_H__

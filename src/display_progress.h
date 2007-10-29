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

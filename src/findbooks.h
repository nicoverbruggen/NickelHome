#ifndef NHM_FINDBOOKS_H
#define NHM_FINDBOOKS_H
#include <QWidget>
extern void (*nhm_find_read_configure)(QWidget *);
extern void (*nhm_go_shopping)(void *);
void nhm_find_books_init();
void nhm_find_books_update();
#endif

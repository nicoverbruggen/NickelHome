#ifndef NHM_MYBOOKS_H
#define NHM_MYBOOKS_H

#include <QVector>

class QWidget;
class Volume;
struct nh_dlsym;

extern void (*nhm_my_books_configure)(QWidget *, QVector<Volume> &, int);
extern struct nh_dlsym nhm_my_books_symbols[];

#endif

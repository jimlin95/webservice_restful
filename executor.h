#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_
#include "mongoose.h"
struct executor
{
    void post(struct mg_connection *conn);
};
#endif

#ifndef RUNNER_H
#define RUNNER_H

#include "./ScriptRunner.h"
#include "./init/init.h"
#include "./webserver/webserver.h"

void setupRunner();
void loopRunner(uint32_t now);
void eventRunner();

extern ScriptRunner scriptRunner;

#endif
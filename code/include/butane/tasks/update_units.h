// This file is part of Butane. See README.md and LICENSE.md for details.
// Copyright (c) 2012 Michael Williams <devbug@bitbyte.ca>

#ifndef _BUTANE_TASKS_UPDATE_UNITS_H_
#define _BUTANE_TASKS_UPDATE_UNITS_H_

#include <butane/butane.h>
#include <butane/task.h>

namespace butane {
  class World;
namespace Tasks {
  struct UpdateUnitsData {
    World* world;
    float dt;
  };

  extern BUTANE_EXPORT void update_units(
    Task* task,
    uintptr_t data );
} // Tasks
} // butane

#endif // _BUTANE_TASKS_UPDATE_UNITS_H_

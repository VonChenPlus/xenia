/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef ALLOY_RUNTIME_DEBUGGER_H_
#define ALLOY_RUNTIME_DEBUGGER_H_

#include <alloy/core.h>


namespace alloy {
namespace runtime {

class Runtime;


class Breakpoint {
public:
  enum Type {
    TEMP_TYPE,
    CODE_TYPE,
  };
public:
  Breakpoint(Type type, uint64_t address);
  ~Breakpoint();

  Type type() const { return type_; }
  uint64_t address() const { return address_; }

private:
  Type type_;
  uint64_t address_;
};


class Debugger {
public:
  Debugger(Runtime* runtime);
  ~Debugger();

  Runtime* runtime() const { return runtime_; }

  int AddBreakpoint(Breakpoint* breakpoint);
  int RemoveBreakpoint(Breakpoint* breakpoint);

private:
  Runtime* runtime_;
};


}  // namespace runtime
}  // namespace alloy


#endif  // ALLOY_RUNTIME_DEBUGGER_H_
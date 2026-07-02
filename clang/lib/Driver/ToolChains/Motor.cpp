#include "Motor.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;

Tool *Motor::buildLinker() const {
  return new tools::gnutools::Linker(*this);
}


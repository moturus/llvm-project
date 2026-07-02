//===--- Motor.h - Motor OS ToolChain -----------------------*- C++ -*-===//
#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MOTOR_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MOTOR_H

#include "Gnu.h"

namespace clang {
namespace driver {
namespace toolchains {

class LLVM_LIBRARY_VISIBILITY Motor : public Generic_ELF {
public:
  Motor(const Driver &D, const llvm::Triple &Triple,
        const llvm::opt::ArgList &Args)
      : Generic_ELF(D, Triple, Args) {}

  bool HasNativeLLVMSupport() const override { return true; }
  bool isPICDefault() const override { return true; }
  bool isPIEDefault(const llvm::opt::ArgList &Args) const override {
    return true;
  }
  bool isPICDefaultForced() const override { return false; }
  const char *getDefaultLinker() const override { return "ld.lld"; }
  RuntimeLibType GetDefaultRuntimeLibType() const override {
    return ToolChain::RLT_CompilerRT;
  }
  CXXStdlibType GetDefaultCXXStdlibType() const override {
    return ToolChain::CST_Libcxx;
  }

protected:
  Tool *buildLinker() const override;
};

} // namespace toolchains
} // namespace driver
} // namespace clang

#endif


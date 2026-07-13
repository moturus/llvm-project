//===--- Motor.h - Motor OS ToolChain -----------------------*- C++ -*-===//
#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MOTOR_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MOTOR_H

#include "Gnu.h"

namespace clang {
namespace driver {
namespace tools {
namespace motor {

class LLVM_LIBRARY_VISIBILITY Linker final : public Tool {
public:
  Linker(const ToolChain &TC) : Tool("motor::Linker", "ld.lld", TC) {}

  bool hasIntegratedCPP() const override { return false; }
  bool isLinkJob() const override { return true; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};

} // namespace motor
} // namespace tools

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

  void
  AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                            llvm::opt::ArgStringList &CC1Args) const override;
  void AddClangCXXStdlibIncludeArgs(
      const llvm::opt::ArgList &DriverArgs,
      llvm::opt::ArgStringList &CC1Args) const override;

protected:
  Tool *buildLinker() const override;
};

} // namespace toolchains
} // namespace driver
} // namespace clang

#endif

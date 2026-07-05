#include "Motor.h"
#include "clang/Driver/CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Options/Options.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

// Motor executables are fully static PIEs entered at motor_start, linked
// against mlibc + the moto-rt-cabi shim (see the porting guide, appendices
// A and J). This ConstructJob owns the whole recipe so that plain
// `clang hello.c -o hello` works, both cross (host, --sysroot) and natively
// on the image. -nostdlib/-nostartfiles/-nodefaultlibs opt out as usual —
// the explicit-link recipes from appendices A..I keep working unchanged.
void tools::motor::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                        const InputInfo &Output,
                                        const InputInfoList &Inputs,
                                        const ArgList &Args,
                                        const char *LinkingOutput) const {
  const auto &TC = getToolChain();
  const Driver &D = TC.getDriver();
  ArgStringList CmdArgs;

  // Silence "argument unused" for flags that don't reach the linker.
  Args.ClaimAllArgs(options::OPT_g_Group);
  Args.ClaimAllArgs(options::OPT_emit_llvm);
  Args.ClaimAllArgs(options::OPT_w);
  // Static PIE is the only link mode; claim the flags that ask for it.
  Args.ClaimAllArgs(options::OPT_static_pie);
  Args.ClaimAllArgs(options::OPT_static);
  Args.ClaimAllArgs(options::OPT_pie);

  CmdArgs.push_back("-m");
  CmdArgs.push_back("elf_x86_64");
  CmdArgs.push_back("-static");
  CmdArgs.push_back("-pie");
  CmdArgs.push_back("--no-dynamic-linker");
  CmdArgs.push_back("-z");
  CmdArgs.push_back("text");
  CmdArgs.push_back("-z");
  CmdArgs.push_back("noexecstack");
  CmdArgs.push_back("--pack-dyn-relocs=none");
  CmdArgs.push_back("--eh-frame-hdr");
  CmdArgs.push_back("-e");
  CmdArgs.push_back("motor_start");

  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  // The on-image toolchain lives under /sys/tools/llvm (headers + libs), not
  // the classic /usr; see docs/porting-libc/dirs.md. D.SysRoot is empty on the
  // image and the cross sysroot on the host, so both resolve correctly.
  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles,
                   options::OPT_r))
    CmdArgs.push_back(
        Args.MakeArgString(D.SysRoot + "/sys/tools/llvm/lib/crt1.o"));

  Args.addAllArgs(CmdArgs, {options::OPT_L, options::OPT_u});
  TC.AddFilePathLibArgs(Args, CmdArgs);
  CmdArgs.push_back(
      Args.MakeArgString("-L" + D.SysRoot + "/sys/tools/llvm/lib"));

  AddLinkerInputs(TC, Inputs, Args, CmdArgs, JA);

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs,
                   options::OPT_r)) {
    // One group: mlibc, the shim, and the builtins have mutual references.
    // Order still matters for duplicate definitions: lld resolves a lazy
    // symbol from the first archive in scan order, and the shim's
    // VDSO-integrated __cxa_thread_atexit must win over libc++abi's
    // pthread-key fallback (dtor ordering interlocks with emutls cleanup),
    // so -lmoto_rt_cabi precedes the C++ libs.
    CmdArgs.push_back("--start-group");
    CmdArgs.push_back("-lmoto_rt_cabi");
    if (D.CCCIsCXX() && TC.ShouldLinkCXXStdlib(Args)) {
      CmdArgs.push_back("-lc++");
      CmdArgs.push_back("-lc++abi");
    }
    // -lunwind unconditionally: _Unwind_* can be referenced from C with
    // -fexceptions/_Unwind_Backtrace, and an unreferenced archive is free.
    CmdArgs.push_back("-lunwind");
    CmdArgs.push_back("-lc");
    CmdArgs.push_back("-lclang_rt.builtins-x86_64");
    CmdArgs.push_back("--end-group");
  }

  // The linker binary: prefer a real ld.lld (host cross builds find it next
  // to clang); if there is none, re-invoke this same binary with the
  // "ld.lld" subcommand — the Motor image ships one multicall `llvm`
  // executable and motor-fs has no symlinks to alias it (spawned children
  // always get the resolved exe path as argv[0], so the busybox trick is
  // unavailable; the llvm-driver's subcommand dispatch is the reliable
  // route).
  std::string Linker = TC.GetLinkerPath();
  const char *Exec;
  if (llvm::sys::fs::exists(Linker)) {
    Exec = Args.MakeArgString(Linker);
  } else {
    Exec = Args.MakeArgString(D.getDriverProgramPath());
    CmdArgs.insert(CmdArgs.begin(), "ld.lld");
  }

  C.addCommand(std::make_unique<Command>(JA, *this,
                                         ResponseFileSupport::AtFileCurCP(),
                                         Exec, CmdArgs, Inputs, Output));
}

Tool *Motor::buildLinker() const { return new tools::motor::Linker(*this); }

// Motor's sysroot layout is <sysroot>/sys/tools/llvm/include{,/c++/v1}
// (see docs/porting-libc/dirs.md). Adding the
// paths here (not via config-file -isystem) keeps the search order right:
// driver-added system includes always follow user -isystem flags, and the
// C++ stdlib dir precedes the C dir — libc++'s wrapper headers depend on
// that for #include_next (found the hard way: mbstate_t, appendix J).
void Motor::AddClangSystemIncludeArgs(const ArgList &DriverArgs,
                                      ArgStringList &CC1Args) const {
  const Driver &D = getDriver();

  if (DriverArgs.hasArg(options::OPT_nostdinc))
    return;

  if (!DriverArgs.hasArg(options::OPT_nobuiltininc)) {
    SmallString<128> P(D.ResourceDir);
    llvm::sys::path::append(P, "include");
    addSystemInclude(DriverArgs, CC1Args, P);
  }

  if (DriverArgs.hasArg(options::OPT_nostdlibinc))
    return;

  addExternCSystemInclude(DriverArgs, CC1Args,
                          D.SysRoot + "/sys/tools/llvm/include");
}

void Motor::AddClangCXXStdlibIncludeArgs(const ArgList &DriverArgs,
                                         ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(options::OPT_nostdinc, options::OPT_nostdlibinc,
                        options::OPT_nostdincxx))
    return;

  if (GetCXXStdlibType(DriverArgs) == ToolChain::CST_Libcxx)
    addSystemInclude(DriverArgs, CC1Args,
                     getDriver().SysRoot + "/sys/tools/llvm/include/c++/v1");
}

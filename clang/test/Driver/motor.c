// RUN: %clang --no-default-config --target=x86_64-unknown-motor \
// RUN:   --sysroot=/motor-root -### %s 2>&1 \
// RUN:   | FileCheck %s
// RUN: %clang --no-default-config --target=x86_64-unknown-motor --driver-mode=g++ \
// RUN:   --sysroot=/motor-root -### -c %s 2>&1 | FileCheck %s --check-prefix=CXX

// CHECK: "-internal-externc-isystem" "/motor-root/devtools/llvm/include"
// CHECK: "/motor-root/devtools/llvm/lib/crt1.o"
// CHECK: "-L/motor-root/devtools/llvm/lib"
// CHECK-NOT: "/sys/tools/llvm"

// CXX: "-internal-isystem" "/motor-root/devtools/llvm/include/c++/v1"
// CXX: "-internal-externc-isystem" "/motor-root/devtools/llvm/include"
// CXX-NOT: "/sys/tools/llvm"

int main(void) { return 0; }

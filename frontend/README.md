编译toy：```clang++ -O3 toy.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o toy```

用toy前端处理`example`：`./toy example`

Codegen时需要这些头文件：`llvm/IR/Verifier.h` `llvm/IR/DerivedTypes.h`
`llvm/IR/IRBuilder.h` `llvm/IR/LLVMContext.h` `llvm/IR/Module.h`

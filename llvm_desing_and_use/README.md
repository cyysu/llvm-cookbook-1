指令合并优化：`opt -S -instcombine testfile.ll -o output1.ll`

无用参数删除优化：`opt -S -deadargelim testfile.ll -o output2.ll`

将C代码转换成IR：`clang -emit-llvm -S multiply.c -o multiply.ll`

IR转换为bitcode：`llvm-as test.ll -o test.bc`

将bitcode转换为目标平台汇编：`llc test.bc -o test.s`

bitcode转换为IR：`llvm-dis test.bc -o test.ll`

内存优化：`opt -mem2reg -S multiply.ll -o multiply1.ll`

链接多个bitcode：`llvm-link test1.bc test2.bc -o output.bc`

执行bitcode：`lli output.bc`

调用预处理命令：`clang test.c -E`

生成ast：`clang -cc1 test.c -ast-dump`

输出到标准输出：`xxx -o -`

直接生成机器汇编：`clang -S test.c -o -`

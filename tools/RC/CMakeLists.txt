
if(DEFINED IN_SOURCE_BUILD)
    set(LLVM_LINK_COMPONENTS BitWriter Core IPO IrReader InstCombine Instrumentation Target Linker Analysis ScalarOpts Support Rc Svf Cudd)
    
    add_llvm_tool(rc StaticAnalysis/rc.cpp)
    add_llvm_tool(rcinstr RCInstr/rcinstr.cpp)

else()
    llvm_map_components_to_libnames(llvm_libs BitWriter Core IPO IrReader InstCombine Instrumentation Target Linker Analysis ScalarOpts Support)
    
    add_executable(rc StaticAnalysis/rc.cpp)
    add_executable(rcinstr RCInstr/rcinstr.cpp)

    target_link_libraries(rc librc LLVMSvf LLVMCudd ${llvm_libs})
    target_link_libraries(rcinstr librc LLVMSvf LLVMCudd ${llvm_libs})

    set_target_properties(rc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(rcinstr PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    
endif()


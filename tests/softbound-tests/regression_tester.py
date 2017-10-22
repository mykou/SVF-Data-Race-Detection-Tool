#! /usr/bin/python

import sys, string, os, popen2, difflib, platform, shutil

test_args={
    "byval_test_ptrs.c": " 1 2 ",
    "byval_test_nptrs.c" : " 1 2 ",
    "byval_test3.c" : "1 2",
    "byval_test4.c" : "1 2",
    "byval_test5.c" : "1 2",
    "byval_test6.c" : "1 2",
    "byval_test7.c" : "1 2",
    "byval_test8.c" : "1 2",
    "buffer-001.c" : " 1 2",
    "basic-00180-min.c" : "1 10 32 34 12",
    "basic-00180-med.c" : "1 10 15 34 12",
    "basic-00180-large.c" : "1 10 32 3300 12",
    
};

do_unsafe_byval = False;

litmus_tests=[
    # "byval_test_ptrs.c", ####### This unittest tests the byval with pointers
    # "byval_test_nptrs.c",
    # "byval_test3.c", ######## This unittest has struct with nested structures
    # "byval_test4.c", ######## This unittest has struct with nested structures with pointers
    # "byval_test5.c", ######## This unittest has struct with nested structurse
    # "byval_test6.c", ###### unitests with arrays and byval
    # "byval_test7.c",
    # "byval_test8.c"
#    "buffer-001.c", ######safecode imported test
    
#    "basic-00019-min.c", ########test case for shrinking bounds, current we are not doing it so we allow it"
#    "basic-00028-min.c",  ########test case for shrinking bounds, current we are not doing it so we allow it"
#    "basic-00023-min.c",   ########test case for shrinking bounds, current we are not doing it so we allow it"
#    "basic-00021-min.c", #######test case for shrinking bounds, we allow it for now
#    "basic-00030-min.c", #######test case for shrinking bounds, we allow it for now
#    "basic-0024-min.c" ,#######test case for shrinking bounds, we allow it for now
#    "basic-0022-min.c" ,#######test case for shrinking bounds, we allow it for now

#    "basic-00032-min.c", ###we do allow the program to write to padding area
#    "basic-00180-min.c"
#    "basic-00180-med.c",
#    "basic-00180-large.c",
    # "basic-00046-min.c", ######## these are strcpy tests 
    # "basic-00046-med.c",
    # "basic-00046-large.c",
    # "basic-00046-ok.c",
    # "basic-00012-min.c",
    # "basic-00012-med.c",
    # "basic-00012-large.c",
    # "basic-00047-min.c", ######strncpy
    # "basic-00047-med.c",
    # "basic-00047-large.c",
    # "basic-00050-min.c", ########strncpy
    # "basic-00050-med.c",
    # "basic-00050-large.c",
    # "basic-00015-min.c",
    # "basic-00015-med.c",
    # "basic-00015-large.c",
    # "basic-00016-min.c",
    # "basic-00016-med.c",
    # "basic-00016-large.c",
    # "basic-00051-min.c",
    # "basic-00051-med.c",
    # "basic-00051-large.c",
    # "basic-00051-ok.c",
    # "basic-00048-min.c",
    # "basic-00048-med.c",
    # "basic-00048-large.c",
    # "basic-00048-ok.c",
    # "basic-00011-min.c",
    # "basic-00011-med.c",
    # "basic-00011-large.c",
    # "basic-00011-ok.c",
    # "basic-00049-min.c",
    # "basic-00049-med.c",
    # "basic-00049-large.c",
    # "basic-00049-ok.c",
#    "basic-00183-min.c", ######tests getcwd
    # "basic-00045-min.c",
    # "basic-00045-med.c",
    # "basic-00045-large.c",
    # "basic-00045-ok.c",

#    "memcpy-test-ok.c",
#     "memcpy-test.c",

#    "basic-00288-ok.c",
     "basic-00288-min.c",
    "basic-00288-med.c",
    "basic-00288-large.c",

    "basic-00289-ok.c",
    "basic-00289-min.c",
    "basic-00289-med.c",
    "basic-00289-large.c",
    
    "basic-00290-ok.c",
    "basic-00290-min.c",
    "basic-00290-large.c",
    "basic-00290-med.c",
    
    "basic-00291-ok.c",
    "basic-00291-min.c",
    "basic-00291-med.c",
    "basic-00291-large.c",
    
    # "memset-test-ok.c",
    # "memset-test.c",
    
    # "buffer-002.c",
    # "double_free-007.c",
    # "double_free-008.c",
    # "double_free-015.c",
    # "double_free-018.c",
    # "double_free-020.c",
    # "double_free-022.c",
    
    # "illegal_cast-004.c",
    # "use_after_free-004.c",

    
    
    
#    "malloc_test.c",
#    "memcpy_boundaries.c",

#    "memcpy_bnd_load.c",
#    "memcpy_struct.c",
#    "realloc_test.c",
#    "tar-wrappers.c",
#    "wilander_tests/dynamic_testbed_john_wilander.c"    
]

special_run={
    "wilander_tests/dynamic_testbed_john_wilander.c": "../wilander.py",
}


do_verbose_mode=True
do_clang_mode = False
do_spatial_temporal=True
do_spatial=False
do_temporal=False
do_debug = False


clang_path="/mnt/eclipse/acg/users/santoshn/softbound-safecode-repo/safecode-build/"
softbound_cets_lib_path= os.environ["SOFTBOUNDCETS_LIB_PATH"] + "/"
softbound_cets_bin_path= os.environ["SOFTBOUNDCETS_BIN_PATH"] + "/"
os.environ["PATH"] = "/mnt/eclipse/acg/users/santoshn/llvm-svn-trunk/llvm-3.0-release-obj/Debug+Asserts/bin:" + os.environ["PATH"]
os.environ["PATH"] = clang_path+ "Debug+Asserts/bin:" + os.environ["PATH"]



def run_command(command):
    if do_verbose_mode:
        print command
    exit_code  = os.system(command)
    if exit_code != 0:
        print "Error in running command:", command
        print "Error is: ", exit_code

    return exit_code
    
def generate_bitcode_clang(file_path_name, file_name, cflags):
    build_string = ""
    build_string = "clang  -c -std=gnu89 -emit-llvm " + cflags + " " + file_path_name + " -o " + file_name + ".bc"
    run_command(build_string)

def run_tests(test):
    print os.getcwd()
    args = ""
    if test in test_args:
        args = test_args[test];
    
    if test in special_run:
        run_command("python " + special_run[test])
    else:        
        run_command("./final " + args)

    

def handle_clang_mode(test):

    build_string = " clang -fsoftbound " + "../"+test + " -o final  -L" + clang_path+"Debug+Asserts/lib "
    run_command(build_string)
    
    run_tests(test)

def handle_spatial_temporal(test):
    
    
    debug_flags = ""

    if do_debug:
        debug_flags = " -g "

    spatial_temporal_flags = debug_flags + "-I" + softbound_cets_lib_path + " -D__SOFTBOUNDCETS_TRIE -D__SOFTBOUNDCETS_SPATIAL_TEMPORAL "
    
    generate_bitcode_clang(softbound_cets_lib_path + "softboundcets-checks.c", "checks", spatial_temporal_flags)

    generate_bitcode_clang(softbound_cets_lib_path + "softboundcets-wrappers.c", "wrappers", spatial_temporal_flags)

    generate_bitcode_clang(softbound_cets_lib_path + "softboundcets.c", "main", spatial_temporal_flags);
    
    generate_bitcode_clang("../" + test, "test", spatial_temporal_flags);
 
    # build_string_link = "llvm-link  test.bc  checks.bc > test-link.bc "
    # run_command(build_string_link)

    byval_args =  ""
    if do_unsafe_byval:
        byval_args = " -unsafe_byval_opt "

    build_string_softboundcets = softbound_cets_bin_path + "SoftBoundCETS   " + byval_args + " test.bc "
    exit_code = run_command(build_string_softboundcets)

    if exit_code != 0:
        exit(1);

    build_string_final_link = " llvm-link test.bc.sbpass.bc wrappers.bc main.bc > final.bc "
    run_command(build_string_final_link)

    disable_opt_flags = " "
    if do_debug:
        disable_opt_flags = " -disable-opt "
    build_string_native = "llvm-ld -native " + disable_opt_flags + " final.bc -o final -lm -lrt "
    run_command(build_string_native)
    
    run_tests(test)

    return 0;



def handle_spatial():
    
    return

def handle_temporal():

    return

if not os.path.exists("obj"): 
    os.mkdir("obj")

os.chdir("obj");


for test in litmus_tests:
    
    if do_clang_mode:
        handle_clang_mode(test);
    
    if do_spatial_temporal:
        handle_spatial_temporal(test);
        
    if do_spatial:
        handle_spatial(test);
            
    if do_temporal:
        handle_temporal(test);

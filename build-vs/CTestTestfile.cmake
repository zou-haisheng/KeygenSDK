# CMake generated Testfile for 
# Source directory: C:/Users/19697/Documents/KeygenSDK/KeygenSDK
# Build directory: C:/Users/19697/Documents/KeygenSDK/KeygenSDK/build-vs
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[KeygenSDKTests]=] "C:/Users/19697/Documents/KeygenSDK/KeygenSDK/build-vs/Debug/KeygenSDKTests.exe")
  set_tests_properties([=[KeygenSDKTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/19697/Documents/KeygenSDK/KeygenSDK/CMakeLists.txt;87;add_test;C:/Users/19697/Documents/KeygenSDK/KeygenSDK/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[KeygenSDKTests]=] "C:/Users/19697/Documents/KeygenSDK/KeygenSDK/build-vs/Release/KeygenSDKTests.exe")
  set_tests_properties([=[KeygenSDKTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/19697/Documents/KeygenSDK/KeygenSDK/CMakeLists.txt;87;add_test;C:/Users/19697/Documents/KeygenSDK/KeygenSDK/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[KeygenSDKTests]=] "C:/Users/19697/Documents/KeygenSDK/KeygenSDK/build-vs/MinSizeRel/KeygenSDKTests.exe")
  set_tests_properties([=[KeygenSDKTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/19697/Documents/KeygenSDK/KeygenSDK/CMakeLists.txt;87;add_test;C:/Users/19697/Documents/KeygenSDK/KeygenSDK/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[KeygenSDKTests]=] "C:/Users/19697/Documents/KeygenSDK/KeygenSDK/build-vs/RelWithDebInfo/KeygenSDKTests.exe")
  set_tests_properties([=[KeygenSDKTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/19697/Documents/KeygenSDK/KeygenSDK/CMakeLists.txt;87;add_test;C:/Users/19697/Documents/KeygenSDK/KeygenSDK/CMakeLists.txt;0;")
else()
  add_test([=[KeygenSDKTests]=] NOT_AVAILABLE)
endif()

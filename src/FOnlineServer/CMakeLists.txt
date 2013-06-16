# Copyright (c) 2013 Alexander Konovalov. All rights reserved.
# Use of this source code is governed by a GPL-style license that can be
# found in the LICENSE file.

project(FOnlineServer)

file(GLOB APP_HXX
  "*.h"
  "*.hpp"
  "math/*.hpp"
)

file(GLOB APP_CXX
  "*.cpp"
  "math/*.cpp"
)

# List all the *_test.cpp files in the alphabetic order.
set(TEST_CXX

)

add_executable(FOnlineServer ${APP_CXX} ${APP_HXX})
target_link_libraries(FOnlineServer FOnlineCommon FOnlineFileManager IniFile mysqlclient)
include_directories(..)
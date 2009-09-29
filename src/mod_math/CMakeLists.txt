set(MOD_MATH_SRCS
  CVector.cpp
  intgr.cpp
  linearreg.cpp
  matrix33.cpp
  pt1.cpp
  quaternion.cpp
  ratelim.cpp
  vector3.cpp
  )
add_library(mod_math ${MOD_MATH_SRCS})

set (MOD_MATH_LIBS    )
set (MOD_MATH_INCDIRS )
    
link_directories      ( ${MOD_MATH_LINKDIRS} )

add_executable       (quat_test quat_test.cpp )
target_link_libraries(quat_test mod_math)


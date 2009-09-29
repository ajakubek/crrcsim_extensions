set(MOD_LANDSCAPE_SRCS
  crrc_scenery.cpp
  crrc_sky.cpp
  ssgLoadJPG.cpp
  )
add_library(mod_landscape ${MOD_LANDSCAPE_SRCS})

set (MOD_LANDSCAPE_LIBS    )
set (MOD_LANDSCAPE_INCDIRS )
    
include_directories(
  ${MOD_LANDSCAPE_EXT_INCDIRS}
)

link_directories      ( ${MOD_LANDSCAPE_LINKDIRS} )

set(MOD_CNTRL_SRCS
  cntrl_initinputs/cntrl_initinputs.cpp
  cntrl_phugoid/cntrl_phugoid.cpp
  cntrl_rateofclimb/cntrl_rateofclimb.cpp
  cntrl_setuserinput/cntrl_setuserinput.cpp
  controller.cpp
  )
add_library(mod_cntrl ${MOD_CNTRL_SRCS})

set (MOD_CNTRL_LIBS    )
set (MOD_CNTRL_INCDIRS )
    
link_directories      ( ${MOD_CNTRL_LINKDIRS} )

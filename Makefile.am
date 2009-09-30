SUBDIRS = documentation models objects packages scenery sounds textures

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = crrcsim
crrcsim_SOURCES = src/mod_mode/F3F/handlerF3F.h \
       src/mod_mode/F3F/handlerF3F.cpp \
       src/GUI/crrc_audio.h \
       src/GUI/crrc_calibmap.h \
       src/GUI/crrc_ctrldev.h \
       src/GUI/crrc_ctrlgen.h \
       src/GUI/crrc_dialog.h \
       src/GUI/crrc_f3f.h \
       src/GUI/crrc_gui_main.h \
       src/GUI/crrc_joy.h \
       src/GUI/crrc_launch.h \
       src/GUI/crrc_location.h \
       src/GUI/crrc_mousebutton.h \
       src/GUI/crrc_msgbox.h \
       src/GUI/crrc_planesel.h \
       src/GUI/crrc_scaleinput.h \
       src/GUI/crrc_slider.h \
       src/GUI/crrc_spin.h \
       src/GUI/crrc_video.h \
       src/GUI/crrc_windthermal.h \
       src/GUI/puaFileBox.h \
       src/GUI/puaGLPreview.h \
       src/GUI/puaScrListBox.h \
       src/GUI/util.h \
       src/GUI/crrc_audio.cpp \
       src/GUI/crrc_calibmap.cpp \
       src/GUI/crrc_ctrldev.cpp \
       src/GUI/crrc_ctrlgen.cpp \
       src/GUI/crrc_dialog.cpp \
       src/GUI/crrc_f3f.cpp \
       src/GUI/crrc_gui_main.cpp \
       src/GUI/crrc_joy.cpp \
       src/GUI/crrc_launch.cpp \
       src/GUI/crrc_location.cpp \
       src/GUI/crrc_mousebutton.cpp \
       src/GUI/crrc_msgbox.cpp \
       src/GUI/crrc_planesel.cpp \
       src/GUI/crrc_scaleinput.cpp \
       src/GUI/crrc_slider.cpp \
       src/GUI/crrc_spin.cpp \
       src/GUI/crrc_video.cpp \
       src/GUI/crrc_windthermal.cpp \
       src/GUI/puaFileBox.cpp \
       src/GUI/puaGLPreview.cpp \
       src/GUI/puaScrListBox.cpp \
       src/GUI/util.cpp \
       src/mod_chardevice/bufferedchardevice.h \
       src/mod_chardevice/chardevicebase.h \
       src/mod_chardevice/chardevicec.h \
       src/mod_chardevice/chardevicecommon.h \
       src/mod_chardevice/chardevice.h \
       src/mod_chardevice/chardevicewrapper.h \
       src/mod_chardevice/filechardevice.h \
       src/mod_chardevice/serialchardevice.h \
       src/mod_chardevice/socketchardevicebase.h \
       src/mod_chardevice/tcpchardevice.h \
       src/mod_chardevice/tcpserverchardevice.h \
       src/mod_chardevice/udpchardevice.h \
       src/mod_chardevice/udpserverchardevice.h \
       src/mod_chardevice/bufferedchardevice.cpp \
       src/mod_chardevice/chardevicebase.cpp \
       src/mod_chardevice/chardevicec.cpp \
       src/mod_chardevice/chardevicewrapper.cpp \
       src/mod_chardevice/filechardevice.cpp \
       src/mod_chardevice/serialchardevice.cpp \
       src/mod_chardevice/socketchardevicebase.cpp \
       src/mod_chardevice/tcpchardevice.cpp \
       src/mod_chardevice/tcpserverchardevice.cpp \
       src/mod_chardevice/udpchardevice.cpp \
       src/mod_chardevice/udpserverchardevice.cpp \
       src/mod_cntrl/controller.cpp \
       src/mod_cntrl/controller.h \
       src/mod_cntrl/cntrl_initinputs/cntrl_initinputs.cpp \
       src/mod_cntrl/cntrl_initinputs/cntrl_initinputs.h \
       src/mod_cntrl/cntrl_rateofclimb/cntrl_rateofclimb.cpp \
       src/mod_cntrl/cntrl_rateofclimb/cntrl_rateofclimb.h \
       src/mod_cntrl/cntrl_phugoid/cntrl_phugoid.cpp \
       src/mod_cntrl/cntrl_phugoid/cntrl_phugoid.h \
       src/mod_cntrl/cntrl_setuserinput/cntrl_setuserinput.cpp \
       src/mod_cntrl/cntrl_setuserinput/cntrl_setuserinput.h \
       src/mod_env/earth/atmos_62.h \
       src/mod_env/earth/ls_earth.h \
       src/mod_env/earth/ls_gravity.h \
       src/mod_env/earth/atmos_62.cpp \
       src/mod_env/earth/ls_gravity.cpp \
       src/mod_fdm/fdm_002/fdm_002.h \
       src/mod_fdm/fdm_002/fdm_002.cpp \
       src/mod_fdm/fdm_displaymode/fdm_displaymode.h \
       src/mod_fdm/fdm_displaymode/fdm_displaymode.cpp \
       src/mod_fdm/fdm_larcsim/fdm_larcsim.h \
       src/mod_fdm/fdm_larcsim/fdm_larcsim.cpp \
       src/mod_fdm/fdm_heli01/fdm_heli01.h \
       src/mod_fdm/fdm_heli01/fdm_heli01.cpp \
       src/mod_fdm/fdm_testmode/fdm_testmode.h \
       src/mod_fdm/fdm_testmode/fdm_testmode.cpp \
       src/mod_fdm/formats/airtoxml.h \
       src/mod_fdm/formats/airtoxml.cpp \
       src/mod_fdm/physics/eom.h \
       src/mod_fdm/physics/eom.cpp \
       src/mod_fdm/power/battery.h \
       src/mod_fdm/power/engine_dcm.h \
       src/mod_fdm/power/gearing.h \
       src/mod_fdm/power/power.h \
       src/mod_fdm/power/propeller.h \
       src/mod_fdm/power/shaft.h \
       src/mod_fdm/power/simplethrust.h \
       src/mod_fdm/power/values_step.h \
       src/mod_fdm/power/battery.cpp \
       src/mod_fdm/power/engine_dcm.cpp \
       src/mod_fdm/power/gearing.cpp \
       src/mod_fdm/power/power.cpp \
       src/mod_fdm/power/propeller.cpp \
       src/mod_fdm/power/shaft.cpp \
       src/mod_fdm/power/simplethrust.cpp \
       src/mod_fdm/fdm_env.h \
       src/mod_fdm/fdm.h \
       src/mod_fdm/fdm_inputs.h \
       src/mod_fdm/ls_geodesy.h \
       src/mod_fdm/ls_types.h \
       src/mod_fdm/xmlmodelfile.h \
       src/mod_fdm/fdm.cpp \
       src/mod_fdm/ls_geodesy.cpp \
       src/mod_fdm/xmlmodelfile.cpp \
       src/mod_fdm/gear01/gear.h \
       src/mod_fdm/gear01/gear.cpp \
       src/mod_inputdev/inputdev_audio/inputdev_audio.h \
       src/mod_inputdev/inputdev_audio/inputdev_audio.cpp \
       src/mod_inputdev/inputdev_mnav/inputdev_mnav.h \
       src/mod_inputdev/inputdev_mnav/mnav.h \
       src/mod_inputdev/inputdev_mnav/inputdev_mnav.cpp \
       src/mod_inputdev/inputdev_mnav/mnav.cpp \
       src/mod_inputdev/inputdev_parallel/inputdev_parallel.h \
       src/mod_inputdev/inputdev_parallel/inputdev_parallel.cpp \
       src/mod_inputdev/inputdev_PPM/inputdev_PPM.h \
       src/mod_inputdev/inputdev_PPM/inputdev_PPM.cpp \
       src/mod_inputdev/inputdev_rctran/inputdev_rctran.h \
       src/mod_inputdev/inputdev_rctran/mbuff.h \
       src/mod_inputdev/inputdev_rctran/inputdev_rctran.cpp \
       src/mod_inputdev/inputdev_rctran2/inputdev_rctran2.h \
       src/mod_inputdev/inputdev_rctran2/inputdev_rctran2.cpp \
       src/mod_inputdev/inputdev_serial/inputdev_serial.h \
       src/mod_inputdev/inputdev_serial/inputdev_serial.cpp \
       src/mod_inputdev/inputdev_serial2/inputdev_serial2.h \
       src/mod_inputdev/inputdev_serial2/LoggerReader_byte.h \
       src/mod_inputdev/inputdev_serial2/LoggerReader_ttyS.h \
       src/mod_inputdev/inputdev_serial2/inputdev_serial2.cpp \
       src/mod_inputdev/inputdev_serial2/LoggerReader_byte.cpp \
       src/mod_inputdev/inputdev_serial2/LoggerReader_ttyS.cpp \
       src/mod_inputdev/inputdev_serpic/inputdev_serpic.h \
       src/mod_inputdev/inputdev_serpic/inputdev_serpic.cpp \
       src/mod_inputdev/inputdev_socket/inputdev_socket.cpp \
       src/mod_inputdev/inputdev_socket/interface_socket.cpp \
       src/mod_inputdev/inputdev_software/inputdev_software.h \
       src/mod_inputdev/inputdev_software/inputdev_software.cpp \
       src/mod_inputdev/inputdev_zhenhua/inputdev_zhenhua.h \
       src/mod_inputdev/inputdev_zhenhua/inputdev_zhenhua.cpp \
       src/mod_inputdev/inputdev.h \
       src/mod_inputdev/inputdev.cpp \
       src/mod_landscape/crrc_scenery.h \
       src/mod_landscape/winddata3D.h \
       src/mod_landscape/crrc_sky.h \
       src/mod_landscape/crrc_scenery.cpp \
       src/mod_landscape/crrc_sky.cpp \
       src/mod_landscape/winddata3D.cpp \
       src/mod_landscape/ssgLoadJPG.cpp \
       src/mod_math/CVector.h \
       src/mod_math/intgr.h \
       src/mod_math/linearreg.h \
       src/mod_math/matrix33.h \
       src/mod_math/pt1.h \
       src/mod_math/quaternion.h \
       src/mod_math/ratelim.h \
       src/mod_math/vector3.h \
       src/mod_math/CVector.cpp \
       src/mod_math/intgr.cpp \
       src/mod_math/linearreg.cpp \
       src/mod_math/matrix33.cpp \
       src/mod_math/pt1.cpp \
       src/mod_math/quaternion.cpp \
       src/mod_math/ratelim.cpp \
       src/mod_math/vector3.cpp \
       src/mod_misc/crrc_rand.h \
       src/mod_misc/lib_conversions.h \
       src/mod_misc/ls_constants.h \
       src/mod_misc/scheduler.h \
       src/mod_misc/SimpleXMLTransfer.h \
       src/mod_misc/crrc_rand.cpp \
       src/mod_misc/lib_conversions.cpp \
       src/mod_misc/scheduler.cpp \
       src/mod_misc/filesystools.h \
       src/mod_misc/filesystools.cpp \
       src/mod_misc/SimpleXMLTransfer.cpp \
       src/mod_windfield/thermal03/solve.h \
       src/mod_windfield/thermal03/thconf.h \
       src/mod_windfield/thermal03/thermikschale.h \
       src/mod_windfield/thermal03/tschalen.h \
       src/mod_windfield/thermal03/solve.cpp \
       src/mod_windfield/thermal03/thermikschale.cpp \
       src/mod_windfield/thermal03/tschalen.cpp \
       src/mod_windfield/thermalprofile.h \
       src/mod_windfield/windfield.h \
       src/mod_windfield/windfield.cpp \
       src/config.h \
       src/crrc_animation.h \
       src/crrc_fdm.h \
       src/crrc_graphics.h \
       src/crrc_loadair.h \
       src/crrc_main.h \
       src/crrc_sound.h \
       src/crrc_soundserver.h \
       src/crrc_system.h \
       src/CTime.h \
       src/defines.h \
       src/glconsole.h \
       src/global_airplane.h \
       src/global_fdm.h \
       src/global.h \
       src/global_sound.h \
       src/gloverlay.h \
       src/ImageLoaderTGA.h \
       src/include_gl.h \
       src/mod_fdm_config.h \
       src/mod_windfield_config.h \
       src/mouse_kbd.h \
       src/SimStateHandler.h \
       src/mod_mode/T_GameHandler.h \
       src/zoom.h \
       src/config.cpp \
       src/crrc_animation.cpp \
       src/crrc_checkopts.cpp \
       src/crrc_fdm.cpp \
       src/crrc_graphics.cpp \
       src/crrc_keyboard.cpp \
       src/crrc_loadair.cpp \
       src/crrc_main.cpp \
       src/crrc_ssgutils.h \
       src/crrc_ssgutils.cpp \
       src/crrc_sound.cpp \
       src/crrc_soundserver.cpp \
       src/crrc_system.cpp \
       src/CTime.cpp \
       src/glconsole.cpp \
       src/global.cpp \
       src/gloverlay.cpp \
       src/ImageLoaderTGA.cpp \
       src/mouse_kbd.cpp \
       src/SimStateHandler.cpp \
       src/zoom.cpp \
       src/mod_main/eventhandler.h \
       src/mod_main/eventhandler.cpp \
       src/aircraft.h \
       src/aircraft.cpp

EXTRA_DIST = Doxyfile Makefile.linux Makefile.linux64 Makefile.macosx \
             Makefile.mingw Makefile.sound Makefile.win32 autogen.sh \
             src/mod_inputdev/inputdev_rctran2/kernel_module/Makefile.24 \
             src/mod_inputdev/inputdev_rctran2/kernel_module/Makefile.26 \
             src/mod_inputdev/inputdev_rctran2/kernel_module/rctran2.c \
             src/mod_inputdev/inputdev_rctran2/kernel_module/README.txt \
             CMakeLists.txt cmake/config.h.in cmake/test_plib.cpp cmake.sh \
             src/mod_math/quat_test.cpp \
             src/GUI/CMakeLists.txt \
             src/mod_main/CMakeLists.txt \
             src/mod_math/CMakeLists.txt \
             src/mod_misc/CMakeLists.txt \
             src/mod_mode/CMakeLists.txt \
             src/mod_cntrl/CMakeLists.txt \
             src/mod_chardevice/CMakeLists.txt \
             src/mod_env/CMakeLists.txt \
             src/mod_fdm/CMakeLists.txt \
             src/mod_landscape/CMakeLists.txt \
             src/mod_windfield/CMakeLists.txt \
             src/mod_inputdev/CMakeLists.txt \
             HISTORY

crrcsim_CXXFLAGS = $(GLU_CFLAGS) $(PA_CFLAGS) $(SDL_CFLAGS) -DPU_USE_SDL \
                   -DCRRC_DATA_PATH="\"$(datadir)/@PACKAGE@\""
crrcsim_LDADD = $(XTRA_OBJS) $(PA_LIBS) $(SDL_LIBS) \
                $(CGAL_LIBS) -ljpeg -lplibssg -lplibsg -lplibpuaux -lplibpu -lplibul -lplibfnt \
                $(GLU_LIBS)

crrcsim_DEPENDENCIES = $(XTRA_OBJS)

win32icon.rc: Makefile
	echo "A ICON MOVEABLE PURE LOADONCALL DISCARDABLE \"@srcdir@/packages/icons/crrcsim.ico\"" > win32icon.rc

win32icon.res: win32icon.rc Makefile
	windres.exe --input-format=rc -o win32icon.res -O coff win32icon.rc

win32package: all
	@echo "--- Creating Nullsoft installer package"
	strip --strip-all $(bin_PROGRAMS)
	cp $(srcdir)/packages/Win32/crrcsim.nsi .
	makensis crrcsim.nsi
	rm -f crrcsim.nsi


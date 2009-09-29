/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2008 Olivier Bordes (original author)
 * Copyright (C) 2005 Lionel Cailler
 * Copyright (C) 2005, 2009 Jens Wilhelm Wulf
 * Copyright (C) 2005, 2006, 2007, 2008 Jan Reucker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/**
 * \file handlerF3F.cpp
 *
 * Purpose: Add  F3F functions to crrcsim. 
 *          F3F is a FAI category which define slope soaring contest
 * History: crcsim-f3f has been historically developped on a separate branch of
 *          crrcsim 0.8.1 and for windows only. The last release of this specific
 *          branch is the version 1.5.1.
 */

#include "../../include_gl.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../../global.h"
#include "../../crrc_soundserver.h"
#include "../../crrc_graphics.h"
#include "../../crrc_system.h"
#include "../../mod_misc/ls_constants.h"
#include "handlerF3F.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/** \brief The default constructor
 *
 *  Creates an F3F game handler
 */
HandlerF3F::HandlerF3F() 
  : start_sound_id(-1), start_time(""), end_time(""), use_beep(false)
{
  /// \todo This file defines default values for all configuration
  ///       attributes. The GUI dialog (GUI/crrc_f3f.*) does the
  ///       same, so both should use a common function for
  ///       making sure that the config attributes exist and
  ///       contain at least a default value. See
  ///       CGUIF3FDialog::prepareConfigFile().
  
  /// \todo must add a check of the slope. 
  repeat_mode = 0;
  cfgfile->makeSureAttributeExists("game.f3f.security_line", "-4");
  security_line= cfgfile->getInt ("game.f3f.security_line");

  cfgfile->makeSureAttributeExists("game.f3f.extend_bases", "0");
  extend_base= cfgfile->getInt ("game.f3f.extend_bases");

  cfgfile->makeSureAttributeExists("game.f3f.plan_limit", "328");
  if (cfgfile->getInt ("game.f3f.plan_limit") == 0)
    plan_limit=PLANLIMIT/2;
  else 
    plan_limit=cfgfile->getInt("game.f3f.plan_limit") / 2;

  cfgfile->makeSureAttributeExists("game.f3f.start_left", "0");
  start_on_left= cfgfile->getInt("game.f3f.start_left");

  window_xsize = 0;
  window_ysize = 0; 

  cfgfile->makeSureAttributeExists("game.f3f.sound.dir", "sounds/f3f/default");
  set_sound_dir(cfgfile->getString("game.f3f.sound.dir"));
  /// \todo Add a check if the specified directory really
  ///       exists. If not, the sound dir should be reset
  ///       to sounds/f3f/default, and maybe there should
  ///       be a search algorithm for "good matches" if
  ///       no exact match is found (see GUI/crrc_f3f.cpp)

  f3f_soundStart    = "start";
  f3f_soundPenalty  = "penalty";
  f3f_soundEntry    = "entry";
  f3f_soundFirst    = "first";
  f3f_soundBase     = "base";
  f3f_soundLast     = "last";
  
  // states for OpenGL rendering
  pylon_rendering_state = new ssgSimpleState();
  pylon_rendering_state->enable(GL_CULL_FACE);
  pylon_rendering_state->disable(GL_COLOR_MATERIAL);
  pylon_rendering_state->disable(GL_TEXTURE_2D);
  pylon_rendering_state->enable(GL_LIGHTING);
  pylon_rendering_state->disable(GL_BLEND);

  text_rendering_state = new ssgSimpleState();
  text_rendering_state->disable(GL_CULL_FACE);
  text_rendering_state->disable(GL_COLOR_MATERIAL);
  text_rendering_state->disable(GL_TEXTURE_2D);
  text_rendering_state->disable(GL_LIGHTING);
  text_rendering_state->disable(GL_BLEND);
  text_rendering_state->setMaterial(GL_EMISSION, 0.0, 0.0, 0.0, 0.0);
  text_rendering_state->setMaterial(GL_AMBIENT, 1.0, 1.0, 1.0, 1.0);
  text_rendering_state->setMaterial(GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0);
  text_rendering_state->setMaterial(GL_SPECULAR, 0.0, 0.0, 0.0, 0.1);

  reset();
}


/** \brief The destructor.
 *
 *  Deletes an F3F game handler
 */
HandlerF3F::~HandlerF3F()
{
  if (start_sound_id >= 0)
  {
    Global::soundserver->stopChannel(start_sound_id);
  }
  delete pylon_rendering_state;
  delete text_rendering_state;
}


/** \brief Render game-mode-specific details
 *
 *  This method renders graphics objects that are specific to the
 *  F3F game mode, namely the base pylons.
 */
void HandlerF3F::draw()
{
  pylon_rendering_state->apply();
  draw_f3f_base (0,   plan_limit, 100);
  draw_f3f_base (10,  plan_limit, 100);
  draw_f3f_base (0,  -plan_limit, 100);
  draw_f3f_base (10, -plan_limit, 100);
}


/** \brief Print the game-mode-specific text overlay
 *
 *  This method renders the text overlay for the F3F mode.
 *
 *  \todo This method should use PLIB instead of GLUT for font rendering.
 *
 *  \param  ww  current OpenGL window width
 *  \param  hh  current OpenGL window height
 */
void HandlerF3F::display_infos(GLfloat ww, GLfloat hh)
{
  int  y_offset= 30;
  int i;
  int lineheight = 35;
  int itime = 0;
  int iturn = 0;
  int ilost = 0;
  char astring[256];
  
  window_xsize=ww;
  window_ysize=hh;

  startTextRendering();
  text_rendering_state->apply();
  
  if (ww <= 800)
  {
    fontRenderer.setFont(fntGetBitmapFont(FNT_BITMAP_HELVETICA_12));
  }
  else
  {
    fontRenderer.setFont(fntGetBitmapFont(FNT_BITMAP_TIMES_ROMAN_24));
  }

  if (run_started == FALSE ) 
  {
    // before start
    output (window_xsize/2 - textLength(f3f_score)/2,
            window_ysize - y_offset, 
            f3f_score);
  }
  if ((run_will_start == TRUE) && (run_started== FALSE))
  {
    output (window_xsize/2 - textLength(f3f_score)/2,
            window_ysize/2, 
            "IN START");
  }

  if (run_completed == TRUE)
  {
    // overall score
    sprintf (astring, "- SCORE -");
    output (window_xsize/2 - textLength(astring)/2,
            window_ysize - y_offset - 0*lineheight, 
            astring);
    output (window_xsize/2 - textLength(f3f_score)/2,
            window_ysize - y_offset - 1*lineheight, 
            f3f_score);

    // table header for lap statistics
    sprintf (astring,"- Lap  RunTime  TurnTime  LostMeters -");
    GLfloat header_length = textLength(astring);
    GLfloat a_width = textLength("a");            /// \todo rather use height("X") than width("a")

    // start time
    output (window_xsize - header_length,
            window_ysize - y_offset - 8 * a_width,
            (std::string("Run started: ") + start_time).c_str());
    
    // lap statistics
    output (window_xsize - header_length, 
            window_ysize - y_offset - 11 * a_width,
            astring);
    for (i = 1; i <= base_count; i++) 
    {
      itime = elapsed_time[i];
      ilost = lostm[i];
      iturn = turntime[i];
      UPDATE_RESULT;
      output (window_xsize - header_length,
              window_ysize - (y_offset + a_width * (11 + i * 2)), 
              f3f_result);
    }
    
    // end time
    output (window_xsize - header_length,
            window_ysize - (y_offset + 34 * a_width), 
            (std::string("Run finished: ") + end_time).c_str());
    
  }
  else
  {
    GLfloat w = textLength(f3f_meters)
                + textLength(f3f_score)
                + textLength(f3f_turntime);
    // display lap specific infos of current run
    output (window_xsize/2 - w/2, 
            window_ysize - y_offset, 
            f3f_score);
    output (window_xsize/2 - w/2 + textLength(f3f_score), 
            window_ysize - y_offset, 
            f3f_meters);
    output (window_xsize/2 - w/2 + textLength(f3f_score) + textLength(f3f_meters),
            window_ysize - y_offset, 
            f3f_turntime);
    // current run time and penalty
    w = textLength(f3f_time) 
        + textLength(f3f_penalty);
    output (window_xsize/2 - w/2, 
            window_ysize - y_offset - lineheight, 
            f3f_time);
    output (window_xsize/2 - w/2 + textLength(f3f_time), 
            window_ysize - y_offset - lineheight, 
            f3f_penalty);
  }
  finishTextRendering();
}


/** \brief Switch to a text rendering state/projection
 *
 *  Sets up the OpenGL projection matrix for 2D text rendering.
 */
void HandlerF3F::startTextRendering() const
{
  glMatrixMode (GL_PROJECTION);
  glPushMatrix();

  glLoadIdentity ();
  gluOrtho2D (0, window_xsize, 0, window_ysize);
}


/** \brief Switch back to 3D rendering
 *
 *  Revert the changes done in startTextRendering()
 */
void HandlerF3F::finishTextRendering() const
{
  glPopMatrix();
  glMatrixMode (GL_MODELVIEW);
}


/** \brief Render a text string
 *
 *  This method renders a string at a given position using the
 *  specified font.
 *
 *  \todo This method should use PLIB instead of GLUT for font rendering.
 *
 *  \param  x     Horizontal start of text
 *  \param  y     Vertical start of text
 *  \param  text  the string to be displayed
 */
void HandlerF3F::output(GLfloat x, GLfloat y, const char *text)
{
  fontRenderer.begin();
  glColor4f (1.0, 1.0, 1.0, 1.0);
  fontRenderer.start2f(x, y);
  fontRenderer.puts(text);
  fontRenderer.end();
}


/** \brief Calculate the length of a rendered text string
 *
 *  This method calculates the length of a text string rendered
 *  in the current font.
 *
 *  \param    text    pointer to the text to be rendered
 *  \return   width of the rendered text string
 */
GLfloat HandlerF3F::textLength(const char *text) const
{
  float left, right;
  
  fontRenderer.getFont()->getBBox(text, 
                                  fontRenderer.getPointSize(),
                                  fontRenderer.getSlant(),
                                  &left, &right,
                                  NULL, NULL);

  return (right - left);
}



/** \brief Draw a base pylon
 *
 *  This method draws one pole of a base pylon.
 *
 *  \todo The use_textures variable overrides the global variable
 *        of the same name and does not what it implies. Should be
 *        removed, renamed or changed otherwise.
 *  \todo The poles should be rendered without using GLUT.
 *
 *  \param  fx  horizontal location of the pole
 *  \param  fy  vertical location of the pole
 *  \param  fh  height of the pole
 */
void HandlerF3F::draw_f3f_base(int fx, int fy, int  fh) const
{ 
  int use_textures=1;
  static GLuint f3f_baseTexture;

  GLfloat red_base[] = {1,0,0, 1};
  GLfloat white_base[] = {1,1,1, 1};
  GLfloat mat_handi_house[] = {0.343,0.622,0.747, 1};
  GLfloat no_mat[] = {0.0,0.0,0.0,0.0};
  GLfloat no_shininess[] = {0.0};

  if (!use_textures)
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_handi_house);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_handi_house);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,f3f_baseTexture);

    glBegin(GL_QUADS);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0);
    glVertex3f(fx,fh,fy-1);
    glNormal3f(1,0,0);
    glTexCoord2f(1,0);
    glVertex3f(fx,fh,fy);
    glNormal3f(1,0,0);
    glTexCoord2f(1,1);
    glVertex3f(fx,fh+7.5,fy);
    glNormal3f(1,0,0);
    glTexCoord2f(0,1);
    glVertex3f(fx,fh+7.5,fy-1);
    glNormal3f(0,0,1);
    glTexCoord2f(0,0);
    glVertex3f(fx,fh,fy);
    glNormal3f(0,0,1);
    glTexCoord2f(1,0);
    glVertex3f(fx+4,fh,fy);
    glNormal3f(0,0,1);
    glTexCoord2f(1,1);
    glVertex3f(fx+4,fh+7.5,fy);
    glNormal3f(0,0,1);
    glTexCoord2f(0,1);
    glVertex3f(fx,fh+7.5,fy);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0); 
  }
  else
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,red_base);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,red_base);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glPushMatrix();
    //    glTranslatef(fx,fh+3.75,fy-1);
    glTranslatef(fx,fh+(2.5*1)/2,fy-1);
    glScalef(0.4,2.5,0.4);
    drawSolidCube(1);
    glPopMatrix();

    glMaterialfv(GL_FRONT,GL_AMBIENT,white_base);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,white_base);
    glPushMatrix();
    glTranslatef(fx,fh+2.5+(2.5*1)/2,fy-1);
    glScalef(0.4,2.5,0.4);
    drawSolidCube(1);
    glPopMatrix();

    glMaterialfv(GL_FRONT,GL_AMBIENT,red_base);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,red_base);
    glPushMatrix();
    glTranslatef(fx,fh+5+(2.5*1)/2,fy-1);
    glScalef(0.4,2.5,0.4);
    drawSolidCube(1);
    glPopMatrix();

    glMaterialfv(GL_FRONT,GL_AMBIENT,white_base);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,white_base);
    glPushMatrix();
    glTranslatef(fx,fh+7.5+(2.5*1)/2,fy-1);
    glScalef(0.4,2.5,0.4);
    drawSolidCube(1);
    glPopMatrix();

    glMaterialfv(GL_FRONT,GL_AMBIENT,red_base);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,red_base);
    if (extend_base == TRUE) 
    {
      glPushMatrix();
      glTranslatef(fx,fh+3.75+2.5,fy-1);
      glScalef(0.01,250,0.01);
      drawSolidCube(3);
      glPopMatrix();
    }
  }
}


/** \brief Reset the game handler.
 *
 *  Everything will be reset to be ready for a new run.
 */
void HandlerF3F::reset()
{
  run_will_start = FALSE;
  run_started = FALSE;
  run_completed = FALSE;
  x1_toggle = FALSE;
  x2_toggle = FALSE;
  if (start_on_left == TRUE)
    next_base_to_cross = PYLON_RIGHT;
  else
    next_base_to_cross = PYLON_LEFT;
  base_count = 0;
  penality_count = 0;
  in_penalty = TRUE;

  f3f_score [0]=0;
  f3f_time [0]=0;
  f3f_turntime[0]=0;
  f3f_meters [0]=0;
  f3f_result [0]=0;
  f3f_penalty [0]=0;

  pausetime = 0;
  currtime = 0;
  runstarttime = 0;
  runtime = 0;
  runcurrentstart = 0;
  runlaststart = 0;
  runcurrentstop = 0;
  runcurrentturn = 0;
  runturntime = 0;
  resettime=SDL_GetTicks();

  for (int i=0; i<MAX_LAPS; i++) 
  {
    elapsed_time[i] = 0;
    lostm[i]        = 0;
    turntime[i]     = 0;
  }

  lost_meters_rel = 0;
  lost_meters_abs = 0;

  UPDATE_START;
  if (start_sound_id >= 0)
  {
    Global::soundserver->stopChannel (start_sound_id);
    start_sound_id = -1;
  }
  start_sound_id = play(f3f_soundStart);
}


/** \brief Play a sound file.
 *
 *  This method plays the sound named soundName (which is mapped
 *  to the file soundName.wav in the selected F3F sound folder).
 *  If the "beep" folder is selected, print a "bell" character to
 *  stdout.
 *
 *  \todo concatenate strings without using local buffer
 *
 *  \param  soundName   name of the sound to be played
 *  \return the sample-ID returned by the sound server
 */
int HandlerF3F::play (std::string soundName)
{
  char sfile[1024];
  int sampleId = 0;
  if (use_beep)
  {
    printf ("\007*beep*\n");
  }
  else
  {
    sprintf (sfile, "%s/%s.wav",f3f_sound_dir.c_str(), soundName.c_str());
    sampleId = Global::soundserver->playSample (sfile);
    printf ("playing %s\n", sfile);
  }
  return sampleId;
}


/** \brief Play a sound file.
 *
 *  This method plays a sound depending on the current base
 *  number. The given soundName and base number will be
 *  concatenated to form the name of the actual sound file.
 *
 *  \todo concatenate strings without using local buffer
 *
 *  \param soundName  name of the sound to be played
 *  \param base       current base count
 *  \return the sample-ID returned by the sound server
 */
int HandlerF3F::play (std::string soundName, int base)
{
  char sfile[1024];
  sprintf (sfile,"%s%d",soundName.c_str(),base);
  return play (sfile);
}


/** \brief Update internal timers
 *
 *  Updates the internal "current time" and actual "run time".
 */
void HandlerF3F::update_time ()
{
  // *** to replace by the parameter if needed
  // *** preferable use the exact clock at the time of execution
  currtime = SDL_GetTicks() - resettime;
  runtime = currtime - runstarttime - pausetime;
}


/**
 *  Purpose: Get Time when starting the run
 */
void HandlerF3F::run_start()
{
  update_time ();
  UPDATE_START;

  if (start_on_left == TRUE) /* start  on left */
  {
    if ((XX_cg_rwy > plan_limit) && (run_will_start == FALSE))
    {
      run_will_start = TRUE;
      play (f3f_soundEntry);
    }
    if (((XX_cg_rwy < plan_limit) && (run_will_start == TRUE)) || repeat_mode == 2)
    {
      run_started = TRUE;
      getSystemTimeString(start_time);
      if (start_sound_id >= 0) 
      {
        Global::soundserver->stopChannel (start_sound_id);
        start_sound_id = -1;
      }
      play (f3f_soundFirst);
      update_time ();
      if (runtime > MAX_START_TIME)
        runstarttime = MAX_START_TIME;
      else
        runstarttime = runtime;
    }
  }
  else  /* start on right */ 
  {
    if ((XX_cg_rwy < -plan_limit) && (run_will_start == FALSE)) 
    {
      run_will_start = TRUE;
      play (f3f_soundEntry);
    }
    if (((XX_cg_rwy > -plan_limit) && (run_will_start == TRUE)) || repeat_mode == 2) 
    {
      run_started = TRUE;
      getSystemTimeString(start_time);
      if (start_sound_id >= 0) 
      {
        Global::soundserver->stopChannel (start_sound_id);
        start_sound_id = -1;
      }
      play (f3f_soundFirst);
      update_time ();
      if (runtime > MAX_START_TIME)
        runstarttime = MAX_START_TIME;
      else 
        runstarttime = runtime;
    }
  }
}


/** \brief Finish a run
 *
 *  Ends the current run and saves the run results to a file.
 */
void HandlerF3F::end_run()
{
  run_completed = TRUE;
  getSystemTimeString(end_time);

  /* save result in score file */
  {
    FILE *fp;
    char thestart [256];

    fp = fopen ("f3f_score.txt","a+");

    if (start_from_ground == TRUE)
      sprintf (thestart,"Start from ground");
    else
      sprintf (thestart,"Start from altitude");

    fprintf(fp, "%s, Run: %2d.%2ds, Penalty: %d, Lost: %dm, Turn: %2d.%2ds, %s\n", 
            end_time.c_str(),
            (runtime)/1000,
            (runtime)%1000/10,
            penality_count,
            lost_meters_abs,
            (runturntime)/1000,
            (runturntime)%1000/10,
            thestart);
    fclose (fp);
  }
}


/**
 *  calculate the meters run over the base in the turn 
 */
void HandlerF3F::update_lost_meters ()
{
  double x = fabs(XX_cg_rwy);

  x = (x - plan_limit) * FT_TO_M;

  if (x > lost_meters_rel)
  {
    // actual glider pos - pylon pos
    lost_meters_rel = (int) x;
  }
}


/** \brief Update turn time
 *
 *  Update the turn time.
 */
void HandlerF3F::update_turntime ()
{
  if ( ((XX_cg_rwy > plan_limit) && (next_base_to_cross == PYLON_RIGHT))
        ||
       ((XX_cg_rwy < -plan_limit) && (next_base_to_cross == PYLON_LEFT)) )
  {
    update_time ();
    runcurrentturn = runtime;
  }
}


/** \brief Cyclic game-handler function for F3F
 *
 *  Check if the model cross pylons A or pylons B.
 *  Increase BASE counter.
 */
void HandlerF3F::update(float a, float b, float c)
{
  XX_cg_rwy = -a;
  YY_cg_rwy = -b;
  ZZ_cg_rwy =  c;

  // start countdown
  if (run_started == FALSE && run_completed == FALSE)
  {
    run_start();
    if (run_started == FALSE) 
      return;
  }

  // penalties verhaengen
  if (YY_cg_rwy < security_line * M_TO_FT)
  {
    if (in_penalty == FALSE) {
      if (run_completed == FALSE)
      {
        penality_count++;
      }
      in_penalty = TRUE;
      if (run_completed == FALSE) 
        play(f3f_soundPenalty);
    }
  }
  else 
    in_penalty = FALSE;


  // check if the model cross pylons A or pylons B
  if ((XX_cg_rwy > plan_limit) && (next_base_to_cross == PYLON_LEFT)) 
  {
    next_base_to_cross = PYLON_RIGHT;
    play (f3f_soundBase,base_count+1);

    if (run_completed == FALSE) 
    {
      base_count++;
      turntime[base_count-1] = runcurrentturn - runcurrentstop;
      runturntime += (runcurrentturn - runcurrentstop);
      update_time ();
      if (base_count > 0) 
      {
        runcurrentstop = runtime;
      }
      elapsed_time[base_count] = runcurrentstop - runcurrentstart;
      runlaststart = runcurrentstart;
      runcurrentstart = runtime;
      lostm[base_count-1] = lost_meters_rel;
      lost_meters_abs += lost_meters_rel;
      lost_meters_rel = 0;
      if (base_count >= MAX_LAPS) 
      {
        end_run();
      }
    }
  }
  else 
  {
    if ((XX_cg_rwy < -plan_limit) && (next_base_to_cross == PYLON_RIGHT)) 
    {
      next_base_to_cross = PYLON_LEFT;
      play (f3f_soundBase,base_count+1);

      if (run_completed == FALSE) 
      {
        base_count++;
        turntime[base_count-1] = runcurrentturn - runcurrentstop;
        runturntime += (runcurrentturn - runcurrentstop);
        update_time ();
        if (base_count > 0) 
        {
          runcurrentstop = runtime;
        }
        elapsed_time[base_count] = runcurrentstop - runcurrentstart;
        runlaststart = runcurrentstart;
        runcurrentstart = runtime;
        lostm[base_count-1] = lost_meters_rel;
        lost_meters_abs += lost_meters_rel;
        lost_meters_rel = 0;
        if (base_count == MAX_LAPS) 
        {
          end_run();
        }
      }
    }
  }

  // status update
  if (run_completed == TRUE) 
  {
    UPDATE_COMPLETE;
  }
  else if (run_started == TRUE)
  {
    UPDATE_SCORE;
    UPDATE_PENALTY;
    update_time ();
    UPDATE_TIME;
    update_lost_meters ();
    UPDATE_METERS;
    update_turntime ();
    UPDATE_TURN;
  }
}


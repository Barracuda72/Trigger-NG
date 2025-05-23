
// app.cpp [pengine]

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)

#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <glm/mat4x4.hpp> // For glm::mat4
#include <glm/ext/matrix_transform.hpp> // For glm::translate
#include <glm/ext/matrix_clip_space.hpp> // For glm::frustrum

#include "pengine.h"
#include "physfs_utils.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if 0
#ifndef DATADIR
//#error DATADIR not defined! Use ./configure --datadir=...
//#define DATADIR "../data"
#endif
#endif

#if 0
#ifndef LOCALSTATEDIR
#warning LOCALSTATEDIR not defined! Will attempt to determine at run time.
#warning Use ./configure --localstatedir=...
#endif
#endif


/*
FIXME: not bothering to close joysticks because I
suspect SDL does it for you on quit. Am I right?
*/

#ifndef GLES2
void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  (void)source;
  (void)id;
  (void)length;
  (void)userParam;

  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    return; // Skip debug spam

  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}
#endif

int PUtil::deblev = DEBUGLEVEL_ENDUSER;

PApp::PApp(const std::string &title, const std::string &name):
        best_times("/players"),
        appname(name), // for ~/.name
        apptitle(title) // for window title
{
    //PUtil::outLog() << "Initialising SDL" << std::endl;
    const int si = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);

    if (si < 0)
    {
        PUtil::outLog() << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
    }

    cx = cy = 0;
    bpp = 0;
    fullscr = false;
    noframe = false;
    exit_requested = false;
    screenshot_requested = false;
    reqRGB = reqAlpha = reqDepth = reqStencil = false;
    stereo = StereoNone;
    stereoEyeTranslation = 0.0f;
    grabinput = false;
}

void PApp::setScreenModeAutoWindow()
{
#ifdef WIN32
  cy = GetSystemMetrics(SM_CYSCREEN) * 6 / 7;
  cx = cy * 4 / 3;
#else
  //#error COMPLETE FOR PLATFORM
  cy = 1024 * 6 / 7;
  cx = cy * 4 / 3;
#endif

  fullscr = false;
}

void PApp::setScreenModeFastFullScreen()
{
#ifdef WIN32
  cx = GetSystemMetrics(SM_CXSCREEN);
  cy = GetSystemMetrics(SM_CYSCREEN);
#else
  //#error COMPLETE FOR PLATFORM
  cx = 1280;
  cy = 1024;
#endif

  fullscr = false;

  noframe = true;
}

/* This routine performs the perspective projection for one eye's subfield.
   The projection is in the direction of the negative z axis.

   xmin, ymax, ymin, ymax = the coordinate range, in the plane of zero
   parallax setting, that will be displayed on the screen. The ratio between
   (xmax-xmin) and (ymax-ymin) should equal the aspect ration of the display.

   znear, zfar = the z-coordinate values of the clipping planes.

   zzps = the z-coordinate of the plane of zero parallax setting.

   dist = the distance from the center of projection to the plane of zero
   parallax.

   eye = half the eye separation; positive for the right eye subfield,
   negative for the left eye subfield.
*/
/*
void PApp::stereoGLProject(float xmin, float xmax, float ymin, float ymax, float znear, float zfar, float zzps, float dist, float eye)
{
  float xmid, ymid, clip_near, clip_far, top, bottom, left, right, dx, dy, n_over_d;

  dx = xmax - xmin;
  dy = ymax - ymin;

  xmid = (xmax + xmin) / 2.0;
  ymid = (ymax + ymin) / 2.0;

  clip_near = dist + zzps - znear;
  clip_far = dist + zzps - zfar;

  n_over_d = clip_near / dist;

  top = n_over_d * dy / 2.0;
  bottom = -top;
  right = n_over_d * (dx / 2.0 - eye);
  left = n_over_d * (-dx / 2.0 - eye);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(left, right, bottom, top, clip_near, clip_far);
  glTranslatef(-xmid - eye, -ymid, -zzps - dist);
}
*/

// I'm afraid I didn't understand how stereoGLProject worked, so I rewrote it

glm::mat4 PApp::stereoFrustum(float xmin, float xmax, float ymin, float ymax, float znear, float zfar, float zzps, float eye)
{
  // xmove = eye * (zzps - znear) / zzps - eye, simplifies to

  float xmove = -eye * znear / zzps;

  glm::mat4 frust = glm::frustum(xmin + xmove, xmax + xmove, ymin, ymax, znear, zfar);

  return frust;
}

void PApp::setScreenMode(int w, int h, bool fullScreen, bool hideFrame)
{
    // use automatic video mode
    if (autoVideo)
    {
        SDL_DisplayMode dm;

        if (SDL_GetCurrentDisplayMode(0, &dm) == 0)
        {
            cx = dm.w;
            cy = dm.h;
            fullscr = fullScreen;
            noframe = hideFrame;
            PUtil::outLog() << "Automatic video mode resolution: " << cx << 'x' << cy << std::endl;
        }
        else
        {
            PUtil::outLog() << "SDL error, SDL_GetCurrentDisplayMode(): " << SDL_GetError() << std::endl;
            autoVideo = false;
        }
    }

    // not written as an `else` branch because `autoVideo` may have
    // been updated in the case that automatic video mode failed
    if (!autoVideo)
    {
        cx = w;
        cy = h;
        fullscr = fullScreen;
        noframe = hideFrame;
    }
}

int PApp::run(int argc, char *argv[])
{
  PUtil::outLog() << apptitle << " init" << std::endl;

  PUtil::outLog() << "Build: " << PACKAGE_VERSION << " on " << __DATE__ << " at " << __TIME__ << std::endl;

  if (exit_requested) {
    PUtil::outLog() << "Exit requested" << std::endl;
    return 0;
  }

  PUtil::outLog() << "Initialising PhysFS" << std::endl;

  #ifdef __ANDROID__
  // Use "/storage/emulated/0/Android/data/etcetcetc" as a base directory for all files
  const char* app_dir = SDL_AndroidGetExternalStoragePath();
  // This is required for PhysFS *nix module to pick up this directory correctly as a user home directory
  setenv("XDG_DATA_HOME", app_dir, 1);
  #else
  const char* app_dir = (argc >= 1) ? argv[0] : nullptr;
  #endif

  PUtil::outLog() << "App dir is " << app_dir << std::endl;

  if (PHYSFS_init(app_dir) == 0) {
    PUtil::outLog() << "PhysFS failed to initialise" << std::endl;
    PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
    return 1;
  }

  PHYSFS_permitSymbolicLinks(1);

  {
    std::string lsdbuff;

    lsdbuff = physfs_getDir();

    PUtil::outLog() << "Setting writable user directory to \"" << lsdbuff << "\"" << std::endl;

    if (PHYSFS_setWriteDir(lsdbuff.c_str()) == 0) {
      PUtil::outLog() << "Failed to set PhysFS writable directory to \"" << lsdbuff << "\"" << std::endl
          << "PhysFS: " << physfs_getErrorString() << std::endl;
    }

    if (PHYSFS_mkdir("/players") == 0)
    {
        PUtil::outLog() << "Failed to create directory \"/players\"" << std::endl
            << "PhysFS: " << physfs_getErrorString() << std::endl;
    }

    std::string basedir = PHYSFS_getBaseDir();
    PUtil::outLog() << "Application base directory \"" << basedir << '\"' << std::endl;
    if (PHYSFS_mount(basedir.c_str(), NULL, 1) == 0) {
      PUtil::outLog() << "Failed to add PhysFS search directory \"" << basedir << "\"" << std::endl
          << "PhysFS: " << physfs_getErrorString() << std::endl;
    }

    if (PHYSFS_mount(lsdbuff.c_str(), NULL, 1) == 0) {
      PUtil::outLog() << "Failed to add PhysFS search directory \"" << lsdbuff << "\"" << std::endl
          << "PhysFS: " << physfs_getErrorString() << std::endl;
    }

    // we run MainApp::config() here in order to add data dirs to PhysFS search path
    try
    {
        config();
    }
    catch (PException &e)
    {
        PUtil::outLog() << "Config failed: " << e.what() << std::endl;

        if (PHYSFS_deinit() == 0)
        {
            PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
        }

        return 1;
    }

    // Find any .zip files and add them to search path
    std::list<std::string> zipfiles = PUtil::findFiles("", ".zip");
    #ifdef __ANDROID__
    // TODO: this is a hack.
    // PhysFS seems to be trying to go over all path components in the decending manner when looking for the files.
    // Unfortunately, this doesn't work for Android and that "/storage/emulated/0/Android/data/etcetcetc" path
    // we specified above, because regular programs can't access "/storage/emulated" directory on most devices.
    // File itself will be accessible by PhysFS tho, so no problems here.
    zipfiles.push_back("data.zip");
    #endif

    for (std::list<std::string>::iterator i = zipfiles.begin();
      i != zipfiles.end(); ++i) {

      const char *realpath = PHYSFS_getRealDir(i->c_str());

      if (realpath) {
        std::string fullpath = (std::string)realpath + *i;

        if (PHYSFS_mount(fullpath.c_str(), NULL, 1) == 0) {
          PUtil::outLog() << "Failed to add archive \"" << fullpath << "\"" << std::endl
              << "PhysFS: " << physfs_getErrorString() << std::endl;
        }
      } else {
        PUtil::outLog() << "Failed to find path of archive \"" << *i << "\"" << std::endl
            << "PhysFS: " << physfs_getErrorString() << std::endl;
      }
    }
  }

  srand(SDL_GetTicks());

  PUtil::outLog() << "Create window and set video mode" << std::endl;

#ifndef GLES2
#ifndef GL30PLUS
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif // GL30PLUS
#endif // GLES2

  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  if (reqRGB) {
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
  }

  if (reqAlpha) {
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
  }

  if (reqDepth) {
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
  }

  if (reqStencil)
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );

  #ifndef GLES2
  if (stereo == StereoQuadBuffer) {
    SDL_GL_SetAttribute( SDL_GL_STEREO, 1 );
  }
  #endif

  if (cx <= 0 || cy <= 0) setScreenModeAutoWindow();

    if (!autoVideo)
    {
        screen = SDL_CreateWindow(apptitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            cx, cy,
            SDL_WINDOW_OPENGL |
            (fullscr ? SDL_WINDOW_FULLSCREEN : 0)
            /* | (noframe ? SDL_NOFRAME : 0) */ ); // broken by SDL2, no more `SDL_NOFRAME`
    }
    else // automatic video mode
    {
        //
        // NOTE:
        //  The `SDL_GetDesktopMode()` function is used instead of the automatic
        //  flag `SDL_WINDOW_FULLSCREEN_DESKTOP` because the latter fails to work
        //  reliably on Linux in certain versions of SDL2.
        //

#define SDL_WINDOW_FULLSCREEN_DESKTOP_IS_STILL_BROKEN

#ifdef SDL_WINDOW_FULLSCREEN_DESKTOP_IS_STILL_BROKEN
        SDL_DisplayMode dm;

        if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
        {
            cx = dm.w;
            cy = dm.h;
            PUtil::outLog() << "Desktop video mode resolution: " << cx << 'x' << cy << std::endl;
        }
        else
        {
            PUtil::outLog() << "SDL error, SDL_GetDesktopDisplayMode(): " << SDL_GetError() << std::endl;
            cx = 800;
            cy = 600;
        }

        screen = SDL_CreateWindow(apptitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            cx, cy,
            SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
#else
        //
        // NOTE:
        //  This would be the "proper" way of doing things, but at the time of this
        //  writing it only works reliably on Windows 7 using SDL2 version 2.0.5.
        //
        screen = SDL_CreateWindow(apptitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            0, 0,
            SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
#endif
    }

  if (!screen) {
    PUtil::outLog() << "Failed to create window or set video mode" << std::endl;
    PUtil::outLog() << "SDL error: " << SDL_GetError() << std::endl;
    PUtil::outLog() << "Try changing your video settings in trigger-rally.config" << std::endl;
    SDL_Quit();
    if (PHYSFS_deinit() == 0) {
      PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
    }
    return 1;
  }

  context = SDL_GL_CreateContext(screen);

    if (context == NULL)
    {
        PUtil::outLog() << "Failed to create OpenGL context for game window\n";
        PUtil::outLog() << "SDL error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(screen);
        SDL_Quit();

        if (PHYSFS_deinit() == 0)
		{
            PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
		}
        return 1;
    }

  #ifdef GLES2
  SDL_Renderer* renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
  #endif

  sdl_mousemap = 0;

  sdl_joy.resize(SDL_NumJoysticks());

  PUtil::outLog() << "Found " << sdl_joy.size() << " joystick" <<
    (sdl_joy.size() == 1 ? "" : "s") << std::endl;

  for (unsigned int i=0; i<sdl_joy.size(); i++) {
    PUtil::outLog() << "Joystick " << (i+1) << ": ";
    sdl_joy[i].sdl_joystick = SDL_JoystickOpen(i);
    if (sdl_joy[i].sdl_joystick == nullptr) {
      PUtil::outLog() << "failed to open joystick" << std::endl;
      SDL_GL_DeleteContext(context);
      SDL_DestroyWindow(screen);
      SDL_Quit();
      if (PHYSFS_deinit() == 0) {
        PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
      }
      return 1;
    }
    sdl_joy[i].name = SDL_JoystickName(sdl_joy[i].sdl_joystick);
    sdl_joy[i].axis.resize(SDL_JoystickNumAxes(sdl_joy[i].sdl_joystick));
    for (unsigned int j=0; j<sdl_joy[i].axis.size(); j++)
      sdl_joy[i].axis[j] = ((float)SDL_JoystickGetAxis(sdl_joy[i].sdl_joystick, j) + 0.5f) / 32767.5f;
    sdl_joy[i].button.resize(SDL_JoystickNumButtons(sdl_joy[i].sdl_joystick));
    for (unsigned int j=0; j<sdl_joy[i].button.size(); j++)
      sdl_joy[i].button[j] = (SDL_JoystickGetButton(sdl_joy[i].sdl_joystick, j) != 0);
    sdl_joy[i].hat.resize(SDL_JoystickNumHats(sdl_joy[i].sdl_joystick));
    for (unsigned int j=0; j<sdl_joy[i].hat.size(); j++) {
      Uint8 state = SDL_JoystickGetHat(sdl_joy[i].sdl_joystick, j);
      sdl_joy[i].hat[j] = vec2i::zero();
      if (state & SDL_HAT_RIGHT) sdl_joy[i].hat[j].x = 1;
      else if (state & SDL_HAT_LEFT) sdl_joy[i].hat[j].x = -1;
      if (state & SDL_HAT_UP) sdl_joy[i].hat[j].y = 1;
      else if (state & SDL_HAT_DOWN) sdl_joy[i].hat[j].y = -1;
    }

    PUtil::outLog() << sdl_joy[i].name << ", " <<
      sdl_joy[i].axis.size() << " axis, " <<
      sdl_joy[i].button.size() << " button, " <<
      sdl_joy[i].hat.size() << " hat" << std::endl;

    sdl_joy[i].sdl_haptic = SDL_HapticOpenFromJoystick(sdl_joy[i].sdl_joystick);
    if (sdl_joy[i].sdl_haptic) {
      if (SDL_HapticRumbleInit(sdl_joy[i].sdl_haptic) != 0) {
        SDL_HapticClose(sdl_joy[i].sdl_haptic);
        sdl_joy[i].sdl_haptic = nullptr;
      }
    }
  }

  SDL_JoystickEventState(SDL_ENABLE);

#ifdef GLES2
  /*
   * Previously I've employed a few extensions to GLESv2 to be compatible with core OpenGL.
   * I've rewritten the code in a way that doesn't require any extension but decided to keep
   * this check in place for the future.
   */
  #if 0
  std::vector<std::string> extensions = {
  };

  for (auto& extension: extensions) {
    int supported = SDL_GL_ExtensionSupported(extension.c_str());
    if (!supported) {
        PUtil::outLog() << "Required extension is missing: " << extension << std::endl;
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(screen);
        SDL_Quit();
        if (PHYSFS_deinit() == 0) {
            PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
        }
        return 1;
    }
  }
  #endif
#else
  glewExperimental = true;

  int err = glewInit();

  if (err != GLEW_OK) {
    PUtil::outLog() << "GLEW failed to initialise: " << glewGetErrorString(err) << std::endl;
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(screen);
    SDL_Quit();
    if (PHYSFS_deinit() == 0) {
      PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
    }
    return 1;
  }

  PUtil::outLog() << "GLEW initialized" << std::endl;
#endif

  PUtil::outLog() << "Graphics: " <<
    glGetString(GL_VENDOR) << " " << glGetString(GL_RENDERER) << std::endl;

  PUtil::outLog() << "Using OpenGL " << glGetString(GL_VERSION) << std::endl;

  #ifndef GLES2
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);
  #endif

#ifdef GL30PLUS
    // TODO: this is a really cheap hack to workaround OpenGL 3.0+ core profile requirement of VAO
  	GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#endif

  switch (stereo) {
  default: break;
  #ifndef GLES2
  case StereoQuadBuffer:
    PUtil::outLog() << "Using hardware quad buffer stereo, separation ="
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  #endif
  case StereoRedBlue:
    PUtil::outLog() << "Using red-blue anaglyph stereo, separation = "
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  case StereoRedGreen:
    PUtil::outLog() << "Using red-green anaglyph stereo, separation = "
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  case StereoRedCyan:
    PUtil::outLog() << "Using red-cyan anaglyph stereo, separation = "
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  case StereoYellowBlue:
    PUtil::outLog() << "Using yellow-blue anaglyph stereo, separation = "
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  }

  std::list<PSubsystem *> sslist;

  try
  {
    sslist.push_back(ssrdr = new PSSRender(*this));
    sslist.push_back(sstex = new PSSTexture(*this));
    sslist.push_back(ssfx = new PSSEffect(*this));
    sslist.push_back(ssmod = new PSSModel(*this));
    sslist.push_back(ssaud = new PSSAudio(*this));
  }
  catch (PException &e)
  {
    PUtil::outLog () << "Subsystem failed to init: " << e.what() << std::endl;

    while (!sslist.empty())
    {
      delete sslist.back();
      sslist.pop_back();
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(screen);
    SDL_Quit();

    if (PHYSFS_deinit() == 0)
    {
      PUtil::outLog () << "PhysFS: " << physfs_getErrorString() << std::endl;
    }

    return 1;
  }

  PUtil::outLog() << "Performing app load" << std::endl;

  try
  {
    load();
  }
  catch (PException &e)
  {
    PUtil::outLog() << "App load failed: " << e.what () << std::endl;

    while (!sslist.empty()) {
      delete sslist.back();
      sslist.pop_back();
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(screen);
    SDL_Quit();

    if (PHYSFS_deinit() == 0) {
      PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
    }

    return 1;
  }

  //SDL_ShowCursor(SDL_DISABLE);
  //SDL_WM_GrabInput(SDL_GRAB_ON);
  //SDL_WM_GrabInput(SDL_GRAB_OFF);
  //SDL_SetWindowGrab(screen, SDL_TRUE);
  //SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  //SDL_EnableUNICODE(1);

  sdl_keymap = SDL_GetKeyboardState(&sdl_numkeys);

  resize();

  PUtil::outLog() << "Initialisation complete, entering main loop" << std::endl;

  srand(rand() + SDL_GetTicks());

  bool active = true, repaint = true, axis_down = false;
  uint32 curtime = SDL_GetTicks() - 1;

  while (1) {
    SDL_Event event;

    while ( SDL_PollEvent(&event) ) {
      switch(event.type) {

      // Using ACTIVEEVENT only seems to cause trouble.

      /*
      case SDL_ACTIVEEVENT:
        active = event.active.gain;
        if (active) {
          SDL_ShowCursor(SDL_DISABLE);
          //SDL_WM_GrabInput(SDL_GRAB_ON);
          PUtil::outLog() << "Window made active" << std::endl;
        } else {
          SDL_ShowCursor(SDL_ENABLE);
          //SDL_WM_GrabInput(SDL_GRAB_OFF);
          PUtil::outLog() << "Window made inactive" << std::endl;
        }
        break;
      */

      // unavailable (and unneeded?) in SDL2
#if 0
      case SDL_VIDEOEXPOSE:
        repaint = true;
        break;
#endif

      case SDL_KEYDOWN:
      case SDL_KEYUP:
        keyEvent(event.key);
        break;

      case SDL_MOUSEMOTION:
        if (grabinput)
          mouseMoveEvent(event.motion.xrel, -event.motion.yrel);
        else
          cursorMoveEvent(event.motion.x, event.motion.y);
        break;

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        mouseButtonEvent(event.button);
        break;

      case SDL_JOYAXISMOTION:
        sdl_joy[event.jaxis.which].axis[event.jaxis.axis] =
          ((float)event.jaxis.value + 0.5f) / 32767.5f;
        axis_down = joyAxisEvent(event.jaxis.which,event.jaxis.axis,sdl_joy[event.jaxis.which].axis[event.jaxis.axis],axis_down);
        break;

      case SDL_JOYBUTTONDOWN:
        sdl_joy[event.jbutton.which].button[event.jbutton.button] = true;
        joyButtonEvent(event.jbutton.which,event.jbutton.button,true);
        break;

      case SDL_JOYBUTTONUP:
        sdl_joy[event.jbutton.which].button[event.jbutton.button] = false;
        joyButtonEvent(event.jbutton.which,event.jbutton.button,false);
        break;

      case SDL_JOYHATMOTION:
        sdl_joy[event.jhat.which].hat[event.jhat.hat] = vec2i::zero();
        if (event.jhat.value & SDL_HAT_RIGHT) sdl_joy[event.jhat.which].hat[event.jhat.hat].x = 1;
        else if (event.jhat.value & SDL_HAT_LEFT) sdl_joy[event.jhat.which].hat[event.jhat.hat].x = -1;
        if (event.jhat.value & SDL_HAT_UP) sdl_joy[event.jhat.which].hat[event.jhat.hat].y = 1;
        else if (event.jhat.value & SDL_HAT_DOWN) sdl_joy[event.jhat.which].hat[event.jhat.hat].y = -1;
        break;

      case SDL_QUIT:
        requestExit();
        break;
      }
      if (exit_requested) break;
    }

    if (exit_requested) {
      break;
    }

    sdl_mousemap = SDL_GetMouseState(nullptr, nullptr);

#define TIMESCALE 1.0

    uint32 nowtime = SDL_GetTicks();

    if (1) {//if (active) {
      uint32 timepassed = nowtime - curtime;
      if (timepassed > 100) timepassed = 100;
      if (timepassed > 0) {
        float delta = (float)timepassed * 0.001 * TIMESCALE;

        tick(delta);

        for (std::list<PSubsystem *>::iterator i = sslist.begin();
          i != sslist.end(); ++i) {
          (*i)->tick(delta, cam_pos, cam_orimat, cam_linvel);
        }
      }
    }

    curtime = nowtime;

    if (exit_requested) break;

    if (active || repaint) {
      switch (stereo) {

      case StereoNone: // Normal, non-stereo rendering

        render(0.0f);
        glFlush();
        SDL_GL_SwapWindow(screen);
        break;

#ifndef GLES2
      case StereoQuadBuffer: // Hardware quad buffer stereo

        glDrawBuffer(GL_BACK_LEFT);
        render(-stereoEyeTranslation);
        glFlush();

        glDrawBuffer(GL_BACK_RIGHT);
        render(stereoEyeTranslation);
        glFlush();

        SDL_GL_SwapWindow(screen);
        break;
#endif

      case StereoRedBlue: // Red-blue anaglyph stereo

        // Green will not be rendered to, so clear it
        glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
        glClearColor(0.5, 0.5, 0.5, 0.5);
        glClear(GL_COLOR_BUFFER_BIT);

        glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
        render(-stereoEyeTranslation);
        glFlush();

        glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
        render(stereoEyeTranslation);
        glFlush();

        SDL_GL_SwapWindow(screen);
        break;

      case StereoRedGreen: // Red-green anaglyph stereo

        // Blue will not be rendered to, so clear it
        glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
        glClearColor(0.5, 0.5, 0.5, 0.5);
        glClear(GL_COLOR_BUFFER_BIT);

        glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
        render(-stereoEyeTranslation);
        glFlush();

        glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
        render(stereoEyeTranslation);
        glFlush();

        SDL_GL_SwapWindow(screen);
        break;

      case StereoRedCyan: // Red-cyan anaglyph stereo

        glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
        render(-stereoEyeTranslation);
        glFlush();

        glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
        render(stereoEyeTranslation);
        glFlush();

        SDL_GL_SwapWindow(screen);
        break;

      case StereoYellowBlue: // Yellow-blue anaglyph stereo

        glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
        render(-stereoEyeTranslation);
        glFlush();

        glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
        render(stereoEyeTranslation);
        glFlush();

        SDL_GL_SwapWindow(screen);
        break;
      }
      repaint = false;

#ifndef GLES2
      if (screenshot_requested) {
        glReadBuffer(GL_FRONT);
        unsigned char *data1 = new unsigned char[cx*(cy+1)*3];
        glReadPixels(0, 0, cx, cy, GL_RGB, GL_UNSIGNED_BYTE, data1);
        glReadBuffer(GL_BACK);
        const int rowsize = cx * 3;
        for(int i = 0; i < cy/2; ++i) {
          memcpy(&data1[(cy) * rowsize], &data1[(cy-1-i) * rowsize], rowsize);
          memcpy(&data1[(cy-1-i) * rowsize], &data1[(i) * rowsize], rowsize);
          memcpy(&data1[(i) * rowsize], &data1[(cy) * rowsize], rowsize);
          //memset(&data1[(i) * rowsize], 128, rowsize);
        }
        char buff[200];
        sprintf(buff, "P6\n"
          "# CREATOR: Trigger PNM Screenshot\n"
          "%i %i\n255\n", cx, cy);
        char filename[100];
        sprintf(filename, "screen-%09u.ppm", SDL_GetTicks());
        PUtil::outLog() << "Writing screenshot \"" << filename << "\"" << std::endl;
        PHYSFS_file* pfile = PHYSFS_openWrite(filename);
        if (pfile) {
          physfs_write(pfile, buff, sizeof(char), strlen(buff));
          physfs_write(pfile, data1, sizeof(char), cx*cy*3);
          PHYSFS_close(pfile);
        } else {
          PUtil::outLog() << "Screenshot write failed" << std::endl;
          PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
        }
        delete[] data1;
        screenshot_requested = false;
      }
#endif

    } else {
      SDL_WaitEvent(nullptr);
    }

    if (exit_requested) break;
  }

  PUtil::outLog() << "Exit requested" << std::endl;

  unload();

  while (!sslist.empty()) {
    delete sslist.back();
    sslist.pop_back();
  }

  //SDL_WM_GrabInput(SDL_GRAB_OFF);
  //SDL_SetWindowGrab(screen, SDL_FALSE);
  SDL_ShowCursor(SDL_ENABLE);

  #ifdef GL30PLUS
  glDeleteVertexArrays(1, &vao);
  #endif

  #ifdef GLES2
  SDL_DestroyRenderer(renderer);
  #endif

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(screen);
    SDL_Quit();

  best_times.savePlayer();

  if (PHYSFS_deinit() == 0) {
    PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
  }

  PUtil::outLog() << "Shutdown complete" << std::endl;

  return 0;
}

void PApp::grabMouse(bool grab)
{
  //SDL_WM_GrabInput(grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
  SDL_ShowCursor(grab ? SDL_DISABLE : SDL_ENABLE);

  grabinput = grab;
}

void PApp::drawModel(PModel &model, const Light& light, const Material& material, bool is_ghost, const glm::mat4& mv, const glm::mat4& p)
{
  getSSRender().drawModel(model, getSSEffect(), getSSTexture(), light, material, is_ghost, mv, p);
}


// default callback functions

void PApp::config()
{
}

void PApp::load()
{
}

void PApp::unload()
{
}

void PApp::tick(float delta)
{
  delta = delta;
}

void PApp::resize()
{
}

void PApp::render(float eyetranslation)
{
  eyetranslation = eyetranslation;

  glClearColor(0.5, 0.5, 0.5, 0.0);

  glClear(GL_COLOR_BUFFER_BIT);
}

void PApp::keyEvent(const SDL_KeyboardEvent &ke)
{
  if (ke.type != SDL_KEYDOWN) return;

  switch (ke.keysym.sym) {
  case SDLK_ESCAPE:
    requestExit();
    break;
  default:
    break;
  }
}

void PApp::mouseButtonEvent(const SDL_MouseButtonEvent &mbe)
{
  int unused = mbe.type; unused = unused;
}

void PApp::mouseMoveEvent(int dx, int dy)
{
  dx = dx; dy = dy;
}

void PApp::cursorMoveEvent(int posx, int posy)
{
  posx = posx; posy = posy;
}

void PApp::joyButtonEvent(int which, int button, bool down)
{
  which = which; button = button; down = down;
}

bool PApp::joyAxisEvent(int which, int axis, float value, bool down)
{
  which = which; axis = axis; value = value; down = down;
  return false;
}

float PApp::getCtrlActionBackValue()
{
  return 0.0f;
}

int PApp::getVehicleCurrentGear() {
  return 0;
}

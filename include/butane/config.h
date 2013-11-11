/* ========================================================================== */
/*                                                                            */
/* This file is part of Butane.                                               */
/*                                                                            */
/* Author(s):                                                                 */
/*                                                                            */
/*   Michael Williams <devbug@bitbyte.ca>                                     */
/*                                                                            */
/* See LICENSE.md for licensing information.                                  */
/*                                                                            */
/* ========================================================================== */
/*! @file include/butane/config.h
     Documents a collection of pre-processor defines used for the
     configuration of Butane's compile-, link-, and runtime behaviour.        */
/* ========================================================================== */

#ifndef _BUTANE_CONFIG_H_
#define _BUTANE_CONFIG_H_

/* ========================================================================== */
/*  Configuration:                                                            */
/*   * Compiler, architecture, and platform autodetection;                    */
/*   * Paranoid and/or debug, development, and release selection;             */
/*   * Linkage.                                                               */
/* ========================================================================== */

/* ========================================================================== */
/*  Compiler, architecture, and platform autodetection:                       */
/* ========================================================================== */

/*! @def BUTANE_DONT_AUTODETECT_COMPILER
  See FND_DONT_AUTODETECT_COMPILER. */
#ifdef BUTANE_DONT_AUTODETECT_COMPILER
  #define FND_DONT_AUTODETECT_COMPILER
#endif

/*! @def BUTANE_DONT_AUTODETECT_PLATFORM
  See FND_DONT_AUTODETECT_PLATFORM. */
#ifdef BUTANE_DONT_AUTODETECT_PLATFORM
  #define FND_DONT_AUTODETECT_PLATFORM
#endif

/*! @def BUTANE_DONT_AUTODETECT_ARCHITECTURE
  See FND_DONT_AUTODETECT_ARCHITECTURE. */
#ifdef BUTANE_DONT_AUTODETECT_ARCHITECTURE
  #define FND_DONT_AUTODETECT_ARCHITECTURE
#endif

/* ========================================================================== */
/*  Paranoid and/or debug, development, and release selection:                */
/* ========================================================================== */

/*! @def BUTANE_PARANOID
  See FND_PARANOID. */
#ifdef BUTANE_PARANOID
  #define FND_PARANOID
#endif

/*! @def BUTANE_DEBUG
  See FND_DEBUG. */
#define BUTANE_DEBUG FND_DEBUG

/*! @def BUTANE_DEVELOPMENT
  See FND_DEVELOPMENT. */
#define BUTANE_DEVELOPMENT FND_DEVELOPMENT

/*! @def BUTANE_RELEASE
  See FND_RELEASE. */
#define BUTANE_RELEASE FND_RELEASE

/*! @def BUTANE_CONFIGURATION
  See FND_CONFIGURATION. */
#ifndef BUTANE_CONFIGURATION
  #error ("Please specify a configuration by defining `BUTANE_CONFIGURATION`.")
#else
  #ifndef FND_CONFIGURATION
    #define FND_CONFIGURATION BUTANE_CONFIGURATION
  #endif
#endif

/* ========================================================================== */
/*  Linkage:                                                                  */
/* ========================================================================== */

/*! @def BUTANE_LINK_STATICALLY
  Linking to Butane statically, e.g., using libbutane.a. */
#define BUTANE_LINK_STATICALLY 1

/*! @def BUTANE_LINK_DYNAMICALLY
  Linking to Butane dynamically, e.g., using libbutane.so. */
#define BUTANE_LINK_DYNAMICALLY 2

/*! @def BUTANE_LINKAGE
  Specifies if Butane is being linked to statically or dynamically. */
#ifndef BUTANE_LINKAGE
  #error ("Please specify how you are linking to Butane by defining `BUTANE_LINKAGE`.")
#endif

/* ========================================================================== */

#endif /* _BUTANE_CONFIG_H_ */

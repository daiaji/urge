#ifndef RUBY_VERSION_H                               /*-*-C++-*-vi:se ft=cpp:*/
#define RUBY_VERSION_H 1
/**
 * @file
 * @author     $Author$
 * @date       Wed May 13 12:56:56 JST 2009
 * @copyright  Copyright (C) 1993-2009 Yukihiro Matsumoto
 * @copyright  Copyright (C) 2000  Network Applied Communication Laboratory, Inc.
 * @copyright  Copyright (C) 2000  Information-technology Promotion Agency, Japan
 * @copyright  This  file  is   a  part  of  the   programming  language  Ruby.
 *             Permission  is hereby  granted,  to  either redistribute  and/or
 *             modify this file, provided that  the conditions mentioned in the
 *             file COPYING are met.  Consult the file for details.
 *
 * This file contains only
 * - never-changeable information, and
 * - interfaces accessible from extension libraries.
 *
 * Never try to check RUBY_VERSION_CODE etc in extension libraries,
 * check the features with mkmf.rb instead.
 */

/**
 * @name The origin.
 *
 * These information never change.  Just written here to remember.
 *
 * @{
 */

/** Author of this project. */
#define RUBY_AUTHOR "Yukihiro Matsumoto"

/** Ruby's birth year. */
#define RUBY_BIRTH_YEAR 1993

/** Ruby's birth month. */
#define RUBY_BIRTH_MONTH 2

/** Ruby's birth day. */
#define RUBY_BIRTH_DAY 24

/** @} */

/**
 * @name The API version.
 *
 * API version  is different from  binary version.   These numbers are  for API
 * stability.  When you  have distinct API versions x and  y, you cannot expect
 * codes targeted to x also works for y.
 *
 * However   let   us  repeat   here   that   it's   a   BAD  idea   to   check
 * #RUBY_API_VERSION_CODE form extension libraries.  Different API versions are
 * just different.  There is no such thing like upper compatibility.
 *
 * @{
 */

/**
 * Major version.  This  digit changes sometimes for various  reasons, but that
 * doesn't mean a total rewrite.  Practically  when it comes to API versioning,
 * major and minor version changes are equally catastrophic.
 */
#define RUBY_API_VERSION_MAJOR 3

/**
 * Minor  version.   As of  writing  this  version changes  annually.   Greater
 * version doesn't mean "better"; they just mean years passed.
 */
#define RUBY_API_VERSION_MINOR 2

/**
 * Teeny version.  This digit  is kind of reserved these days.   Kept 0 for the
 * entire 2.x era.  Waiting for future uses.
 */
#define RUBY_API_VERSION_TEENY 0

/**
 * This macro is API versions encoded into a C integer.
 *
 * @note  Use mkmf.
 * @note  Don't rely on it.
 */
#define RUBY_API_VERSION_CODE (RUBY_API_VERSION_MAJOR*10000+RUBY_API_VERSION_MINOR*100+RUBY_API_VERSION_TEENY)

/** @} */

#endif

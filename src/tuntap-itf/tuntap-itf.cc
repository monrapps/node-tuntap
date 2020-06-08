/*
 * Copyright (c) 2010-2014 BinarySEC SAS
 * Tuntap binding for nodejs [http://www.binarysec.com]
 * 
 * This file is part of Gate.js.
 * 
 * Gate.js is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#if defined(__APPLE__)
#error "Apple OSes are not supported for now"
#elif defined(__linux__)
#include "tuntap-itf-linux.inc.cc"
#elif defined(__unix__) || defined(_POSIX_VERSION)
/* This may not work, but still, try... */
#include "tuntap-itf-linux.inc.cc"
#else
#error "Your operating system does not seems to be supported"
#endif
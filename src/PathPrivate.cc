/*
 * Copyright (C) 2015 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <sstream>

#include "ignition/math/FrameException.hh"
#include "ignition/math/PathPrivate.hh"

using namespace ignition;
using namespace math;

/////////////////////////////////////////////////
PathPrivate::PathPrivate(const std::string &_s)
  : path(_s)
{
  if( _s.empty())
  {
    std::stringstream ss;
    ss << "Error: path cannot be empty";
    throw FrameException(ss.str());
  }

  std::stringstream ss(_s);
  std::string item;
  while (std::getline(ss, item, '/'))
  {
    if (item == "")
      continue;
    if (item == ".")
      continue;
    // avoid path elements with wrong names
    if(!this->CheckName(item))
    {
      std::stringstream ss;
      ss << "Error: path \"" << _s << "\" contains an invalid element: \"";
      ss << item << "\"";
      throw FrameException(ss.str());
    }
    this->pathElems.push_back(item);
  }
}

/////////////////////////////////////////////////
bool PathPrivate::CheckName(const std::string &_name)
{
  // authorize special path elements
  if(_name == "." || _name == "..")
    return true;
  // frame names should not be empty
  if (_name.empty())
    return false;
  // and not contain any of these characters
  if (_name.find_first_of("/!@#$%^&*\t ()\":;'.~`_+=,<>") != std::string::npos)
    return false;
  // good for now
  return true;
}

/////////////////////////////////////////////////
const std::vector<std::string> &PathPrivate::Elems() const
{
  return this->pathElems;
}

/////////////////////////////////////////////////
std::string PathPrivate::Path() const
{
  return this->path;
}

/////////////////////////////////////////////////
bool PathPrivate::IsAbsolute() const
{
  if (this->path[0] != '/')
    return false;
  // does it start with world?
  if (this->pathElems[0] != "world")
  {
    return false;
  }
  for (const std::string &s : this->pathElems)
  {
    if (s == "..")
    {
      return false;
    }
  }
  return true;
}

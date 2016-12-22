//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Ken Barker
// (ken dot barker at via-technology dot co dot uk)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
/// @file request_router.cpp
/// @brief Classes to route HTTP requests.
//////////////////////////////////////////////////////////////////////////////
#include "via/http/request_router.hpp"
#include <string>
#include <sstream>
#include <vector>

namespace via
{
  void split(std::vector<std::string> &elems, const std::string &str, char const delim) 
  {
    std::stringstream ss;
    ss.str(str);
    std::string item;
    while (std::getline(ss, item, delim))
    {
      elems.push_back(item);
    }  
  }
  namespace http
  {
    //////////////////////////////////////////////////////////////////////////
    Parameters get_route_parameters(std::string uri_path, std::string route_path)
    {
      Parameters parameters;

      // Find the first ':' in the route_path
      auto param_start(route_path.find(':'));
      if (param_start != std::string::npos)
      {
        // erase both paths prior to the ':'
        route_path = route_path.substr(param_start);
        uri_path   = uri_path.substr(param_start);

        // get the strings between the '/'s
        std::vector<std::string> names;
        split(names, route_path,'/');

        std::vector<std::string> values;
        split(values, uri_path, '/');

        // get the route_parameter name value pairs
        if (names.size() == values.size())
        {
          for (auto i(0U); i < names.size(); ++i)
          {
            std::string name(names[i]);
            if ((name[0] != ':') && (name != values[i]))
              return Parameters();
            else
            {
              name.erase(0, 1); // ignore the ':'
              parameters.insert(Parameters::value_type(name, values[i]));
            }
          }
        }
      }

      return parameters;
    }
    //////////////////////////////////////////////////////////////////////////
  }
}

/* (c) Copyright 2012 Felipe Magno de Almeida
*
* Distributed under the Boost Software License, Version 1.0. (See
* accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef OB_DIAG_CONDITIONS_HPP
#define OB_DIAG_CONDITIONS_HPP

#include <stdexcept>

namespace ob_diag {

struct require_error : std::exception
{
  ~require_error() throw() {}
  const char* what() throw() { return "ob_diag::require_error"; }
};

}

#define OB_DIAG_REQUIRE(COND, MSG_SUC, MSG_ERR)        \
  if(COND)                                            \
  {                                                   \
    std::cout << "[INFO] " << MSG_SUC << std::endl;                  \
  }                                                     \
  else                                                  \
  {                                                   \
    std::cout << "[ERROR] " << MSG_ERR << std::endl;                \
    throw ::ob_diag::require_error();                 \
  }

#define OB_DIAG_WARN(COND, MSG_WARN)        \
  if(COND)                                            \
  {                                                   \
    std::cout << "[WARNING] " << MSG_WARN << std::endl;                  \
  }

#define OB_DIAG_FAIL(COND, MSG_ERR)        \
  if(COND)                                            \
  {                                                   \
    std::cout << "[FAIL] " << MSG_ERR << std::endl;                  \
    throw ::ob_diag::require_error();                    \
  }

#endif


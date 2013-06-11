/* (c) Copyright 2012 Felipe Magno de Almeida
*
* Distributed under the Boost Software License, Version 1.0. (See
* accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef OB_DIAG_READ_SYSTEM_EXCEPTION_HPP
#define OB_DIAG_READ_SYSTEM_EXCEPTION_HPP

#include <ob-diag/read_reply.hpp>
#include <ob-diag/system_exception.hpp>

#include <morbid/giop/forward_back_insert_iterator.hpp>
#include <morbid/giop/grammars/arguments.hpp>
#include <morbid/giop/grammars/message_1_0.hpp>
#include <morbid/giop/grammars/request_1_0.hpp>
#include <morbid/giop/grammars/reply_1_0.hpp>
#include <morbid/giop/grammars/system_exception_reply_body.hpp>
#include <morbid/iiop/all.hpp>
#include <morbid/iiop/grammars/profile_body_1_1.hpp>
#include <morbid/iiop/profile_body.hpp>

#include <boost/asio.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace ob_diag {

system_exception read_system_exception(boost::asio::ip::tcp::socket& socket
                                       , service_context_list& service_context)
{
  std::vector<char> reply_buffer(4096);
  boost::system::error_code ec;
  std::size_t size = socket.read_some
    (boost::asio::mutable_buffers_1(&reply_buffer[0], reply_buffer.size()), ec);
  reply_buffer.resize(size);

  OB_DIAG_REQUIRE(!ec, "Read reply with " << reply_buffer.size() << " bytes"
                  ,  "Failed reading with error " << ec.message())

  typedef std::vector<char>::iterator iterator_type;
  iterator_type first = reply_buffer.begin()
    ,  last = reply_buffer.end();

  typedef reply_types<fusion::vector0<> > reply_type;
  reply_type reply(spirit::eps);

  bool g = qi::parse(first, last
                     , giop::compile<iiop::parser_domain>
                     (reply.message_grammar_)
                     , reply.attribute)
    && first == last;
  
  OB_DIAG_REQUIRE(g, "Parsing reply successfully"
                  , "Parsing reply failed. This is a bug in the diagnostic or a bug in OpenBus")

  reply_type::variant_attribute_type variant_attr
    = fusion::at_c<3u>(fusion::at_c<0u>(reply.attribute));

  OB_DIAG_FAIL(!boost::get<system_exception>(&variant_attr)
               , "A System Exception was expected, but not thrown by the server")

  service_context = fusion::at_c<0>(fusion::at_c<0>(reply.attribute));
  return boost::get<system_exception>(variant_attr);
}

}

#endif

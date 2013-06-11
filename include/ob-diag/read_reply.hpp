/* (c) Copyright 2012 Felipe Magno de Almeida
*
* Distributed under the Boost Software License, Version 1.0. (See
* accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef OB_DIAG_READ_REPLY_HPP
#define OB_DIAG_READ_REPLY_HPP

#include <ob-diag/service_context_list.hpp>
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

namespace ob_diag {

namespace giop = morbid::giop;
namespace iiop = morbid::iiop;
namespace fusion = boost::fusion;
namespace mpl = boost::mpl;
namespace karma = boost::spirit::karma;
namespace qi = boost::spirit::qi;
namespace spirit = boost::spirit;
namespace phoenix = boost::phoenix;

template <typename A>
struct reply_types
{
  typedef std::vector<char>::iterator iterator_type;
  typedef boost::variant<system_exception, user_exception, A> variant_attribute_type;

  typedef fusion::vector4<service_context_list, unsigned int, unsigned int
                          , variant_attribute_type>
    reply_attribute_type;
  typedef fusion::vector1<reply_attribute_type>
    message_attribute_type;
  typedef giop::grammars::system_exception_reply_body
    <iiop::parser_domain, iterator_type, system_exception>
    system_exception_grammar;
  typedef giop::grammars::reply_1_0<iiop::parser_domain
                                    , iterator_type, reply_attribute_type>
    reply_grammar;
  typedef giop::grammars::message_1_0
    <iiop::parser_domain
     , iterator_type, message_attribute_type, 1u /* Reply */>
    message_grammar;
  system_exception_grammar system_exception_grammar_;

  reply_grammar reply_grammar_;

  message_grammar message_grammar_;
  message_attribute_type attribute;

  template <typename U>
  reply_types(U const& args_grammar)
    : reply_grammar_
      (
       (
        spirit::eps(phoenix::at_c<2u>(spirit::_val) == 0u)
        & args_grammar
       ) |
       (
        spirit::eps(phoenix::at_c<2u>(spirit::_val) == 1u)
        & spirit::attr_cast<user_exception>(giop::string)
       ) |
       (
        spirit::eps(phoenix::at_c<2u>(spirit::_val) == 2u)
        & system_exception_grammar_
       )
      )
    , message_grammar_(reply_grammar_)
  {}  
};

template <typename OutGrammar, typename Out>
void read_reply(boost::asio::ip::tcp::socket& socket
                , OutGrammar const& out_grammar
                , Out& out
                //                , service_context_list&)
                )
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

  typedef reply_types<Out> reply_type;
  reply_type reply(out_grammar);

  bool g = qi::parse(first, last
                     , giop::compile<iiop::parser_domain>
                     (reply.message_grammar_)
                     , reply.attribute)
    && first == last;
  
  OB_DIAG_REQUIRE(g, "Parsing reply successfully"
                  , "Parsing reply failed. This is a bug in the diagnostic or a bug in OpenBus")

  typename reply_type::variant_attribute_type variant_attr
    = fusion::at_c<3u>(fusion::at_c<0u>(reply.attribute));

  OB_DIAG_FAIL(system_exception* p = boost::get<system_exception>(&variant_attr)
               , "The following system exception was thrown: " << p->exception_id << " minor code: " << p->minor_code_value)
  OB_DIAG_FAIL(user_exception* p = boost::get<user_exception>(&variant_attr)
               , "The following user exception was thrown: " << p->exception_id)

  out = boost::get<Out>(variant_attr);
}

}

#endif

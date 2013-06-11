/* (c) Copyright 2012 Felipe Magno de Almeida
*
* Distributed under the Boost Software License, Version 1.0. (See
* accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef OB_DIAG_CREATE_SESSION_HPP
#define OB_DIAG_CREATE_SESSION_HPP

#include <ob-diag/session.hpp>
#include <ob-diag/make_request.hpp>
#include <ob-diag/read_system_exception.hpp>

namespace ob_diag {

template <typename ArgsGrammar, typename Args>
session create_session(boost::asio::ip::tcp::socket& socket
                       , std::vector<char> const& object_key
                       , std::string const& method
                       , ArgsGrammar const& args_grammar
                       , Args const& args
                       , std::string const& bus_id
                       , std::string const& login_id
                       , EVP_PKEY* key)
{
  std::vector<char> credential_data;

  giop::forward_back_insert_iterator<std::vector<char> > iterator(credential_data);
  bool g = karma::generate(iterator, giop::compile<iiop::generator_domain>
                           (
                            giop::endianness(giop::native_endian)
                            [
                             giop::string & giop::string
                             & giop::ulong_ & giop::ulong_
                             & +giop::octet
                             // Signed Call Chain
                             & +giop::octet
                             & giop::sequence[giop::octet]
                            ]
                           )
                           , fusion::make_vector
                           (bus_id, login_id, 0u, 0u, std::vector<unsigned char>(32u)
                            , std::vector<unsigned char>(256u), std::string()));

  OB_DIAG_REQUIRE(g, "Generated service context for starting session"
                  , "Failed generating context for starting session. This is a bug in the diagnostic tool")

  {
    ob_diag::service_context_list service_context;
    service_context.push_back
      (fusion::make_vector(0x42555300, credential_data)); 
    make_request(socket, object_key, method, args_grammar, args, service_context);
  }

  ob_diag::service_context_list service_context;
  system_exception ex = read_system_exception(socket, service_context);

  OB_DIAG_FAIL(service_context.size() == 0
               , "System exception thrown doesn't contain any ServiceContext. Can't create session.")
  
  OB_DIAG_FAIL(ex.exception_id != "IDL:omg.org/CORBA/NO_PERMISSION:1.0"
               , "System exception thrown is " << ex.exception_id
               << ". Expecting \"IDL:omg.org/CORBA/NO_PERMISSION:1.0\" exception")

  OB_DIAG_FAIL(ex.minor_code_value != 0x42555300
               , "Minor code of system exception thrown is " << ex.minor_code_value
               << ". Expecting 0x42555300 in exception")

  OB_DIAG_FAIL(fusion::at_c<0u>(service_context[0]) != 0x42555300
               , "Service Context id of system exception thrown is " << fusion::at_c<0u>(service_context[0])
               << ". Expecting 0x42555300 in exception")

  fusion::vector3<std::string, unsigned int, std::vector<char> > credential_reset;
  std::vector<char>::iterator first = fusion::at_c<1u>(service_context[0]).begin()
    , last = fusion::at_c<1u>(service_context[0]).end();
  g = qi::parse(first, last
                , giop::compile<iiop::parser_domain>
                (giop::endianness
                 [
                  giop::string & giop::ulong_ & (spirit::repeat(256u)[giop::octet])
                 ])
                , credential_reset)
    && first == last;

  OB_DIAG_REQUIRE(g, "Parsing credential reset successfully in context"
                  , "Parsing credential reset failed in context. This is a bug in the diagnostic or a bug in OpenBus")

  std::vector<char> secret;
  {
    std::vector<char>const& challange = fusion::at_c<2u>(credential_reset);
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(key, 0);
    EVP_PKEY_decrypt_init(ctx);
    ::size_t out_length = 0;
    EVP_PKEY_decrypt(ctx, 0, &out_length, (unsigned char*)&challange[0], challange.size());
    secret.resize(out_length);
    EVP_PKEY_decrypt(ctx, (unsigned char*)&secret[0], &out_length
                     , (unsigned char*)&challange[0], challange.size());
  }
  assert(secret.size() >= 16u);
  secret.resize(16u);

  return session(fusion::at_c<0u>(credential_reset)
                 , fusion::at_c<1u>(credential_reset), secret);
}

}

#endif

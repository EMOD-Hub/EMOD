#pragma once

#ifndef USE_BOOST_GENERAL
#define USE_BOOST_GENERAL 1
#endif

#ifndef USE_BOOST_ALGORITHM
#define USE_BOOST_ALGORITHM 1
#endif

#ifndef USE_BOOST_MATH
#define USE_BOOST_MATH 1
#endif

#ifndef USE_BOOST_ASSIGN
#define USE_BOOST_ASSIGN 1
#endif

#ifndef USE_BOOST_UUID
#define USE_BOOST_UUID 1
#endif


#pragma warning (disable: 4267)
#pragma warning (disable: 4244)
#pragma warning (disable: 4503)
#pragma warning (disable: 4250)

#if USE_BOOST_GENERAL
#include <boost/scoped_ptr.hpp>
#include <boost/bimap.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#endif

#if USE_BOOST_ALGORITHM
#include <boost/algorithm/string.hpp>
#endif

#if USE_BOOST_ASSIGN
#include <boost/assign/list_of.hpp>
#endif

#if USE_BOOST_UUID
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#endif

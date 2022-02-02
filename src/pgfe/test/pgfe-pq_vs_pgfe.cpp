// -*- C++ -*-
// Copyright (C) 2022 Dmitry Igrishin
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Dmitry Igrishin
// dmitigr@gmail.com

#include "pgfe-unit.hpp"
#include "../../pgfe.hpp"

#include <libpq-fe.h>

#include <new>

const char* const query = "select generate_series(1,100000)";

struct Result final {
  int length{};
  int format{};
  char* value{};
  int is_null{};
  std::unique_ptr<PGresult> res_;
};

auto result(PGresult* res)
{
  Result r;
  r.length = PQgetlength(res, 0, 0);
  r.format = PQfformat(res, 0);
  r.value = PQgetvalue(res, 0, 0);
  r.is_null = PQgetisnull(res, 0, 0);
  r.res_.reset(res);
  return r;
}

void test_pq()
{
  auto* const conn = PQconnectdb("hostaddr=127.0.0.1 user=pgfe_test"
    " password=pgfe_test dbname=pgfe_test connect_timeout=7");

  if (!conn)
    throw std::bad_alloc{};

  if (PQstatus(conn) != CONNECTION_OK) {
    PQfinish(conn);
    throw std::runtime_error{"cannot connect to server"};
  }

  if (!PQsendQuery(conn, query)) {
    PQfinish(conn);
    throw std::runtime_error{"cannot send query"};
  }

  if (!PQsetSingleRowMode(conn)) {
    PQfinish(conn);
    throw std::runtime_error{"cannot switch to single row mode"};
  }

  while (auto* const res = PQgetResult(conn)) {
    switch (PQresultStatus(res)) {
    case PGRES_TUPLES_OK:
      PQclear(res);
      break;
    case PGRES_SINGLE_TUPLE: {
      auto r = result(res);
      break;
    } default:
      PQclear(res);
      PQfinish(conn);
      throw std::runtime_error{PQresultErrorMessage(res)};
    }
  }

  PQfinish(conn);
}

void test_pgfe()
{
  namespace pgfe = dmitigr::pgfe;
  pgfe::Connection conn{pgfe::Connection_options{
    pgfe::Communication_mode::net}.net_address("127.0.0.1").username("pgfe_test")
      .password("pgfe_test").database("pgfe_test").connect_timeout(std::chrono::seconds{7})};
  conn.connect();
  conn.execute([](auto&& r) { auto d = r.data(); }, query);
}

int main()
{
  using dmitigr::util::with_measure;
  std::cout << "Pq: ";
  const auto elapsed_pq = with_measure(test_pq);
  std::cout << elapsed_pq.count() << std::endl;
  std::cout << "Pgfe: ";
  const auto elapsed_pgfe = with_measure(test_pgfe);
  std::cout << elapsed_pgfe.count() << std::endl;
}
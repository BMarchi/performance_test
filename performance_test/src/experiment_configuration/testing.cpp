//
// Created by monika.idzik on 9/5/19.
//
// file      : relationship/driver.cxx
// copyright : not copyrighted - public domain

#include <memory>   // std::auto_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

#include "testing.hpp"
#include "testing_odb.hpp"

using namespace std;
using namespace odb::core;

void
print (const employee& e)
{
  cout << e.first () << " " << e.last () << endl
       << "  employer: " << e.employer ()->name () << endl;

  const projects& ps (e.projects ());

  for (projects::const_iterator i (ps.begin ()); i != ps.end (); ++i)
  {
    tr1::shared_ptr<project> p (*i);
    cout << "  project: " << p->name () << endl;
  }

  cout << endl;
}

int
main (int argc, char* argv[])
{
  using tr1::shared_ptr;

  try
  {
    std::string exe_name = EXE_NAME;
    std::string db_type = "--database";
    std::string exec = "./" + exe_name;
    std::string db_name = "test_database";

    char * argv_db[] = {&exec[0], &db_type[0], &db_name[0]};
    int argc_db = sizeof(argv_db) / sizeof(argv_db[0]);

    std::unique_ptr<database> db = std::unique_ptr<database>(new
        odb::sqlite::database(argc_db, argv_db, false, SQLITE_OPEN_READWRITE |
        SQLITE_OPEN_CREATE));
    {
      odb::core::connection_ptr c(db->connection());
      c->execute("PRAGMA foreign_keys=OFF");
      odb::core::transaction t(c->begin());
      try {
        db->query<database>(false);
      } catch (const odb::exception & e) {
        odb::core::schema_catalog::create_schema(*db);
      }
      t.commit();
      c->execute("PRAGMA foreign_keys=ON");
    }
    // Create a few persistent objects.
    //
    {
      // Simple Tech Ltd.
      //
      {
        shared_ptr<employer> er (new employer ("Simple Tech Ltd"));

        shared_ptr<project> sh (new project ("Simple Hardware"));
        shared_ptr<project> ss (new project ("Simple Software"));

        shared_ptr<employee> john (new employee ("John", "Doe", er));
        shared_ptr<employee> jane (new employee ("Jane", "Doe", er));

        john->projects ().push_back (sh);
        john->projects ().push_back (ss);
        jane->projects ().push_back (ss);

        transaction t (db->begin ());

        db->persist (er);

        db->persist (sh);
        db->persist (ss);

        db->persist (john);
        db->persist (jane);

        t.commit ();
      }

      // Complex Systems Inc.
      //
      {
        shared_ptr<employer> er (new employer ("Complex Systems Inc"));

        shared_ptr<project> ch (new project ("Complex Hardware"));
        shared_ptr<project> cs (new project ("Complex Software"));

        shared_ptr<employee> john (new employee ("John", "Smith", er));
        shared_ptr<employee> jane (new employee ("Jane", "Smith", er));

        john->projects ().push_back (cs);
        jane->projects ().push_back (ch);
        jane->projects ().push_back (cs);

        transaction t (db->begin ());

        db->persist (er);

        db->persist (ch);
        db->persist (cs);

        db->persist (john);
        db->persist (jane);

        t.commit ();
      }
    }

    typedef odb::query<employee> query;
    typedef odb::result<employee> result;

    // Load employees with "Doe" as the last name and print what we've got.
    // We use a session in this and subsequent transactions to make sure
    // that a single instance of any particular object (e.g., employer) is
    // shared among all objects (e.g., employee) that relate to it.
    //
    {
      session s;
      transaction t (db->begin ());

      result r (db->query<employee> (query::last == "Doe"));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        print (*i);

      t.commit ();
    }

    // John Doe has moved to Complex Systems Inc and is now working on
    // Complex Hardware.
    //
    {
      session s;
      transaction t (db->begin ());

      shared_ptr<employer> csi (db->load<employer> ("Complex Systems Inc"));
      shared_ptr<project> ch (db->load<project> ("Complex Hardware"));

      shared_ptr<employee> john (
          db->query_one<employee> (query::first == "John" &&
                                   query::last == "Doe"));

      john->employer (csi);
      john->projects ().clear ();
      john->projects ().push_back (ch);

      db->update (john);

      t.commit ();
    }

    // We can also use members of the pointed-to objects in the queries. The
    // following transaction prints all the employees of Complex Systems Inc.
    //
    {
      session s;
      transaction t (db->begin ());

      result r (db->query<employee> (
          query::employer->name == "Complex Systems Inc"));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        print (*i);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test DOI::help() function", "[DOI][plugins]" )
{
#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("DOI::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage() == "" );
    REQUIRE( result.isTree()  );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "help" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "STRING" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Scalar value = child.atomicScalar("help");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(char*).name() );
    REQUIRE( value.size() == 0 );

    const char* expected = "\nIssue a new DOI for a specific scientific study.\n\n"
            "get(owner=owner, icatRef=icatRef)\n\n"
            "Issue a new DOI with a pending status\n\n"
            "status(doi=doi, status=[Delete | Firm | Pending])\n\n"
            "Enquire about or Change a DOI's status\n\n"
            "list(doi=doi)\n\n"
            "put(user=user, doi=doi, requestedSignal=requestedSignal, requestedSource=requestedSource, \n"
            "trueSignal=trueSignal, trueSource=trueSource, trueSourceDOI=trueSourceDOI, \n"
            "logRecord=logRecord, created=created, status=[New|Update|Close|Delete])\n\n";

    REQUIRE( std::string(expected) == value.as<char*>() );
}

TEST_CASE( "Test DOI::list() function", "[DOI][plugins]" )
{
#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("DOI::list(doi=ccfe/1, dbname=doi, dbuser=idamowner, dbhost=idam1.mast.ccfe.ac.uk, dbport=56566)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage() == "" );
    REQUIRE( result.isTree()  );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

// doi_log_id |  doi   | requestedsignal | requestedsource |     truesignal     |   truesource    | truesourcedoi | logrecord | status |  creation  | tmpkey
//------------+--------+-----------------+-----------------+--------------------+-----------------+---------------+-----------+--------+------------+--------
//          1 | ccfe/1 | ip              | 13500           | amc_plasma current |                 | qwerty        |           |      0 | 2014-01-29 |
//          2 | ccfe/1 | ip              | 13500           | amc_plasma current |                 | qwerty        |           |      0 | 2014-01-29 |
//          3 | ccfe/1 | ip              | 13500           | amc_plasma current |                 | qwerty        |           |      0 | 2014-01-29 |
//          7 | ccfe/1 | xxx             | yyy             | asdfghj            | /abs/def/fgh.vc | ccfe/999      |           |      0 | 2014-01-29 |
//          8 | ccfe/1 | xxx             | yyy             | asdfghj            | /abs/def/fgh.vc | ccfe/999      |           |      0 | 2014-02-04 |
//          9 | ccfe/1 | xxx             | yyy             | asdfghj            | /abs/def/fgh.vc | ccfe/999      |           |      0 | 2014-02-04 |
//         10 | ccfe/1 | xxx             | yyy             | asdfghj            | /abs/def/fgh.vc | ccfe/999      |           |      0 | 2014-02-04 |
//         12 | ccfe/1 | xxx             | yyy             | asdfghj            | /abs/def/fgh.vc | ccfe/999      |           |      0 | 2014-02-04 |
//         14 | ccfe/1 | xxx             | yyy             | asdfghj            | /abs/def/fgh.vc | ccfe/999      |           |      0 | 2014-02-04 |
//         16 | ccfe/1 | xxx             | yyy             | asdfghj            | /abs/def/fgh.vc | ccfe/999      |           |      0 | 2014-02-04 |
//         31 | ccfe/1 | xxx             | yyy             | asdfghj            | /abs/def/fgh.vc | ccfe/999      |           |      0 | 2014-02-05 |
//         43 | ccfe/1 | xxx             | yyy             | asdfghj            | /abs/def/fgh.vc | ccfe/999      |           |      0 | 2014-02-05 |

    const char* expected_dois[] = { "ccfe/1", "ccfe/1", "ccfe/1", "ccfe/1", "ccfe/1", "ccfe/1", "ccfe/1", "ccfe/1", "ccfe/1", "ccfe/1", "ccfe/1", "ccfe/1" };
    const char* expected_requestedsignals[] = { "ip", "ip", "ip", "xxx", "xxx", "xxx", "xxx", "xxx", "xxx", "xxx", "xxx", "xxx" };
    const char* expected_requestedsources[] = { "13500", "13500", "13500", "yyy", "yyy", "yyy", "yyy", "yyy", "yyy", "yyy", "yyy", "yyy" };
    const char* expected_truesignals[] = { "amc_plasma current", "amc_plasma current", "amc_plasma current", "asdfghj", "asdfghj", "asdfghj", "asdfghj", "asdfghj", "asdfghj", "asdfghj", "asdfghj", "asdfghj" };
    const char* expected_truesources[] = { "", "", "", "/abs/def/fgh.vc", "/abs/def/fgh.vc", "/abs/def/fgh.vc", "/abs/def/fgh.vc", "/abs/def/fgh.vc", "/abs/def/fgh.vc", "/abs/def/fgh.vc", "/abs/def/fgh.vc", "/abs/def/fgh.vc" };
    const char* expected_truesourcedois[] = { "qwerty", "qwerty", "qwerty", "ccfe/999", "ccfe/999", "ccfe/999", "ccfe/999", "ccfe/999", "ccfe/999", "ccfe/999", "ccfe/999", "ccfe/999" };
    const char* expected_logrecords[] = { "", "", "", "", "", "", "", "", "", "", "", "" };
    const char* expected_creations[] = { "2014-01-29", "2014-01-29", "2014-01-29", "2014-01-29", "2014-02-04", "2014-02-04", "2014-02-04", "2014-02-04", "2014-02-04", "2014-02-04", "2014-02-05", "2014-02-05" };

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 12 );

    for (int i = 0; i < 12; ++i) {
        uda::TreeNode subchild = child.child(i);

        REQUIRE( subchild.atomicCount() == 8 );

        const char* expected_names[] = { "doi", "requestedSignal", "requestedSource", "trueSignal", "trueSource", "trueSourceDOI", "logRecord", "creation" };
        REQUIRE( subchild.atomicNames() == std::vector<std::string>(expected_names, expected_names + 8) );

        bool expected_pointers[] = { true, true, true, true, true, true, true, true };
        REQUIRE( subchild.atomicPointers() == std::vector<bool>(expected_pointers, expected_pointers + 8) );

        const char* expected_types[] = { "STRING", "STRING", "STRING", "STRING", "STRING", "STRING", "STRING", "STRING" };
        REQUIRE( subchild.atomicTypes() == std::vector<std::string>(expected_types, expected_types + 8) );

        size_t expected_ranks[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        REQUIRE( subchild.atomicRank() == std::vector<size_t>(expected_ranks, expected_ranks + 8) );

        uda::Scalar doi = subchild.atomicScalar("doi");
        REQUIRE( !doi.isNull() );
        REQUIRE( doi.type().name() == typeid(char*).name() );
        REQUIRE( doi.size() == 0 );
        REQUIRE( std::string(expected_dois[i]) == doi.as<char*>() );

        uda::Scalar requestedSignal = subchild.atomicScalar("requestedSignal");
        REQUIRE( !requestedSignal.isNull() );
        REQUIRE( requestedSignal.type().name() == typeid(char*).name() );
        REQUIRE( requestedSignal.size() == 0 );
        REQUIRE( std::string(expected_requestedsignals[i]) == requestedSignal.as<char*>() );

        uda::Scalar requestedSource = subchild.atomicScalar("requestedSource");
        REQUIRE( !requestedSource.isNull() );
        REQUIRE( requestedSource.type().name() == typeid(char*).name() );
        REQUIRE( requestedSource.size() == 0 );
        REQUIRE( std::string(expected_requestedsources[i]) == requestedSource.as<char*>() );

        uda::Scalar trueSignal = subchild.atomicScalar("trueSignal");
        REQUIRE( !trueSignal.isNull() );
        REQUIRE( trueSignal.type().name() == typeid(char*).name() );
        REQUIRE( trueSignal.size() == 0 );
        REQUIRE( std::string(expected_truesignals[i]) == trueSignal.as<char*>() );

        uda::Scalar trueSource = subchild.atomicScalar("trueSource");
        REQUIRE( !trueSource.isNull() );
        REQUIRE( trueSource.type().name() == typeid(char*).name() );
        REQUIRE( trueSource.size() == 0 );
        REQUIRE( std::string(expected_truesources[i]) == trueSource.as<char*>() );

        uda::Scalar trueSourceDOI = subchild.atomicScalar("trueSourceDOI");
        REQUIRE( !trueSourceDOI.isNull() );
        REQUIRE( trueSourceDOI.type().name() == typeid(char*).name() );
        REQUIRE( trueSourceDOI.size() == 0 );
        REQUIRE( std::string(expected_truesourcedois[i]) == trueSourceDOI.as<char*>() );

        uda::Scalar logRecord = subchild.atomicScalar("logRecord");
        REQUIRE( !logRecord.isNull() );
        REQUIRE( logRecord.type().name() == typeid(char*).name() );
        REQUIRE( logRecord.size() == 0 );
        REQUIRE( std::string(expected_logrecords[i]) == logRecord.as<char*>() );

        uda::Scalar creation = subchild.atomicScalar("creation");
        REQUIRE( !creation.isNull() );
        REQUIRE( creation.type().name() == typeid(char*).name() );
        REQUIRE( creation.size() == 0 );
        REQUIRE( std::string(expected_creations[i]) == creation.as<char*>() );
    }
}
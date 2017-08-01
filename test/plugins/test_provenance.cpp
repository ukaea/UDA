#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test PROV::help() function", "[PROV][plugins]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("PROV::help()", "");

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

    const char* expected = "\nProvenance: Issue and register a new UUID for a specific scientific study.\n\n"

            "Summary of functions:\n\n"
            "get\t\tRegister and return a new UUID with an 'OPEN' status\n"
            "status\t\tEnquire about or change a registered provenance UUID's status\n"
            "put\t\tRecord metadata in the Provenance database and copy files to the Provenance data archive\n"
            "listSignals\tList all signals accessed with a specific client's UUID\n"
            "putSignal\tRecord a signal's use by an application with a specific UUID\n\n\n"

            "get(owner=owner, status=status, class=class, title=title, description=description, "
            "icatRef=icatRef, /returnUUID)\n\n"

            "status(uuid=uuid, status=[Open | Closed | Delete | Firm | Pending], /returnStatus)\n\n"

            "put(uuid=uuid, fileLocation=fileLocation, format=format)\n\n"

            "listSignals(uuid=uuid)\n\n"

            "putSignal(user=user, uuid=uuid, requestedSignal=requestedSignal, requestedSource=requestedSource, \n"
            "\ttrueSignal=trueSignal, trueSource=trueSource, trueSourceUUID=trueSourceUUID, \n"
            "\tlogRecord=logRecord, created=created, status=[New|Update|Close|Delete])\n\n";

    REQUIRE( std::string(expected) == value.as<char*>() );
}

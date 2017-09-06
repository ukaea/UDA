#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test MONGO::help() function", "[MONGO][plugins]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("MONGO::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nMongoDBPlugin: Function Names, Syntax, and Descriptions\n\n"
            "Query the mongoDB IDAM database for specific instances of a data object by alias or generic name and shot number\n\n"
            "\tquery( [signal|objectName]=objectName, [shot|exp_number|pulse|pulno]=exp_number [,source|objectSource=objectSource] [,device=device] [,/allMeta])\n"
            "\t       [objectClass=objectClass] [,sourceClass=sourceClass] [,type=type])\n\n"
            "\tobjectName: The alias or generic name of the data object.\n"
            "\texp_number: The experiment shot number. This may be passed via the client API's second argument.\n"
            "\tobjectSource: the abstract name of the source. This may be passed via the client API's second argument, either alone or with exp_number [exp_number/objectSource]\n"
            "\tdevice: the name of the experiment device, e.g. ITER\n"
            "\tallMeta: a keyword to request the full database record be returned\n"
            "\tobjectClass: the name of the data's measurement class, e.g. magnetics\n"
            "\tsourceClass: the name of the data's source class, e.g. imas\n"
            "\ttype: the data type classsification, e.g. P for Plugin\n\n"

            "\tobjectName is a mandatory argument. One or both of exp_number and ObjectSource is also mandatory unless passed via the client API's second argument.\n\n"

            "\tExample\tidamGetAPI(\"mongodbplugin::query(signal=ip, shot=12345, device=ITER, /allmeta)\", \"\");\n"
            "\t\tidamGetAPI(\"mongodbplugin::query(signal=ip, device=ITER, /allmeta)\", \"12345\");\n\n";

    REQUIRE( str->str() == expected );
}
